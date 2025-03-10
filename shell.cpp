#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <signal.h>

#define MAX_LINE 1024
#define MAX_ARGS 100

using namespace std;

void executeSingleCommand(char* args[]);
void executePipedCommand(char* commands[], int n);
void parseInput(char* input);
void printDir();
void displayHelp();
void handleSleep(char* args[]);

int main() {
    char input[MAX_LINE];

    // Add introductory message
    cout << "Custom Shell\nType 'help' for a list of commands or 'exit' to quit\n";
    
    while (true) {
        // Print the current directory
        printDir();
        cout << " > ";
        fflush(stdout);

        if (!fgets(input, MAX_LINE, stdin)) {
            break; // EOF
        }

        // Remove trailing newline from input
        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        // Handle "exit" command
        if (strcmp(input, "exit") == 0) {
            cout << "Exiting shell...\n";
            break;
        }

        parseInput(input); // Parse user input
    }
    return 0;
}

// Implement input parsing with pipe support
void parseInput(char* input) {
    char* commands[MAX_ARGS];
    char* token = strtok(input, "|");
    int count = 0;

    // Split input into commands based on the pipe '|'
    while (token != NULL) {
        commands[count++] = token;
        token = strtok(NULL, "|");
    }

    // Handle single commands separately
    if (count == 1) {
        char* args[MAX_ARGS];
        int i = 0;
        token = strtok(commands[0], " \n");

        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " \n");
        }
        args[i] = NULL;

        // Add support for "help", "cd", and "sleep" commands
        if (strcmp(args[0], "help") == 0) {
            displayHelp();
        }
        else if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                cerr << "cd: missing argument\n";
            } else if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
        else if (strcmp(args[0], "sleep") == 0) {
            handleSleep(args);
        }
        else {
            executeSingleCommand(args);
        }
    } else {
        executePipedCommand(commands, count); // Add piped command execution
    }
}

// Execute single command
void executeSingleCommand(char* args[]) {
    pid_t pid = fork();
    if (pid == 0) { // Child process
        if (execvp(args[0], args) == -1) {
            perror("Error executing command");
        }
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent process
        wait(NULL);
    } else {
        perror("Fork failed");
    }
}

// Execute piped command
void executePipedCommand(char* commands[], int n) {
    int pipefd[2];
    int input = 0;

    for (int i = 0; i < n; i++) {
        pipe(pipefd);
        pid_t pid = fork();

        if (pid == 0) { // Child process
            dup2(input, STDIN_FILENO);
            if (i != n - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
            }
            close(pipefd[0]);

            char* args[MAX_ARGS];
            char* token = strtok(commands[i], " \n");
            int j = 0;
            while (token != NULL) {
                args[j++] = token;
                token = strtok(NULL, " \n");
            }
            args[j] = NULL;

            if (execvp(args[0], args) == -1) {
                perror("Error executing command");
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) { // Parent process
            wait(NULL);
            close(pipefd[1]);
            input = pipefd[0];
        } else {
            perror("Fork failed");
        }
    }
}

// Print the current directory
void printDir() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        cout << cwd;
    } else {
        perror("getcwd error");
    }
}

// Display help menu
void displayHelp() {
    cout << "List of Commands:\n";
    cout << "cd <directory>    - Change the current directory to <directory>\n";
    cout << "help              - Display this help menu\n";
    cout << "exit              - Exit the shell\n";
    cout << "sleep <seconds>   - Pause for the specified number of seconds\n";
    cout << "<command>         - Execute the specified command\n";
    cout << "<command1> | <command2> - Execute command1 and pass its output to command2\n";
}

// Handle the sleep command
void handleSleep(char* args[]) {
    if (args[1] == NULL) {
        cerr << "sleep: missing argument\n";
    } else {
        int seconds = atoi(args[1]);
        sleep(seconds);
    }
}
