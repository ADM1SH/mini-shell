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
void process_line(char *line);
void execute_pipeline(char *args[], int background);
void handle_command(char *args[]);

// For accessing environment variables
extern char **environ;

void startup_script() {
    char rc_path[1024];
    char *home = getenv("HOME");
    if (home) {
        snprintf(rc_path, sizeof(rc_path), "%s/.myshellrc", home);
        FILE *fp = fopen(rc_path, "r");
        if (fp) {
            char line[1024];
            while (fgets(line, sizeof(line), fp)) {
                // Create a mutable copy for process_line, which uses strtok
                char *line_copy = strdup(line);
                process_line(line_copy);
                free(line_copy);
            }
            fclose(fp);
        }
    }
}

int main() {
    char *line;
    signal(SIGINT, SIG_IGN);

    startup_script();

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

        process_line(line);
        free(line);
    }
    return 0;
}

void process_line(char *line) {
    char *args[128];
    char *token = strtok(line, " ");
    int i = 0;
    while (token) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (!args[0]) {
        return; 
    }

    int background = 0;
    if (i > 0 && strcmp(args[i - 1], "&") == 0) {
        background = 1;
        args[i - 1] = NULL;
    }

    if (strcmp(args[0], "exit") == 0) {
        // In startup script, exit just stops the script, not the shell.
        // In interactive mode, the main loop will break.
        return;
    }
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            char* home_dir = getenv("HOME");
            if (home_dir != NULL && chdir(home_dir) != 0) {
                perror("myshell");
            }
        } else {
            if (args[1][0] == '$') {
                char* value = getenv(args[1] + 1);
                if (value != NULL && chdir(value) != 0) {
                    perror("myshell");
                }
            } else if (chdir(args[1]) != 0) {
                perror("myshell");
            }
        }
        return;
    }
    if (strcmp(args[0], "setenv") == 0) {
        if (args[1] != NULL && args[2] != NULL) {
            if (setenv(args[1], args[2], 1) != 0) {
                perror("myshell: setenv");
            }
        } else {
            fprintf(stderr, "myshell: usage: setenv VAR VALUE\n");
        }
        return;
    }
    if (strcmp(args[0], "unsetenv") == 0) {
        if (args[1] != NULL) {
            if (unsetenv(args[1]) != 0) {
                perror("myshell: unsetenv");
            }
        } else {
            fprintf(stderr, "myshell: usage: unsetenv VAR\n");
        }
        return;
    }
    
    execute_pipeline(args, background);
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
            if (pipe(pipe_fds) == -1) { perror("pipe"); exit(EXIT_FAILURE); }
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

        if (in_fd != 0) close(in_fd);
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

    for (int i = 0; args[i] != NULL; i++) {
        if (args[i][0] == '$') {
            char *value = getenv(args[i] + 1);
            args[i] = (value != NULL) ? value : "";
        }
    }

    if (strcmp(args[0], "printenv") == 0) {
        for (char **env = environ; *env != 0; env++) {
            printf("%s\n", *env);
        }
        exit(0);
    }

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
