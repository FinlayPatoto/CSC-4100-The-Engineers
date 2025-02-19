/*
This doesn't do much right now since I was lazy.
Implemented some of the built in commands and basic stuff like that.
Seem to work, kind of hard to tell without ls working though since cd hard (impossible I think) to see when command line is just wish> 
Included comments about other important stuff that needs to be done.
*/


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LINE_LENGTH 256
#define MAX_TOKENS 100
#define ERROR_MESSAGE "An error has occurred\n"

// Function to print error message for ease of use
void print_error() {
    write(STDERR_FILENO, ERROR_MESSAGE, strlen(ERROR_MESSAGE));
}

// Function used to tokenize a single command string into an array of arguments without spacing problems for the wish commands
int tokenize_command(char *strCommands, char **args) {
    int count = 0;

    // Makes token of command for processing
    char *token = strtok(strCommands, " \t\n\r");

    // Adds to the argument counter, looking for null terminator
    while (token != NULL && count < MAX_TOKENS - 1) {
        args[count++] = token; // Called args counte because each command/argument is its own token
        token = strtok(NULL, " \t\n\r"); // Repeat for while loop
    }

    args[count] = NULL; // Null terminate the argument array
    return count;
}

int main(int argc, char *argv[]) {
    FILE *input = stdin;

    int interactiveMode = 1; // Set default to interactive and will be changed if file entered for batch mode

    // The shell should be invoked with no arguments or 1 argument for file in batch mode.
    if (argc > 2) {
        print_error();
        exit(1);
    } 

    // Opens file if it is batch mode, assumes executable was called correctly and file entered is readable
    else if (argc == 2) {
        input = fopen(argv[1], "r");

        // If it can't open the file, print error
        if (!input) {
            print_error();
            exit(1);
        }

        interactiveMode = 0; // Sets mode to batch since a readable file was input into the program
    }

    // Character array and length for getline functions
    char *line = NULL;
    size_t len = 0;

    while (1) {
    	// Print the expected cli structure if in interactive mode
        if (interactiveMode) {
            printf("wish> ");
            fflush(stdout);
        }

        ssize_t read = getline(&line, &len, input);
        if (read == -1) {  // EOF or error, stop the loop
            break;
        }

        // Command from command line
        char *command = strdup(line);

        if (!command) { 
        	print_error(); 
        	continue; 
        }

        char *args[MAX_TOKENS];

        // Get the tokens and agrument count for use in built in command checks
        int argCount = tokenize_command(command, args);

        // No memory leaks
        if (argCount == 0) {
            free(command);
            continue;
        }



        // Built-in commands (only thing I've written so far for commands, I didn't want to deal with paths yet so no other commands or the path command)
        if (strcmp(args[0], "exit") == 0) {

        	// Exit should be only argument
        	if (argCount != 1) {
            	print_error();
            } 

            else {
            	exit(0);
            }
                
            continue;
        } 

        else if (strcmp(args[0], "cd") == 0) {

        	// Should only be cd and the directory
            if (argCount != 2) {
            	print_error();
            } 

            else {
            	if (chdir(args[1]) != 0) {
                	print_error();
                }
            }

            continue;
        }

        // Other commands will use fork and do the command in the child

        // Need to check for & for parallel commands, need some kind of parsing

        // Keep track of child pids for parallel commands

        free(command);

    }

    free(line);

    if (!interactiveMode) {
        fclose(input);
   	}

    exit(0);
}