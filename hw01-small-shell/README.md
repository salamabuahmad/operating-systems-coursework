# Small Shell

## Overview

This project implements a small Unix-like shell in C++.

The shell supports built-in commands, external command execution, background jobs, foreground jobs, signal handling, redirection, pipes, aliases, and process management.

## Features

- Execute external Linux commands
- Run commands in the foreground or background
- Track background jobs
- Bring jobs to the foreground
- Handle signals such as `Ctrl+C` and `Ctrl+Z`
- Support command aliases
- Support output redirection
- Support pipes
- Built-in shell commands such as:
  - `pwd`
  - `cd`
  - `jobs`
  - `fg`
  - `kill`
  - `quit`
  - `alias`
  - `unalias`
  - `unsetenv`
  - `sysinfo`
  - `du`
  - `whoami`

## Technologies Used

- C++
- Linux system calls
- Process management
- Signals
- Pipes
- Redirection
- Makefile

## What I Learned

- How shells parse and execute commands
- How Linux processes are created and managed
- How foreground and background jobs work
- How to handle Unix signals
- How pipes and output redirection are implemented
