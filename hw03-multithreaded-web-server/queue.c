#include "queue.h"
#include <stdio.h>
#include <stdlib.h>


Job* createJob(int fd,struct timeval time){
    Job* job= (Job*)malloc(sizeof(Job));
    if(!job){
        exit(1);
    }
    job->jobFd = fd;
    job->arrivalTime = time;
    job->next = NULL;
    return job;
}

Queue* createQueue(int size){
    Queue* queue= (Queue*)malloc(sizeof(Queue));
    if(!queue){
        exit(1);
    }
    queue->currSize = 0;
    queue->maxSize = size;
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

void enqueue(Queue* queue, Job* job){
    //if(queue->currSize == queue->maxSize) return;
    if(queue->tail == NULL){
        queue->head = job;
        queue->tail = job;
    }
    else{
        queue->tail->next = job;
        queue->tail = job;
    }
    queue->currSize++;
}

//The function return null if the queue is empty
Job* dequeue(Queue* queue){
    if(queue->currSize == 0) return NULL;
    if(queue->head == NULL){
        return NULL;
    }

    Job * temp = queue->head;
    queue->head = queue->head->next;
    if(queue->head == NULL){
        queue->tail = NULL;
    }
    queue->currSize--;
    //This was causing the bug
    temp->next = NULL;
    return temp;
}

int isEmpty(Queue * queue){
    return queue->head == NULL;
}

