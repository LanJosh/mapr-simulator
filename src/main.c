#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <word_count.h>
#include <dirent.h>
#include <sys/types.h>


int main(int argc, char **args) {
    if(argc<=3){
	    printf("Usage: ./main [m] [r] [\\path\\to\\directory]\n");
	    return -1;
    }
    int m = atoi(args[1]);
    int r = atoi(args[2]);
    printf("m=%d, r=%d\n", m, r);

    char *directory = args[3];
    char **files = malloc(sizeof(char*) * m); 
    int num_files = 0;
    DIR *d;
    struct dirent *dir;
    d = opendir(directory);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (num_files < m){ 
                if (dir->d_type == 8) {
                    char *path = malloc(strlen(directory) + strlen(dir->d_name) + 2);
                    path[0] = '\0';
                    strcat(path, directory);
                    strcat(path, "/"); 
                    strcat(path, dir->d_name); 
                    files[num_files] = path;
                    num_files++;
					printf("Processed file %d: %s\n", num_files, dir->d_name);
                }
            } else {
                printf("Error, m=%d but there is not %d files\n", m, m);
                return -1;
            }
        }
        closedir(d);
    } else {
        printf("Error opening directory\n");
        return -1;    
    }
                
    printf("File names: \n");
    for(int i=0; i<m; i++)
	    printf("%s\n", files[i]);
    word_count(m, r, files, directory);
    free(files);
    return 0;
}

