/*

Team Name: The Engineers
Team Members: Katie Swinea, Finlay Patoto, John Donnell

Description: Create wish shell to simulate unix shell functionality. 
Includes built-in commands and can access external commands if executable in path. 
Includes interactive mode and batch mode. Includes redirection of command output to file.
Inlcudes parallel commands to be run with & symbol in between commands.

*/

#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LINE_LENGTH 256
#define MAX_TOKENS 100
#define ERROR_MESSAGE "An error has occurred\n"

// Variable for the path and amount of directories in the path
char **paths = NULL;
int path_count = 0;

char* line = NULL;
size_t len = 0;
bool redirect = false;
char* outFile = NULL;

char *tokens[MAX_TOKENS];  // Array to store split parts
int count = 0;

// Free the memory allocated for the path directory array
void free_paths() {

    for (int i = 0; i < path_count; i++) {
        free(paths[i]);
    }

    free(paths);
}

// Function to print error message for ease of use
void print_error() {
    write(STDERR_FILENO, ERROR_MESSAGE, strlen(ERROR_MESSAGE));
}

// Function to initialize the default path (only "/bin").
void init_path() {

    path_count = 1; // Start count at one directory

    // Makes paths a size of one for intilization of default path
    paths = malloc(sizeof(char*) * 1);
    
    if (!paths) { 
        print_error(); 
        exit(1); 
    }

    paths[0] = strdup("/bin"); // Sets path variable
}

// Helper function for the built-in path command to update the path array.
void set_path(char **new_paths, int count) {
    // Free previous paths from intilization or previous setting
    free_paths();

    path_count = count; // Set count for size of array later

    // Sets path to noting if no path is set per assignment instructions
    if (count == 0) {
        paths = NULL;
        return;
    }

    paths = malloc(sizeof(char*) * count); // Sets path array size for number of directories in new path

    // Works similarly to the intilization of path function from here, each path variable is set in array instead of just one
    if (!paths) { 
        print_error(); 
        exit(1); 
    }

    for (int i = 0; i < count; i++) {
        paths[i] = strdup(new_paths[i]);
    }
}

// Given a command name, search through the paths to find an executable if it exists
char* find_command(char *cmd) {

    for (int i = 0; i < path_count; i++) {
        char full_path[MAX_LINE_LENGTH];

        snprintf(full_path, sizeof(full_path), "%s/%s", paths[i], cmd);
        
        // If command exists in the path, command is executed and the full path to do so is returned
        if (access(full_path, X_OK) == 0) {
            return strdup(full_path);
        }
    }

    return NULL;
}

// Function used to tokenize a single command string into an array of arguments
int tokenize_command(char *strCommands, char **args) {
    int count = 0;

    char *token = strtok(strCommands, " \t\n\r"); // Processes command to get important information
    
    // Searches for the null terminator in the tokens from the command
    while (token != NULL && count < MAX_TOKENS - 1) {
        args[count++] = token; // Adds to the args array for the command
        token = strtok(NULL, " \t\n\r"); // Used for loop conditions
    }
    
    args[count] = NULL; // Ends the args array with null terminator
    return count;
}

// Function used to check for redirection symbol
int check_redirection(char* tokens[], bool* redirect) {

    int i = 0;

    while(tokens[i] != NULL) {

        if((strcmp(tokens[i], ">") == 0) && (i != 0)) { // If redirection symbol at 0, no command to write output to file
            *redirect = true;
            return i;
        }

        i++; // Increment through all tokens for full command to search for redirection
    }

    *redirect = false;

    return -1; // If symbl is not in command, -1 since 0 is a valid array element
}

void process_command(char *command) {
    char* args[MAX_TOKENS];
    int redirectLocation = -1;
    char *outFile = NULL;
    bool redirect = false;

    int argCount = tokenize_command(command, args);

    if (argCount == 0) {
        free(command);
        return;
    }

    redirectLocation = check_redirection(args, &redirect);

    if (redirectLocation != -1) {
        if (argCount > redirectLocation + 2) {
            print_error();
            free(command);
            return;
        }
        outFile = args[redirectLocation + 1];
        args[redirectLocation] = NULL;
    }

    if (strcmp(args[0], "exit") == 0) {
        if (argCount != 1) {
            print_error();
        } 
        else {
            free(command);
            exit(0);
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (argCount != 2) {
            print_error();
        } else {
            if (chdir(args[1]) != 0) {
                print_error();
            }
        }
    } 
    else if (strcmp(args[0], "path") == 0) {
        if (argCount == 1) {
            set_path(NULL, 0);
        } 
        else {
            set_path(&args[1], argCount - 1);
        }
    } 
    else {
        int pid = fork();
        if (pid < 0) {
            print_error();
        } 
        else if (pid == 0) {
            if (redirect) {
                int fileID = creat(outFile, 0644);
                if (fileID < 0) {
                    print_error();
                    exit(1);
                }
                dup2(fileID, STDOUT_FILENO);
                dup2(fileID, STDERR_FILENO);
                close(fileID);
            }

            char *executable = find_command(args[0]);
            if (executable == NULL) {
                print_error();
                exit(1);
            }

            execv(executable, args);
            print_error();
            free(executable); // Frees allocated memory of executable in case of execv error
            exit(1);
        } 
        else {
            int status;
            waitpid(pid, &status, 0);
        }
    }

    free(command); // Frees allocate memory for command, will free each tokens element
}


int main(int argc, char *argv[]) {
    FILE *input = stdin;
    int interactiveMode = 1; // Default to interactive mode

    // The shell should be invoked with no arguments or 1 argument (for batch mode)
    if (argc > 2) {
        print_error();
        exit(1);
    } 
    
    // Opens file if another argument is entered to start batch mode
    else if (argc == 2) {
        input = fopen(argv[1], "r");
        
        if (!input) {
            print_error();
            exit(1);
        }

        interactiveMode = 0;
    }

    // Initialize the default path (/bin)
    init_path();

    while (1) {

        if (interactiveMode) {
            printf("wish> ");
            fflush(stdout);
        }

        ssize_t read_bytes = getline(&line, &len, input); // Gets input using getline per assignment instructions
        
        if (read_bytes == -1) {  // EOF or error, stop the loop.
            break;
        }
        // Remove trailing newline if present
        if (line[read_bytes - 1] == '\n') {
            line[read_bytes - 1] = '\0';
        }

        char *trimmed = line;

        // Trim whitespace
        while (*trimmed && isspace(*trimmed)) {
            trimmed++;
        }

        // If the line is only empty skip it
        if (*trimmed == '\0') {
            continue;
        }

        count = 0;

         // Splitting the string
        char *token = strtok(trimmed, "&");
        while (token != NULL && count < MAX_TOKENS) {
            while (*token == ' ') token++;  // Trim leading spaces
            tokens[count++] = strdup(token);
            token = strtok(NULL, "&");
        }


        // If no parallel commands, run singular command 
        if (count == 1) {
            process_command(tokens[0]);
        }

        // Otherwise fork each comand for the parallel commands
        else {

            pid_t pids[MAX_TOKENS]; // Keep track of all the children running

            // Fork each command and run command in child
            for (int i = 0; i < count; i++) {
                pids[i] = fork();
                
                if (pids[i] < 0) {
                    print_error();
                }

                else if (pids[i] == 0) {
                    process_command(tokens[i]);
                    exit(0);
                }
            }

            // Waits for all children to end
            for (int i = 0; i < count; i++) {
                waitpid(pids[i], NULL, 0);
            }
        }
    }

    // No memory leaks
    free(line);
    free_paths();

    // Close batch file opened in read mode
    if (!interactiveMode) {
        fclose(input);
    }

    exit(0);
}
