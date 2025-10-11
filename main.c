#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char input[1024];
    char *args[64];

    while (1) {
        printf("myshell> ");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break; // EOF (Ctrl+D)
        }
        input[strcspn(input, "\n")] = 0; // Remove newline

        // --- Parsing the command ---
        char *token = strtok(input, " ");
        int i = 0;
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL; // Null-terminate the argument list

        // --- Command Execution ---
        if (args[0] == NULL) {
            // Empty command, just show the prompt again
            continue;
        }

        // Handle 'exit' built-in
        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        // Handle 'cd' built-in
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "myshell: expected argument to \"cd\"\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("myshell");
                }
            }
            continue; // Skip fork/exec for 'cd'
        }

        // --- Fork and Exec for external commands ---
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1) { // execvp only returns on error
                perror("myshell"); 
            }
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // Error forking
            perror("myshell");
        } else {
            // Parent process
            waitpid(pid, NULL, 0);
        }
    }
    return 0;
}