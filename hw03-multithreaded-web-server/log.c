#include <stdlib.h>
#include <string.h>
#include "log.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

struct Server_Log {
    char* buffer;             // dynamic log buffer
    int length;               // current log length
    int capacity;             // current buffer capacity
    double debug_sleep_time;     // debug sleep time

    // Reader-writer lock with writer priority
    pthread_mutex_t lock;
    pthread_cond_t readers_allowed;
    pthread_cond_t writers_allowed;
    int readers;              // active readers
    int waiting_writers;      // blocked writers
    int writers;              // active writers (0 or 1)
};

#define INITIAL_CAPACITY 1024
#define MAXBUF   8192
server_log create_log() {
    server_log log = malloc(sizeof(struct Server_Log));
    if (log == NULL) {
        exit(1);
    }

    log->buffer = malloc(INITIAL_CAPACITY);
    if (log->buffer == NULL) {
        free(log);
        exit(1); // or return NULL;
    }

    log->buffer[0] = '\0';
    log->length = 0;
    log->capacity = INITIAL_CAPACITY;
    log->debug_sleep_time = 0; // default debug sleep time
    pthread_mutex_init(&log->lock, NULL);
    pthread_cond_init(&log->readers_allowed, NULL);
    pthread_cond_init(&log->writers_allowed, NULL);
    log->readers = 0;
    log->writers = 0;
    log->waiting_writers = 0;
    return log;
}

void destroy_log(server_log log) {
    if (log) {
        free(log->buffer);
        pthread_mutex_destroy(&log->lock);
        pthread_cond_destroy(&log->readers_allowed);
        pthread_cond_destroy(&log->writers_allowed);
        free(log);
    }
}

void reader_lock(server_log log, struct timeval *log_exit) {
    pthread_mutex_lock(&log->lock);
    while (log->writers > 0 || log->waiting_writers > 0)
        pthread_cond_wait(&log->readers_allowed, &log->lock);

    gettimeofday(log_exit,NULL);
    log->readers++;
    pthread_mutex_unlock(&log->lock);
}

void reader_unlock(server_log log) {
    pthread_mutex_lock(&log->lock);
    log->readers--;
    if (log->readers == 0)
        pthread_cond_signal(&log->writers_allowed);
    pthread_mutex_unlock(&log->lock);
}

void writer_lock(server_log log, struct timeval *log_exit) {
    pthread_mutex_lock(&log->lock);
    log->waiting_writers++;
    while (log->readers > 0 || log->writers > 0)
        pthread_cond_wait(&log->writers_allowed, &log->lock);

    gettimeofday(log_exit,NULL);

    log->waiting_writers--;
    log->writers = 1; // we can do log->writers++ but there is only one writer

    pthread_mutex_unlock(&log->lock);
}

void writer_unlock(server_log log) {
    pthread_mutex_lock(&log->lock);
    log->writers = 0;
    if (log->waiting_writers > 0)
        pthread_cond_signal(&log->writers_allowed);
    else
        pthread_cond_broadcast(&log->readers_allowed);
    pthread_mutex_unlock(&log->lock);
}



int get_log(server_log log, char** dst, struct timeval *log_exit) {
    reader_lock(log,log_exit);

    if (log->debug_sleep_time > 0) {
        usleep((useconds_t)(log->debug_sleep_time * 1e6));
    }


    *dst = malloc(log->length + 1);
    int len = 0;

    if (*dst) {
        memcpy(*dst, log->buffer, log->length + 1);
        len = log->length;
    } else {
        exit(1);
    }
    reader_unlock(log);
    return len;
}

int append_stats_in_log(char* buf, threads_stats t_stats, time_stats tm_stats) {
    int offset = strlen(buf);

    offset += sprintf(buf + offset, "Stat-Req-Arrival:: %ld.%06ld\r\n",
                      tm_stats.task_arrival.tv_sec, tm_stats.task_arrival.tv_usec);
    offset += sprintf(buf + offset, "Stat-Req-Dispatch:: %ld.%06ld\r\n",
                      tm_stats.task_dispatch.tv_sec, tm_stats.task_dispatch.tv_usec);
    offset += sprintf(buf + offset, "Stat-Log-Arrival:: %ld.%06ld\r\n",
                      tm_stats.log_enter.tv_sec, tm_stats.log_enter.tv_usec);
    offset += sprintf(buf + offset, "Stat-Log-Dispatch:: %ld.%06ld\r\n",
                      tm_stats.log_exit.tv_sec, tm_stats.log_exit.tv_usec);

    offset += sprintf(buf + offset, "Stat-Thread-Id:: %d\r\n", t_stats->id);
    offset += sprintf(buf + offset, "Stat-Thread-Count:: %d\r\n", t_stats->total_req);
    offset += sprintf(buf + offset, "Stat-Thread-Static:: %d\r\n", t_stats->stat_req);
    offset += sprintf(buf + offset, "Stat-Thread-Dynamic:: %d\r\n", t_stats->dynm_req);
    offset += sprintf(buf + offset, "Stat-Thread-Post:: %d\r\n\r\n", t_stats->post_req);
    return offset;
}



/*void add_to_log(server_log log, const char* data, int data_len, struct timeval *log_exit) {
    if (!data || data_len <= 0) return;
    writer_lock(log,log_exit);



    if (log->debug_sleep_time > 0) {
        usleep((useconds_t)(log->debug_sleep_time * 1e6));
    }


    if (log->length + data_len + 1 >= log->capacity) {
        while (log->length + data_len + 1 >= log->capacity)
            log->capacity *= 2;

        char* new_buf = realloc(log->buffer, log->capacity);
        if (new_buf == NULL) {
            exit(1);
        }
        log->buffer = new_buf;
    }

    memcpy(log->buffer + log->length, data, data_len);
    log->length += data_len;
    log->buffer[log->length] = '\0';

    writer_unlock(log);
}*/

void add_to_log(server_log log,
                threads_stats t_stats,
                time_stats *tm_stats)
{
    char log_buf[MAXBUF] = {0};
    writer_lock(log, &tm_stats->log_exit);

    if (log->debug_sleep_time > 0) {
        usleep((useconds_t)(log->debug_sleep_time * 1e6));
    }
    int len = append_stats_in_log(log_buf, t_stats, *tm_stats);
    log_buf[len++] = '\n';

    if (log->length + len + 1 >= log->capacity) {
        while (log->length + len + 1 >= log->capacity)
            log->capacity *= 2;
        log->buffer = realloc(log->buffer, log->capacity);
    }
    memcpy(log->buffer + log->length, log_buf, len);
    log->length += len;
    log->buffer[log->length] = '\0';

    writer_unlock(log);
}


void set_log_debug_sleep(server_log log, double debug_sleep_time)
{
    log->debug_sleep_time = debug_sleep_time;
}
