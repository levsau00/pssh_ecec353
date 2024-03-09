Project 1
Lev Saunders
14345595

How To Compile & Run
====================
in source directory:
$ make
$ ./pssh


Description
===========
parse.c
    - contains functions for parsing of command line input into 
      comannds, arguments, and shell operators
parse.h
    - header file for parse.c, including function and struct 
      declarations
builtin.c
    - contains functions for recognition and execution of shell
      builtin commands
builtin.h
    - header file for builtin.h, containing function declarations
pssh.c
    - compiles to main executable. Contains logic for running 
      commands including process creation and managment, signal 
      handling, input and output redirection and command 
      pipelining using pipes.