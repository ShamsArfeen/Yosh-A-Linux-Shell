# Yosh - A Linux Shell

## Pre-requisites
* gcc
* libreadline-dev
* make

## How to Invoke Yosh shell
* Create yosh executable using make utility
``make``
* Alternately, compile code using
``gcc yosh.c parse.c main.c -o yosh -lreadline``

## Features
* Multiple pipes support in single cmd, example ``/path/to/directory> ls | wc | wc``
* I/O redirection, example ``/path/to/directory> wc < HELP.txt > NEW.txt``
* Background execution of cmd using ampersand (&) in the end, example ``/path/to/directory> sort < words.txt > sorted_words.txt &``

### Builtin Commands
* jobs - Provides a list of all background processes and their local IDs.
* cd [PATHNAME] - Sets the PATHNAME as working directory.
* history - Prints the list of previously executed commands.
* kill %[NUM] - Terminates the background process identified with NUM in the jobs list. This is equivalent to ``kill [PID]`` without percent (%) sign where PID is the actual process ID.
* help - Prints help about builtin commands
