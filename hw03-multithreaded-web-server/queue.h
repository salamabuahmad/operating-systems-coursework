#ifndef QUEUE_H
#define QUEUE_H
#include <sys/time.h>

typedef struct Job {
    int jobFd;
    struct timeval arrivalTime;
    struct Job* next;
} Job;

typedef struct queue{
    int currSize;
    int maxSize;
    Job * head;
    Job * tail;
}Queue;

Job* createJob(int fd,struct timeval time);
Queue* createQueue(int size);
void enqueue(Queue* queue, Job* job);
Job* dequeue(Queue* queue);
int isEmpty(Queue * queue);

#endif 