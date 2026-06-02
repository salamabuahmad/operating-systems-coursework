# Operating Systems Coursework

This repository contains selected projects from an Operating Systems course.

The projects focus on Linux system programming, process management, signal handling, kernel modification, multithreading, synchronization, web-server design, and custom memory allocation.

## Projects

### HW01 - Small Shell

A small Unix-like shell implemented in C++.

The shell supports built-in commands, external command execution, foreground and background jobs, signal handling, pipes, output redirection, aliases, and process management.

### HW02 - Linux Syscall Ban Mechanism

A Linux kernel modification project that adds syscall-ban logic for processes.

The project includes selected modified Linux kernel files that implement custom syscall restrictions, process metadata changes, syscall table updates, and modified behavior for selected kernel operations.

This project is included for code review and portfolio purposes. It is not a standalone runnable project.

### HW03 - Multithreaded Web Server

A multithreaded HTTP web server implemented in C.

The server uses a fixed-size worker thread pool, a synchronized bounded queue, mutexes, condition variables, static file serving, dynamic CGI execution, request statistics, and synchronized logging.

### HW04 - Custom Memory Allocator

A custom dynamic memory allocator implemented in C++.

The allocator implements `smalloc`, `scalloc`, `sfree`, and `srealloc`, using metadata tracking, free lists, buddy-style allocation, block splitting, block merging, `sbrk`, and `mmap`.

## Repository Structure

```text
operating-systems-coursework/
├── hw01-small-shell/
├── hw02-linux-syscall-ban/
├── hw03-multithreaded-web-server/
└── hw04-custom-memory-allocator/
```

## Technologies Used

* C
* C++
* Linux Kernel Programming
* Linux System Calls
* POSIX threads
* Mutexes
* Condition variables
* Signals
* Pipes
* Process management
* Sockets
* HTTP
* Custom memory management
* Makefile

## Notes

These projects were created as part of academic coursework and later organized for portfolio presentation.

Submission ZIPs, generated binaries, test artifacts, full kernel source trees, and IDE-specific files are intentionally excluded.
