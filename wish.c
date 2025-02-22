/*
Added path command, implemented initialized path, let commands that aren't built-in be used
So basic built-in commands and commands in /bin (or any other directory I guess since path command works) are implemented
Need to basically do the parsing for finding specific command things like redirection and parallel commands (aka the hard part... yay)
Included comments about other important stuff that needs to be done (mostly all of what is mentioned above).
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

// Global variables for path array and count.
char **paths = NULL;
int path_count = 0;

// Free the memory allocated for the path directory array.
void free_paths() {
    for (int i = 0; i < path_count; i++) {
        free(paths[i]);
    }
    free(paths);
}

// Function to print error message for ease of use.
void print_error() {
    write(STDERR_FILENO, ERROR_MESSAGE, strlen(ERROR_MESSAGE));
}

// Function to initialize the default path (only "/bin").
void init_path() {
    path_count = 1; // Start count at one directory.
    paths = malloc(sizeof(char*) * 1);
    if (!paths) { 
        print_error(); 
        exit(1); 
    }
    paths[0] = strdup("/bin");
}

// Helper function for the built-in path command to update the path array.
void set_path(char **new_paths, int count) {
    free_paths();
    path_count = count;
    if (count == 0) {
        paths = NULL;
        return;
    }
    paths = malloc(sizeof(char*) * count);
    if (!paths) { 
        print_error(); 
        exit(1); 
    }
    for (int i = 0; i < count; i++) {
        paths[i] = strdup(new_paths[i]);
    }
}

// Given a command name, search through the paths to find an executable.
char *find_executable(char *cmd) {
    for (int i = 0; i < path_count; i++) {
        char full_path[MAX_LINE_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", paths[i], cmd);
        if (access(full_path, X_OK) == 0) {
            return strdup(full_path);
        }
    }
    return NULL;
}

// Function used to tokenize a single command string into an array of arguments.
int tokenize_command(char *strCommands, char **args) {
    int count = 0;
    char *token = strtok(strCommands, " \t\n\r");
    while (token != NULL && count < MAX_TOKENS - 1) {
        args[count++] = token;
        token = strtok(NULL, " \t\n\r");
    }
    args[count] = NULL;
    return count;
}

int main(int argc, char *argv[]) {
    FILE *input = stdin;
    int interactiveMode = 1; // Default to interactive mode.

    // The shell should be invoked with no arguments or 1 argument (for batch mode).
    if (argc > 2) {
        print_error();
        exit(1);
    } 
    
    else if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            print_error();
            exit(1);
        }
        interactiveMode = 0;
    }

    // Initialize the default path (/bin).
    init_path();

    char *line = NULL;
    size_t len = 0;

    while (1) {
        if (interactiveMode) {
            printf("wish> ");
            fflush(stdout);
        }
        ssize_t read_bytes = getline(&line, &len, input);
        if (read_bytes == -1) {  // EOF or error, stop the loop.
            break;
        }
        // Remove trailing newline if present.
        if (line[read_bytes - 1] == '\n') {
            line[read_bytes - 1] = '\0';
        }
        // Skip empty lines.
        char *trimmed = line;
        while (*trimmed && isspace(*trimmed))
            trimmed++;
        if (*trimmed == '\0') {
            continue;
        }
        // Duplicate the line for tokenization (since strtok modifies the string).
        char *command = strdup(trimmed);
        if (!command) {
            print_error();
            continue;
        }
        char *args[MAX_TOKENS];
        int argCount = tokenize_command(command, args);
        if (argCount == 0) {
            free(command);
            continue;
        }

        // Handle built-in commands.
        if (strcmp(args[0], "exit") == 0) {
            if (argCount != 1) {
                print_error();
            } 
            else {
                free(command);
                break;  // Exit the shell.
            }
        } 
        else if (strcmp(args[0], "cd") == 0) {
            if (argCount != 2) {
                print_error();
            } 
            else {
                if (chdir(args[1]) != 0) {
                    print_error();
                }
            }
        } else if (strcmp(args[0], "path") == 0) {
            if (argCount == 1) {
                set_path(NULL, 0);
            } 
            else {
                set_path(&args[1], argCount - 1);
            }
        } 
        
        else {
            // External command: fork and execute.
            int pid = fork();
            if (pid < 0) {
                print_error();
            } 
            
            else if (pid == 0) {
                // Child process.
                char *executable = find_executable(args[0]);
                if (executable == NULL) {
                    print_error();
                    exit(1);
                }
                execv(executable, args);
                print_error();
                exit(1);
            } 
            
            else {
                // Parent process: wait for the child to finish.
                wait(NULL);
            }
        }

        // Need to check for & for parallel commands, need some kind of parsing

        // Keep track of child pids for parallel commands

        // Need something with the parsing the check for the > for redirection

        free(command);
    }

    free(line);
    free_paths();
    if (!interactiveMode) {
        fclose(input);
    }
    exit(0);
}
