# Linux Syscall Ban Mechanism

## Overview

This project modifies selected parts of the Linux kernel to implement a syscall-ban mechanism.

The system allows syscall restrictions to be applied to processes and process branches. It adds custom kernel logic for storing ban information, checking whether a process is allowed to execute specific system calls, and modifying selected syscall behavior based on those restrictions.

This project was created as part of an Operating Systems course.

## Important Note

This folder is included for code review and portfolio purposes.

It is **not** a standalone runnable project. The files in this folder are selected modified Linux kernel files from an academic kernel-programming assignment. Running the project would require applying these changes to the matching Linux kernel source tree and building that kernel in the required course environment.

## Features

* Adds a syscall-ban mechanism to the Linux kernel
* Tracks syscall restrictions per process
* Supports checking whether a syscall is banned
* Supports updating syscall-ban behavior across a process branch
* Extends process/task metadata with ban-related fields
* Adds custom syscall declarations
* Adds custom entries to the x86-64 syscall table
* Modifies selected kernel behavior related to process/system-call handling

## Main Kernel Changes

The project includes changes related to:

* syscall-ban storage and lookup
* process metadata
* syscall table registration
* process-related system calls
* signal-related behavior
* pipe-related behavior

## Project Structure

```text
hw02-linux-syscall-ban/
├── kernel/
│   ├── hw2.c
│   ├── sys.c
│   └── signal.c
├── fs/
│   └── pipe.c
├── include/
│   └── linux/
│       ├── sched.h
│       ├── init_task.h
│       └── syscalls.h
└── arch/
    └── x86/
        └── entry/
            └── syscalls/
                └── syscall_64.tbl
```

## Project Files

* `kernel/hw2.c` - custom syscall-ban implementation
* `kernel/sys.c` - modified system-call behavior
* `kernel/signal.c` - modified signal-related behavior
* `fs/pipe.c` - modified pipe-related behavior
* `include/linux/sched.h` - task structure changes
* `include/linux/init_task.h` - initial task metadata changes
* `include/linux/syscalls.h` - custom syscall declarations
* `arch/x86/entry/syscalls/syscall_64.tbl` - syscall table entries

## Technologies Used

* C
* Linux Kernel Programming
* Linux System Calls
* Process Management
* Kernel Data Structures
* x86-64 Linux syscall table
* Operating Systems

## What I Learned

* How Linux system calls are declared and registered
* How process-specific metadata is stored inside the kernel
* How process hierarchy can affect kernel behavior
* How syscall restrictions can be enforced at the kernel level
* How kernel changes often require coordinated edits across multiple subsystems
* Why kernel programming is powerful, fragile, and extremely unforgiving
