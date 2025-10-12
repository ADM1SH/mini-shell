#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

// Function prototypes
void execute_pipeline(char *args[], int background);
void handle_command(char *args[]);

// For accessing environment variables
extern char **environ;

int main() {
    char *line;
    signal(SIGINT, SIG_IGN);

    while (1) {
        while (waitpid(-1, NULL, WNOHANG) > 0);

        line = readline("myshell> ");
        if (line == NULL) {
            printf("\n");
            break;
        }

        if (line[0]) {
            add_history(line);
        }

        char *args[128];
        char *token = strtok(line, " ");
        int i = 0;
        while (token) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (!args[0]) {
            free(line);
            continue; 
        }

        int background = 0;
        if (i > 0 && strcmp(args[i - 1], "&") == 0) {
            background = 1;
            args[i - 1] = NULL;
        }

        // --- Handle Built-in Commands that affect the parent ---
        if (strcmp(args[0], "exit") == 0) {
            free(line);
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
            free(line);
            continue;
        }
        if (strcmp(args[0], "setenv") == 0) {
            if (args[1] == NULL || args[2] == NULL) {
                fprintf(stderr, "myshell: usage: setenv VAR VALUE\n");
            } else {
                if (setenv(args[1], args[2], 1) != 0) {
                    perror("myshell: setenv");
                }
            }
            free(line);
            continue;
        }
        if (strcmp(args[0], "unsetenv") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "myshell: usage: unsetenv VAR\n");
            } else {
                if (unsetenv(args[1]) != 0) {
                    perror("myshell: unsetenv");
                }
            }
            free(line);
            continue;
        }
        
        execute_pipeline(args, background);
        free(line);
    }
    return 0;
}

void execute_pipeline(char *args[], int background) {
    int num_commands = 0;
    char **commands[10];
    commands[0] = args;

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            args[i] = NULL;
            commands[++num_commands] = &args[i + 1];
        }
    }
    num_commands++;

    int in_fd = 0;
    pid_t pids[num_commands];

    for (int i = 0; i < num_commands; i++) {
        int pipe_fds[2];
        if (i < num_commands - 1) {
            if (pipe(pipe_fds) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pids[i] = fork();
        if (pids[i] == 0) { // Child process
            if (in_fd != 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < num_commands - 1) {
                dup2(pipe_fds[1], STDOUT_FILENO);
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }
            handle_command(commands[i]);
        }

        if (in_fd != 0) {
            close(in_fd);
        }
        if (i < num_commands - 1) {
            close(pipe_fds[1]);
            in_fd = pipe_fds[0];
        }
    }

    if (!background) {
        for (int i = 0; i < num_commands; i++) {
            waitpid(pids[i], NULL, 0);
        }
    } else {
        printf("[%d]\n", pids[num_commands - 1]);
    }
}

void handle_command(char *args[]) {
    signal(SIGINT, SIG_DFL);

    // Handle built-ins that can run in a child process.
    if (strcmp(args[0], "printenv") == 0) {
        for (char **env = environ; *env != 0; env++) {
            printf("%s\n", *env);
        }
        exit(0);
    }

    // If not a built-in, handle redirection and execution.
    char *clean_args[64];
    int clean_i = 0;

    for (int j = 0; args[j] != NULL; j++) {
        if (strcmp(args[j], "<") == 0 || strcmp(args[j], ">") == 0 || strcmp(args[j], ">>") == 0) {
            j++;
        } else {
            clean_args[clean_i++] = args[j];
        }
    }
    clean_args[clean_i] = NULL;

    for (int j = 0; args[j] != NULL; j++) {
        if (strcmp(args[j], ">") == 0) {
            int fd = open(args[j + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else if (strcmp(args[j], ">> ") == 0) {
            int fd = open(args[j + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else if (strcmp(args[j], "<") == 0) {
            int fd = open(args[j + 1], O_RDONLY);
            if (fd < 0) { perror("open"); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
    }

    if (execvp(clean_args[0], clean_args) == -1) {
        perror("myshell");
        exit(EXIT_FAILURE);
    }
}
