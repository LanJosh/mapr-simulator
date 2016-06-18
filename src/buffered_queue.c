#include "buffered_queue.h"
#include <stdio.h>
#include <stdlib.h>

void buffered_queue_init(struct buffered_queue *queue, int size){
    queue->size = size+1;
    queue->buf = malloc(sizeof(void *) * (size+1));
    queue->start_index = 0;
    queue->end_index = 0;
    pthread_mutex_init(&(queue->lock), NULL);
    pthread_cond_init(&(queue->cond), NULL);
}

void buffered_queue_push(struct buffered_queue *queue, void *item){
    pthread_mutex_lock(&(queue->lock));
    while((queue->end_index+1)%queue->size == queue->start_index)
        pthread_cond_wait(&(queue->cond), &(queue->lock));
    queue->buf[queue->end_index] = item;
    queue->end_index = (queue->end_index+1)%queue->size;
    if((queue->start_index+1)%queue->size == queue->end_index)
    	pthread_cond_broadcast(&(queue->cond));

	pthread_mutex_unlock(&(queue->lock));
}

void* buffered_queue_pop(struct buffered_queue *queue){
    void *ret = NULL;
    pthread_mutex_lock(&(queue->lock));
    while(queue->end_index == queue->start_index)
        pthread_cond_wait(&(queue->cond), &(queue->lock));
    ret = queue->buf[queue->start_index];
    queue->start_index = (queue->start_index+1)%queue->size;
    if((queue->end_index+2)%queue->size == queue->start_index)
    	pthread_cond_broadcast(&(queue->cond));
    pthread_mutex_unlock(&(queue->lock));

    return ret;
}

void buffered_queue_destroy(struct buffered_queue *queue){
    free(queue->buf);
    pthread_mutex_destroy(&(queue->lock));
    pthread_cond_destroy(&(queue->cond));
}
