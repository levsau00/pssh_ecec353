# Project 2
## ECEC 353 Systems Programming
## Lev Saunders ***REMOVED***

### How To Compile & Run
====================  
in source directory:
```bash
$ make
$ ./pssh
```
### Quirks
======  
1. When pssh reports a job's status _(stopped, continued, done, etc.)_, sometimes the prompt will not automatically reappear. __THE SHELL STILL HAS CONTROL OF THE TERMINAL__. You are still able to type and run commands, and as soon as you hit enter the prompt will reappear.

### Description
===========  
#### parse.c
  - contains functions for parsing of command line input into comannds, arguments, and shell operators
#### parse.h
  - header file for parse.c, including function and struct declarations
#### builtin.c
  - contains functions for recognition and execution of shell builtin commands
#### builtin.h
  - header file for builtin.c containing function declarations
#### jobs.c
  - contains functions for creation and managment of jobs and process groups
#### jobs.h
  - header file for jobs.h, containing Job struct and Jobstatus enum definitions and function declarations
#### pssh.c
  - compiles to main executable. Contains logic for running commands including process creation and managment, signal handling, input and output redirection and command pipelining using pipes.