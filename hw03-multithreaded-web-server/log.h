#ifndef SERVER_LOG_H
#define SERVER_LOG_H

#include <sys/time.h>

// TODO:
// Implement a thread-safe server log system.
// - The log should support concurrent access from multiple threads.
// - You must implement a multiple-readers/single-writer synchronization model.
// - Writers must have priority over readers.
//   This means that if a writer is waiting, new readers should be blocked until the writer is done.
// - Use appropriate synchronization primitives (e.g., pthread mutexes and condition variables).
// - The log should allow appending entries and returning the full log content.

typedef struct Threads_stats {
    int id;           // Thread ID
    int stat_req;     // Number of static requests handled
    int dynm_req;     // Number of dynamic requests handled
    int post_req;     // Number of POST requests handled
    int total_req;    // Total number of requests handled
} * threads_stats;

typedef struct Time_stats {
    struct timeval task_arrival;
    struct timeval task_dispatch;
    struct timeval log_enter;
    struct timeval log_exit;
} time_stats;

typedef struct Server_Log* server_log;

// Creates a new server log instance
server_log create_log();

// Destroys and frees the log
void destroy_log(server_log log);

// Returns the log contents as a string (null-terminated)
// NOTE: caller is responsible for freeing dst
int get_log(server_log log, char** dst, struct timeval *log_exit);

// Appends a new entry to the log
/*void add_to_log(server_log log, const char* data, int data_len, struct timeval *log_exit);*/

void add_to_log(server_log log,
                threads_stats t_stats,
                time_stats *tm_stats);

// Sets debug sleep time for the log
void set_log_debug_sleep(server_log log, double debug_sleep_time);

#endif // SERVER_LOG_H
