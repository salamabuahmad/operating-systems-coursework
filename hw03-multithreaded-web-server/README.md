# Multithreaded Web Server

## Overview

This project implements a multithreaded HTTP web server in C.

The server uses a fixed-size thread pool and a synchronized bounded queue to handle incoming client requests concurrently. It supports static file serving, dynamic CGI execution, request statistics, and synchronized logging.

## Features

- Fixed-size worker thread pool
- Bounded request queue
- Mutex and condition-variable synchronization
- Static file handling
- Dynamic CGI request handling
- HTTP GET support
- HTTP POST support for reading the server log
- Per-thread request statistics
- Request arrival and dispatch timing
- Synchronized server log
- Reader-writer locking with writer priority

## Technologies Used

- C
- POSIX threads
- Mutexes
- Condition variables
- Sockets
- HTTP
- CGI
- Linux system programming
- Makefile

## Project Files

- `server.c` - main server loop, thread pool, and request queue handling
- `request.c` / `request.h` - HTTP request parsing and response generation
- `queue.c` / `queue.h` - synchronized job queue support
- `log.c` / `log.h` - server log with reader-writer synchronization
- `segel.c` / `segel.h` - socket and robust I/O helper functions
- `client.c` - simple client for testing
- `output.c` - CGI program used for dynamic request testing
- `home.html` - default static page
- `Makefile` - build configuration

## How to Build

```bash
make
