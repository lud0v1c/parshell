# parshell #

Simple multiplexer written in C for the Operating Systems 2015/16 course project at Instituto Superior Técnico. Its purpose was to explore UNIX's handling of semaphores and other concurrent mechanisms. ```par-shell``` logs, executes and destroys subprocesses of ```par-shell-terminals``` that connect to it via a temporary pipe.

## Build ##

A makefile is provided. This was built and tested on Ubuntu 16.04, but should work on any modern UNIX machine. A C compiler and ```make``` is all you need to build this project.

## Usage ##

```./par-shell path-to-temp-pipe```
Starts the main component. The default makefile uses ```/tmp/par-shell-in``` as the default pipe path.<br/>
```./par-shell-terminal path-to-temp-pipe```
Opens a shell connected via a standard pipe to the main process, after which, is possible to execute commands (the full path needs to be provided). Each sub shell's process output goes to a .txt file with the corresponding PID file name.<br/>

The terminal also accepts the ```stats``` command, which shows the number of active processes, and ```exit``` or ```exit-global``` which kills the local shell or all shells, respectively.

