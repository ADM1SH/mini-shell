# My Mini Shell Project (in C)

I built this mini shell in C because I wanted a project that would force me to understand how Linux and shells work under the hood. It turned out to be a great way to get hands-on with process control, system calls, pipes, and memory management.

This README documents my journey, from the initial concept to the final implementation and ideas for future extensions.

---

## What I Learned

Building this project taught me how to:
- Use core system calls like `fork()`, `execvp()`, and `waitpid()`.
- Handle user input parsing (turning a string like "ls -l /home" into an array of arguments).
- Implement pipes (`|`) and I/O redirection (`>` and `<`).
- Understand how the terminal actually processes and runs commands.
- Manage child processes and handle signals.

---

## My Understanding of How a Shell Works

Here's my simplified mental model of what the shell does when I type a command like `ls -l /home`:

1.  **Read Input**: It first reads the command I typed in from the standard input.
2.  **Parse Input**: It then parses that string into a command and its arguments, like `{"ls", "-l", "/home", NULL}`.
3.  **Fork**: It creates a new child process by calling `fork()`.
4.  **Exec**: The new child process uses `execvp()` to replace its own code with the code for the `ls` command.
5.  **Wait**: Meanwhile, the parent process (my shell) waits for the child process to finish executing.
6.  **Repeat**: Once the command is done, the shell prints the prompt again, ready for my next command.

---

## How I Built It: Step-by-Step

This is the approach I took to implement the shell.

### 1. The Main Shell Loop

First, I created an infinite loop to keep the shell running. In each iteration, it prints a prompt, reads a line of input, parses it, and then executes the command.

```c
// Pseudocode for my main loop
while (1) {
    print_prompt();
    read_input(buffer);
    parse_command(buffer, args);
    execute_command(args);
}
```

### 2. Reading User Input

I used `fgets()` to read a line of input from `stdin`. I also made sure to remove the trailing newline character.

```c
fgets(buffer, sizeof(buffer), stdin);
buffer[strcspn(buffer, "
")] = 0;
```

### 3. Parsing the Command String

To break the input string into an array of arguments for `execvp()`, I used `strtok()`. The loop continues until `strtok()` returns `NULL`, and I finish by adding a `NULL` terminator to the `args` array.

```c
char *args[64];
char *token = strtok(buffer, " ");
int i = 0;
while (token != NULL) {
    args[i++] = token;
    token = strtok(NULL, " ");
}
args[i] = NULL;
```

### 4. Forking and Executing

This is where the core logic happens. I `fork()` the process. The child process gets an ID of 0 and runs `execvp()`. The parent process gets the child's ID and waits for it to complete.

```c
pid_t pid = fork();
if (pid == 0) {
    // I'm in the child process
    execvp(args[0], args);
    // This part only runs if execvp fails
    perror("exec failed");
    exit(1);
} else if (pid > 0) {
    // I'm in the parent process
    waitpid(pid, NULL, 0);
} else {
    // Forking failed
    perror("fork failed");
}
```

### 5. Adding Built-in Commands

Commands like `cd` and `exit` have to be handled directly by my shell, because they change the shell's own process. I can't use `execvp` for these. So, I added checks for these commands before the `fork()` call.

```c
if (strcmp(args[0], "cd") == 0) {
    chdir(args[1]);
    continue; // Skip the fork/exec part
}
if (strcmp(args[0], "exit") == 0) {
    break; // Exit the main loop
}
```

### 6. Implementing Pipes

To handle commands like `ls -l | grep txt`, I needed to connect the output of one process to the input of another. Here was my plan:
1.  Create a pipe with `pipe(fd)`.
2.  `fork()` twice, creating a process for each command.
3.  In the first child, redirect `stdout` to the write-end of the pipe (`fd[1]`)
4.  In the second child, redirect `stdin` to the read-end of the pipe (`fd[0]`)
5.  Finally, close the pipe ends in the parent and wait for both children to finish.

```c
// Example for piping cmd1 to cmd2
int fd[2];
pipe(fd);

if (fork() == 0) {
    dup2(fd[1], STDOUT_FILENO);
    close(fd[0]); close(fd[1]);
    execvp(cmd1[0], cmd1);
}
if (fork() == 0) {
    dup2(fd[0], STDIN_FILENO);
    close(fd[0]); close(fd[1]);
    execvp(cmd2[0], cmd2);
}
close(fd[0]); close(fd[1]);
wait(NULL); wait(NULL);
```

### 7. Implementing Redirection

For redirecting output (`>`) or input (`<`), I detect the redirection symbols, open the specified file, and then use `dup2()` to replace `STDOUT_FILENO` or `STDIN_FILENO` with the file descriptor of the opened file.

```c
// Example for output redirection
int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
dup2(fd, STDOUT_FILENO);
close(fd);
```

### 8. Signal Handling

To prevent my shell from closing when I press `Ctrl+C`, I told the parent process to ignore the `SIGINT` signal.

```c
signal(SIGINT, SIG_IGN);
```

---

## My Base Code Skeleton

Here is the minimal code I started with, which combines the loop, parsing, and fork/exec logic.

```c
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
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "
")] = 0;

        // Parse the input
        char *token = strtok(input, " ");
        int i = 0;
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (args[0] == NULL) continue; // Handle empty command
        if (strcmp(args[0], "exit") == 0) break;
        if (strcmp(args[0], "cd") == 0) {
            chdir(args[1]);
            continue;
        }

        // Fork and execute
        pid_t pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            perror("exec failed");
            exit(1);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork failed");
        }
    }
    return 0;
}
```

---

## Ideas for Future Extensions

Here are some features I'm thinking of adding next:

| Feature             | Description                                                                          |
| ------------------- | ------------------------------------------------------------------------------------ |
| **Command History** | Store previous commands and recall them with the arrow keys (might use the `readline` library). |
| **Background Jobs** | Handle `&` to run tasks in the background without making the shell wait.             |
| **Multiple Pipes**  | Chain more than two commands together, like `ls | grep .c | wc -l`.                  |
| **Custom Prompt**   | Make the prompt more dynamic, showing the username, current directory, maybe in color. |
| **Permissions Check** | Before trying to execute a command, check if the file is actually executable.        |

---

## Key Takeaways

Overall, this project was a deep dive into:
- OS-level process management (`fork`, `exec`, `wait`).
- File descriptors and I/O redirection (`dup2`, `pipe`, `open`).
- String manipulation and command parsing in C.
- General system programming concepts and best practices.
