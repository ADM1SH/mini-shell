#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> // For open() flags

int main() {
    char input[1024];
    char *args[64];

    while (1) {
        printf("myshell> ");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break; // EOF (Ctrl+D)
        }
        input[strcspn(input, "\n")] = 0;

        char *token = strtok(input, " ");
        int i = 0;
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (args[0] == NULL) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "myshell: expected argument to \"cd\"\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("myshell");
                }
            }
            continue;
        }

        // --- Handle Redirection ---
        char *inputFile = NULL;
        char *outputFile = NULL;
        int append_mode = 0; // 0 for >, 1 for >>
        char *clean_args[64];
        int clean_i = 0;
        int parse_error = 0;

        for (int j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], "<") == 0) {
                if (args[j + 1] != NULL) {
                    inputFile = args[j + 1];
                    j++; // Skip the filename in the next iteration
                } else {
                    fprintf(stderr, "myshell: syntax error near unexpected token `newline'\n");
                    parse_error = 1;
                    break;
                }
            } else if (strcmp(args[j], ">") == 0) {
                if (args[j + 1] != NULL) {
                    outputFile = args[j + 1];
                    append_mode = 0;
                    j++; // Skip the filename
                } else {
                    fprintf(stderr, "myshell: syntax error near unexpected token `newline'\n");
                    parse_error = 1;
                    break;
                }
            } else if (strcmp(args[j], ">>") == 0) {
                if (args[j + 1] != NULL) {
                    outputFile = args[j + 1];
                    append_mode = 1;
                    j++; // Skip the filename
                } else {
                    fprintf(stderr, "myshell: syntax error near unexpected token `newline'\n");
                    parse_error = 1;
                    break;
                }
            } else {
                clean_args[clean_i++] = args[j];
            }
        }
        clean_args[clean_i] = NULL;

        if (parse_error) {
            continue; // Skip execution if there was a parsing error
        }

        pid_t pid = fork();
        if (pid == 0) {
            // --- Child Process ---

            // Handle input redirection
            if (inputFile != NULL) {
                int fd_in = open(inputFile, O_RDONLY);
                if (fd_in == -1) {
                    perror("myshell");
                    exit(EXIT_FAILURE);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }

            // Handle output redirection
            if (outputFile != NULL) {
                int flags = O_WRONLY | O_CREAT;
                if (append_mode) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }
                int fd_out = open(outputFile, flags, 0644);
                if (fd_out == -1) {
                    perror("myshell");
                    exit(EXIT_FAILURE);
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }

            if (execvp(clean_args[0], clean_args) == -1) {
                perror("myshell");
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            perror("myshell");
        } else {
            // Parent process
            waitpid(pid, NULL, 0);
        }
    }
    return 0;
}
