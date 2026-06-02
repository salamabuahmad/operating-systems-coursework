#include "segel.h"
#include "request.h"
#include "log.h"
#include "queue.h"
#include <pthread.h>

//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

Queue *waitingQueue;
pthread_mutex_t queue_lock;
pthread_cond_t queue_not_empty;
pthread_cond_t queue_not_full;

int active_jobs = 0;
server_log global_log;


// Parses command-line arguments
void getargs(int *port, int *num_threads, int *queue_size, double *debug_sleep, int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <port> <threads> <queue_size> <debug_sleep_time>\n", argv[0]);
        exit(1);
    }

    *port = atoi(argv[1]);
    *num_threads = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    *debug_sleep = atof(argv[4]);


    if (*port < 1024 || *port > 65534 || *num_threads <= 0 || *queue_size <= 0) {
        exit(1);
    }
}

void* thread_function(void* arg) {
    int thread_id = *((int*)arg);

    threads_stats t = malloc(sizeof(struct Threads_stats));
    t->id = thread_id;
    t->stat_req = 0;
    t->dynm_req = 0;
    t->post_req = 0;
    t->total_req = 0;

    while (1) {
        pthread_mutex_lock(&queue_lock);
        while (isEmpty(waitingQueue)) {
            pthread_cond_wait(&queue_not_empty, &queue_lock);
        }

        Job *job = dequeue(waitingQueue);
        active_jobs++;

        pthread_cond_signal(&queue_not_full); // signal master thread that space opened
        pthread_mutex_unlock(&queue_lock);

        // Handle the request
        time_stats tm;
        tm.task_arrival = job->arrivalTime;
        gettimeofday(&tm.task_dispatch, NULL);
        requestHandle(job->jobFd, tm, t, global_log);

        close(job->jobFd);
        free(job);

        pthread_mutex_lock(&queue_lock);
        active_jobs--;
        pthread_cond_signal(&queue_not_full); // signal again after finishing job
        pthread_mutex_unlock(&queue_lock);
    }

    free(t);
    return NULL;
}

// TODO: HW3 — Initialize thread pool and request queue
// This server currently handles all requests in the main thread.
// You must implement a thread pool (fixed number of worker threads)
// that process requests from a synchronized queue.


int main(int argc, char *argv[]) {
    global_log = create_log();

    int listenfd, connfd, port, clientlen, thread_count, queue_size;
    double debug_sleep_time;
    struct sockaddr_in clientaddr;

    getargs(&port, &thread_count, &queue_size, &debug_sleep_time, argc, argv);

    set_log_debug_sleep(global_log, debug_sleep_time);

    waitingQueue = createQueue(queue_size);
    pthread_mutex_init(&queue_lock, NULL);
    pthread_cond_init(&queue_not_empty, NULL);
    pthread_cond_init(&queue_not_full, NULL);

    pthread_t *thread_pool = malloc(thread_count * sizeof(pthread_t));
    if (thread_pool == NULL) exit(1);

    int *thread_ids = malloc(thread_count * sizeof(int));
    if (thread_ids == NULL) exit(1);

    for (int i = 0; i < thread_count; i++) {
        thread_ids[i] = i+1;
        pthread_create(&thread_pool[i], NULL, thread_function, &thread_ids[i]);
    }

    listenfd = Open_listenfd(port);
    while (1) {
        pthread_mutex_lock(&queue_lock);

        // Wait until there's space before calling accept
        while (waitingQueue->currSize + active_jobs >= waitingQueue->maxSize) {
            pthread_cond_wait(&queue_not_full, &queue_lock); // block when full
        }

        pthread_mutex_unlock(&queue_lock);

        // Now it's safe to accept the connection
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        struct timeval arrival;
        gettimeofday(&arrival, NULL);
        Job* job = createJob(connfd, arrival);

        pthread_mutex_lock(&queue_lock);
        enqueue(waitingQueue, job);
        pthread_cond_signal(&queue_not_empty); // signal worker
        pthread_mutex_unlock(&queue_lock);
    }

    // Cleanup (only reached if server shuts down, e.g., with signal handler)
    for (int i = 0; i < thread_count; i++) {
        pthread_cancel(thread_pool[i]);
        pthread_join(thread_pool[i], NULL);
    }

    free(thread_ids);
    free(thread_pool);

    while (!isEmpty(waitingQueue)) {
        Job *job = dequeue(waitingQueue);
        if (job) {
            close(job->jobFd);
            free(job);
        }
    }
    free(waitingQueue);

    pthread_mutex_destroy(&queue_lock);
    pthread_cond_destroy(&queue_not_empty);
    pthread_cond_destroy(&queue_not_full);

    destroy_log(global_log);

    return 0;
}