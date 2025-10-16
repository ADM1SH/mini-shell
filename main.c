/**
 * A mini shell program in C.
 * This program implements a basic shell with support for pipelines, I/O redirection,
 * background processes, command history, aliases, and a startup script.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_ALIASES 50

// --- Data Structures ---

// Structure to hold an alias mapping a name to a command.
struct alias {
    char *name;
    char *command;
};

// Global table to store all defined aliases.
struct alias alias_table[MAX_ALIASES];
int alias_count = 0;

// --- Function Prototypes ---

void process_line(char *line);
void execute_pipeline(char *args[], int background);
void handle_command(char *args[]);

// External global variable that holds the environment variables.
extern char **environ;

/**
 * @brief Executes commands from the .myshellrc file in the user's home directory on startup.
 */
void startup_script() {
    char rc_path[1024];
    char *home = getenv("HOME"); // Get the home directory path.
    if (home) {
        // Construct the full path to .myshellrc.
        snprintf(rc_path, sizeof(rc_path), "%s/.myshellrc", home);
        FILE *fp = fopen(rc_path, "r");
        if (fp) {
            char line[1024];
            // Read the file line by line.
            while (fgets(line, sizeof(line), fp)) {
                // strdup is used because process_line is destructive (uses strtok).
                char *line_copy = strdup(line);
                process_line(line_copy);
                free(line_copy);
            }
            fclose(fp);
        }
    }
}

/**
 * @brief The main entry point and interactive loop for the shell.
 */
int main() {
    char *line;

    // The parent shell process should ignore Ctrl+C (SIGINT).
    // Child processes will reset this to the default behavior.
    signal(SIGINT, SIG_IGN);

    // Run the startup script once.
    startup_script();

    // Main shell loop.
    while (1) {
        // Before issuing a new prompt, check for any background processes that have finished.
        // WNOHANG ensures that waitpid returns immediately if no child has exited (to prevent blocking).
        while (waitpid(-1, NULL, WNOHANG) > 0);

        // Use readline to get input, which provides history and editing capabilities.
        line = readline("myshell> ");
        if (line == NULL) { // readline returns NULL on EOF (Ctrl+D).
            printf("\n");
            break;
        }

        // If the line is not empty, add it to the command history.
        if (line[0]) {
            add_history(line);
        }
        
        // process_line uses strtok, which modifies the string.
        // We pass a copy so the original `line` from readline can be safely freed.
        char *line_copy = strdup(line);
        process_line(line_copy);
        free(line_copy);

        // readline allocates memory for the line, so it must be freed.
        free(line);
    }
    return 0;
}

/**
 * @brief Processes a single line of command input.
 * This function is the main dispatcher, handling parsing, built-ins, and execution.
 * @param line A mutable string containing the command line.
 */
void process_line(char *line) {
    char *args[128];
    // Tokenize the input line into arguments based on spaces.
    char *token = strtok(line, " ");
    int i = 0;
    while (token) { 
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL; // The argument list must be NULL-terminated.

    // If the line was empty or just spaces, do nothing.
    if (!args[0]) return;

    // --- Alias Expansion ---
    // Before executing, check if the command is an alias.
    for (int k = 0; k < alias_count; k++) {
        if (strcmp(args[0], alias_table[k].name) == 0) {
            args[0] = alias_table[k].command; // Replace the command with the alias value.
            break;
        }
    }

    // --- Built-in commands that modify the shell state ---
    // These must be run in the parent process.
    if (strcmp(args[0], "exit") == 0) { exit(0); }
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) { // `cd` with no arguments goes to home directory.
            char* home_dir = getenv("HOME");
            if (home_dir != NULL) chdir(home_dir);
        } else {
            if (chdir(args[1]) != 0) perror("myshell");
        }
        return; // Return to the main loop.
    }
    if (strcmp(args[0], "alias") == 0) {
        if (args[1] != NULL && args[2] != NULL) { // Define a new alias.
            if (alias_count < MAX_ALIASES) {
                alias_table[alias_count].name = strdup(args[1]);
                alias_table[alias_count].command = strdup(args[2]);
                alias_count++;
            }
        } else { // List all aliases.
            for (int k = 0; k < alias_count; k++) {
                printf("alias %s='%s'\n", alias_table[k].name, alias_table[k].command);
            }
        }
        return;
    }
    if (strcmp(args[0], "unalias") == 0) {
        if (args[1] != NULL) {
            for (int k = 0; k < alias_count; k++) {
                if (strcmp(args[1], alias_table[k].name) == 0) {
                    free(alias_table[k].name);
                    free(alias_table[k].command);
                    // Shift remaining aliases down to fill the gap.
                    for (int j = k; j < alias_count - 1; j++) {
                        alias_table[j] = alias_table[j + 1];
                    }
                    alias_count--;
                    break;
                }
            }
        }
        return;
    }
    if (strcmp(args[0], "clear") == 0) {
        printf("\033[H\033[J"); // ANSI escape sequence to clear the screen.
        return;
    }
    if (strcmp(args[0], "help") == 0) {
        printf("--- MyShell Help ---\n");
        printf("Built-in commands:\n");
        printf("  cd [dir]       - Change directory\n");
        printf("  exit           - Exit the shell\n");
        printf("  printenv       - Print environment variables\n");
        printf("  setenv VAR VAL - Set an environment variable\n");
        printf("  unsetenv VAR   - Unset an environment variable\n");
        printf("  alias NAME CMD - Create a simple, single-word alias\n");
        printf("  unalias NAME   - Remove an alias\n");
        printf("  clear          - Clear the terminal screen\n");
        printf("  help           - Display this help message\n");
        printf("  history        - Show command history\n");
        return;
    }
    if (strcmp(args[0], "history") == 0) {
        HISTORY_STATE *myhist = history_get_history_state();
        if (myhist) {
            for (int i = 0; i < myhist->length; i++) {
                HIST_ENTRY *entry = history_get(i + history_base);
                if (entry) {
                    printf(" %d  %s\n", i + history_base, entry->line);
                }
            }
        }
        return;
    }

    // --- External commands and piped built-ins ---
    // Check for the background operator '&' at the end of the command.
    int background = 0;
    if (i > 0 && strcmp(args[i - 1], "&") == 0) {
        background = 1;
        args[i - 1] = NULL; // Remove '&' from arguments.
    }
    // Pass the command to the pipeline executor.
    execute_pipeline(args, background);
}

/**
 * @brief Executes a pipeline of one or more commands.
 * @param args The initial list of arguments from the command line.
 * @param background Whether the pipeline should be run in the background.
 */
void execute_pipeline(char *args[], int background) {
    int num_commands = 0;
    char **commands[10];
    commands[0] = args;
    // Split the arguments into separate commands based on the pipe '|' operator.
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            args[i] = NULL; // Terminate the current command's argument list.
            commands[++num_commands] = &args[i + 1]; // The next command starts after the '|'.
        }
    }
    num_commands++;

    int in_fd = 0; // The input file descriptor for the next command, initially stdin.
    pid_t pids[num_commands];

    // Create a process for each command in the pipeline.
    for (int i = 0; i < num_commands; i++) {
        int pipe_fds[2]; // File descriptors for the pipe: [0] is read, [1] is write.
        
        // Create a new pipe for all but the last command.
        if (i < num_commands - 1) { if (pipe(pipe_fds) == -1) { perror("pipe"); exit(EXIT_FAILURE); } }

        pids[i] = fork();
        if (pids[i] == 0) { // --- Child Process ---
            // If in_fd is not stdin, redirect this process's input to it.
            if (in_fd != 0) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
            // If this is not the last command, redirect its output to the new pipe.
            if (i < num_commands - 1) { dup2(pipe_fds[1], STDOUT_FILENO); close(pipe_fds[0]); close(pipe_fds[1]); }
            // Execute the command.
            handle_command(commands[i]);
        }

        // --- Parent Process ---
        // Close the parent's copy of the file descriptors used by the child.
        if (in_fd != 0) close(in_fd);
        if (i < num_commands - 1) {
            close(pipe_fds[1]); // Parent closes the write end.
            in_fd = pipe_fds[0]; // And saves the read end for the next command.
        }
    }

    // If not a background process, wait for all children to complete.
    if (!background) {
        for (int i = 0; i < num_commands; i++) { waitpid(pids[i], NULL, 0); }
    } else {
        // For a background job, just print the PID of the last command.
        printf("[%d]\n", pids[num_commands - 1]);
    }
}

/**
 * @brief Handles the execution of a single command within a child process.
 * @param args The arguments for the command to execute.
 */
void handle_command(char *args[]) {
    // Reset Ctrl+C behavior to the default for child processes.
    signal(SIGINT, SIG_DFL);

    // --- Variable Expansion ---
    for (int i = 0; args[i] != NULL; i++) {
        if (args[i][0] == '$') {
            char *value = getenv(args[i] + 1);
            args[i] = (value != NULL) ? value : ""; // Replace with value or empty string.
        }
    }

    // --- Piped Built-ins ---
    // Handle built-ins that can run in a child process (and thus in a pipeline).
    if (strcmp(args[0], "printenv") == 0) {
        for (char **env = environ; *env != 0; env++) { printf("%s\n", *env); }
        exit(0); // Exit child process.
    }

    // --- Redirection ---
    // Before execvp, parse for redirection operators and set up file descriptors.
    char *clean_args[64];
    int clean_i = 0;
    for (int j = 0; args[j] != NULL; j++) {
        if (strcmp(args[j], "<") == 0 || strcmp(args[j], ">") == 0 || strcmp(args[j], ">>") == 0) {
            j++; // Skip operator and filename, they are handled in the next loop.
        } else {
            clean_args[clean_i++] = args[j];
        }
    }
    clean_args[clean_i] = NULL;

    for (int j = 0; args[j] != NULL; j++) {
        if (strcmp(args[j], ">") == 0) { int fd = open(args[j + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); if (fd < 0) { perror("open"); exit(1); } dup2(fd, STDOUT_FILENO); close(fd); }
        else if (strcmp(args[j], ">> ") == 0) { int fd = open(args[j + 1], O_WRONLY | O_CREAT | O_APPEND, 0644); if (fd < 0) { perror("open"); exit(1); } dup2(fd, STDOUT_FILENO); close(fd); }
        else if (strcmp(args[j], "<") == 0) { int fd = open(args[j + 1], O_RDONLY); if (fd < 0) { perror("open"); exit(1); } dup2(fd, STDIN_FILENO); close(fd); }
    }

    // --- Execute the command ---
    if (execvp(clean_args[0], clean_args) == -1) {
        perror("myshell");
        exit(EXIT_FAILURE);
    }
}
