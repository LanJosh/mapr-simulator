#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "word_count.h"
#include "hashmap.h"
#include <pthread.h>
#include "buffered_queue.h"
#include <sys/types.h>

int print_result(char*, int, char*);
void count_string(char*, map_t);

struct mapperArgs {
    int mapperId;
    int reducerCount;
    int *signal;
    char *file;
    struct buffered_queue **conns;
	pthread_mutex_t signal_lock;
};

struct reducerArgs {
    int mapperCount;
    int reducerId;
    map_t *map;
    struct buffered_queue **conns;
}; 


void word_count(int m, int r, char **files, char *directory){
    map_t mymap[r];
    pthread_t mappers[m];    // Thread for each mapper
    pthread_t reducers[r];   // Thread for each reducer
    int *signal = malloc(sizeof(*signal)); // Keep track of finished mappers
    *signal = m;
	pthread_mutex_t lock;
	pthread_mutex_init(&lock, NULL);

    //printf("metting up buffered queue connections\n");
    struct buffered_queue **conns = malloc(sizeof(*conns) * r);// Hold connections

    for (int i = 0; i < r; i++) {
        struct buffered_queue *conn = malloc(sizeof(*conn));
        buffered_queue_init(conn, 2*m); // Let queue fill & wait since text won't fit in memory 
        conns[i] = conn;
    }

    for (int i = 0; i < m; i++){ // Create mapper threads
        struct mapperArgs *mArgs = malloc(sizeof(*mArgs)); // Get arguments for map function
        mArgs->mapperId = i;
        mArgs->reducerCount = r;
        mArgs->file = files[i];
        mArgs->conns = conns;
        mArgs->signal = signal;
		mArgs->signal_lock = lock;
        //printf("Beginning mapper functions\n");
        pthread_create(&mappers[i], NULL, map, (void*)mArgs); // Create thread
    }
    
    //printf("Beginning reducer processes\n"); 
    for (int i = 0; i < r; i++){
		printf("%s\n", directory);
		char *file_no = malloc(sizeof(*file_no));
		sprintf(file_no,"%d", i); 		
		char *file_path = malloc(strlen(directory) + strlen(file_no) + 8);
		strcpy(file_path, directory);
		strcat(file_path, "/output");
		strcat(file_path, file_no);	
		printf("Reducer %d will write to file %s\n", i, file_path);
        mymap[i] = hashmap_new(file_path);
        struct reducerArgs *rArgs = malloc(sizeof(*rArgs));
        rArgs->mapperCount = m;
        rArgs->reducerId = i;
        rArgs->map = &(mymap[i]);
        rArgs->conns = conns;
        pthread_create(&reducers[i], NULL, reduce, (void*)rArgs);
    }


    for (int i = 0; i < m; i++){
        //printf("Waiting for mapper %d to finish\n", i);
        pthread_join(mappers[i], NULL);
    }


    for (int i = 0; i < r; i++){
        //printf("Waiting for reducer %d to finish\n", i);
        pthread_join(reducers[i], NULL);
    }


//    printf("Result:\n");
//    hashmap_iterate(mymap, print_result);	
	
}

int print_result(char *key, int value, char *path){
	FILE *stream = fopen(path, "a");
	fprintf(stream, "%s %d\n", key, value);
	fclose(stream);	
    return 0;
}

// Maps a string to the correct buffered queue
void *map(void* mArgs){
    const char delimiter[2] = " ";
    char *token;
    char *saveptr;
    char *line = NULL;
    ssize_t linelen;
    size_t linecap = 0;
    struct mapperArgs* args = (struct mapperArgs*)mArgs;

//  printf("Mapper %d is reading file %s\n", args->mapperId, args->file);
    FILE *read = fopen(args->file, "r"); 
    while ((linelen = getline(&line, &linecap, read)) > 0) {
		char mod_line[linelen];
		(void)strncpy(mod_line, line, sizeof(mod_line));

		// Cut off the newline character
		if (mod_line[linelen - 1] == '\n') {
			mod_line[linelen - 1] = '\0';
			line = malloc(sizeof(mod_line) - 1);
			memcpy(line, mod_line, sizeof(mod_line) - 1);
		}

	
        printf("Mapper %d read line %s from file %s\n", args->mapperId, line, args->file);
        token = strtok_r(line, delimiter, &saveptr);
        printf("Mapper %d is processing \"%s\"\n", args->mapperId, token);
        while (token != NULL) { 
            /* Find buffered queue to put it in */
            int bucket = (int)(hash(token) % (unsigned long)(args->reducerCount));
            printf("Mapper %d mapped \"%s\" to %d\n", args->mapperId, token, bucket);
            struct buffered_queue *conn = (args->conns)[bucket];
            buffered_queue_push(conn, token);
            token = strtok_r(NULL, delimiter, &saveptr);
        }
		line = NULL;
    }

    // Send signal that mapper is finished
    pthread_mutex_lock(&(args->signal_lock));
    (*(args->signal))--;
	pthread_mutex_unlock(&(args->signal_lock));
    // While mappers are still processing, do nothing 
    while (*(args->signal));
    
    // Tell reducers mappers are finished 
    for (int i = 0; i < args->reducerCount; i++){
        buffered_queue_push((args->conns)[i], NULL);
    }
 
    return NULL;
}

// Grabs a string from a buffered queue and counts it
void *reduce(void* rArgs){
    struct reducerArgs *args = (struct reducerArgs*) rArgs;
    int mappersFinished = 0;
    int error;
    int count;
    map_t map = *(args->map);

    struct buffered_queue *conn = (args->conns)[args->reducerId]; // Corresponding buffered queue

    while (1){ // continually attempt to read until signal received all mappers done
        char *str = (char*)buffered_queue_pop(conn); // Word to reduce from buffered queue
        
        if (str == NULL) {
            // Received mappers finished signal
            break;
        } else {
            printf("Reducer %d is counting \"%s\"\n", args->reducerId, str);
            error = hashmap_get(map, str, &count);
            if (error == MAP_OK){
                hashmap_put(map, str, count + 1);
                printf("Reducer %d counted \"%s\" %d times\n", args->reducerId, str, count + 1);
            } else {
                hashmap_put(map, str, 1);
                printf("Reducer %d counted \"%s\" 1 time\n",args->reducerId, str);
            } 
        }
    }

    hashmap_iterate(map, print_result); 
    
    return NULL;
}

// djb2 hashing to find machine to use
unsigned long hash(char *str){
    unsigned long hash = 5381;
    int c;

    while (c = *str++){
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash;
}

