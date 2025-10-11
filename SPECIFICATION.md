# Mini Shell in C — Full Feature Specification

---

## 1. Core Shell Loop

**Purpose**: The main loop that keeps the shell running continuously until the user exits.

**Features**:
- Display a prompt (e.g. `myshell> `).
- Wait for user input using `fgets()` or `getline()`.
- Remove the newline character from input.
- Parse the command into arguments.
- Execute built-in or external commands.
- Repeat indefinitely until `exit` command or EOF.

---

## 2. Command Parsing

**Purpose**: Convert a string like "ls -l /home" into an array usable by `execvp()`.

**Features**:
- Tokenize input string using spaces as delimiters (`strtok` or a custom parser).
- Store tokens into a `char *args[]` array.
- Handle:
    - Multiple spaces between tokens.
    - Quotes (`"` or `'`) for multi-word arguments.
    - Escaped characters (`\`).
- Ensure `args` is `NULL`-terminated for `execvp()`.

---

## 3. Built-in Commands

**Purpose**: Handle commands that affect the shell’s own process.

**Supported Built-ins**:
- `cd <directory>`: Change current working directory.
    - If no argument given, default to `$HOME`.
    - Handle invalid directories gracefully.
- `exit`: Exit the shell gracefully.
- `help`: Display basic help about built-in commands.
- `clear`: Clear terminal screen using `system("clear")` or ANSI codes.
- `pwd`: Print the current working directory (`getcwd()`).

(Optional but useful built-ins can include: `echo`, `history`, and `alias`.)

---

## 4. External Command Execution

**Purpose**: Run programs found in the system’s `$PATH`.

**Features**:
- Use `fork()` to create a child process.
- Use `execvp()` in the child to execute the command.
- Use `waitpid()` in the parent to wait for completion.
- Print an error message if command not found or fails to execute.
- Support relative and absolute paths (e.g., `./a.out`, `/bin/ls`).

---

## 5. Input & Output Redirection

**Purpose**: Support shell operators `>`, `<`, `>>`.

**Features**:
- Detect and parse redirection symbols in input.
- For `>` (overwrite output):
    - Open target file with `O_WRONLY | O_CREAT | O_TRUNC`.
    - Redirect `STDOUT` using `dup2(fd, STDOUT_FILENO)`.
- For `>>` (append output):
    - Open target file with `O_WRONLY | O_CREAT | O_APPEND`.
- For `<` (input redirection):
    - Open file with `O_RDONLY`.
    - Redirect `STDIN` using `dup2(fd, STDIN_FILENO)`.
- Close file descriptors after redirection.
- Ensure redirection doesn’t interfere with normal args (e.g., remove them from `args`).

---

## 6. Pipes (|)

**Purpose**: Allow output of one command to be input of another.

**Features**:
- Detect pipe symbols (`|`) and split commands.
- Create a pipe using `pipe(fd)`.
- `fork()` each command:
    - Left command: redirect output to `fd[1]` (write end).
    - Right command: redirect input to `fd[0]` (read end).
- Close pipe ends after duplication.
- Support multiple pipes (chain commands like `ls | grep txt | wc -l`).
- Use a loop to chain multiple pipe segments dynamically.

---

## 7. Background Processes (&)

**Purpose**: Run commands without blocking the shell.

**Features**:
- Detect if command ends with `&`.
- If yes, run `fork()` but don’t call `waitpid()`.
- Print background process PID.
- Store running background processes in a list.
- Use `waitpid(-1, NULL, WNOHANG)` periodically to reap finished processes.

---

## 8. Command History

**Purpose**: Allow users to view and re-run previous commands.

**Features**:
- Store last N commands (e.g., 100) in a history array or file.
- Use up/down arrows via `readline()` library (optional).
- Add a built-in command:
    - `history`: display previous commands.
- Save history to `~/.myshell_history` file on exit, and load it on startup.

---

## 9. Signal Handling

**Purpose**: Prevent the shell from closing unexpectedly (like on `Ctrl+C`).

**Features**:
- Ignore `SIGINT` (`Ctrl+C`) in parent shell.
- Restore default `SIGINT` handling in child processes before `execvp()`.
- Handle `SIGCHLD` to reap zombie processes.
- Optional: Handle `Ctrl+Z` (`SIGTSTP`) for stopping foreground jobs.

---

## 10. Environment Variables

**Purpose**: Let users view and set shell environment variables.

**Features**:
- Built-in command: `setenv VAR value` and `unsetenv VAR`.
- Built-in command: `printenv` to display all environment variables.
- Allow `$VAR` expansion in command input (basic parser for `$HOME`, `$USER`, etc.).
- Pass environment variables to `execvp()` calls automatically.

---

## 11. Custom Prompt

**Purpose**: Make the shell look personalized and informative.

**Features**:
- Display username, hostname, and current directory.
- Example prompt: `[adam@myshell ~/Desktop]$`
- Support ANSI colors (e.g., green for success, red for errors).
- Optionally show exit code of last command.

---

## 12. Error Handling

**Purpose**: Graceful handling of all failure cases.

**Features**:
- Detect empty input and skip.
- Handle invalid commands ("Command not found").
- Handle invalid paths on `cd`.
- Handle missing files on redirection.
- Catch `fork()` or `pipe()` errors and print descriptive messages.
- Don’t crash on malformed input (like `| |` or `>` with no filename).

---

## 13. Configuration File (Optional)

**Purpose**: Run startup commands or define aliases automatically.

**Features**:
- Load `.myshellrc` from home directory on startup.
- Execute each line as a shell command.
- Allow custom prompt settings, aliases, and environment setup.

---

## 14. Aliases (Optional Advanced Feature)

**Purpose**: Let users define shortcuts for commands.

**Features**:
- `alias ll='ls -l'`
- `unalias ll`
- Store aliases in memory (and optionally in `.myshellrc`).

---

## 15. Job Control (Advanced)

**Purpose**: Manage multiple foreground/background tasks.

**Features**:
- Use `kill()`, `fg`, `bg`, and `jobs` commands.
- Display active jobs with PIDs and states.
- Resume or terminate jobs manually.

---

## 16. Performance Improvements

**Purpose**: Optimize for efficiency and responsiveness.

**Features**:
- Use dynamic memory (`malloc`/`realloc`) for parsing long commands.
- Use non-blocking I/O for background jobs.
- Cache `$PATH` lookup results to speed up execution.

---

## 17. Cross-Platform Notes

**Purpose**: Make shell compile on both Linux and macOS.

**Features**:
- Use standard POSIX functions only.
- Conditional includes for `<linux/limits.h>` vs `<sys/param.h>`.
- Verify `pipe`, `fork`, `exec`, and `dup2` behavior on both platforms.

---

## 18. Exit Behavior

**Purpose**: Graceful shutdown and cleanup.

**Features**:
- Save history before exit.
- Print farewell message.
- Free allocated memory and close open file descriptors.

---

## Example Command Features Supported

| Command Example | Behavior |
|---|---|
| `ls -l` | Runs `ls` with argument `-l`. |
| `pwd` | Shows current working directory. |
| `cd /home/user` | Changes directory. |
| `ls | grep .c` | Pipes `ls` output to `grep`. |
| `cat file.txt > output.txt` | Redirects output to a file. |
| `sort < input.txt` | Reads from input file. |
| `ls -l &` | Runs command in background. |
| `echo $HOME` | Prints value of environment variable. |
| `setenv PATH /usr/bin` | Changes environment variable. |
| `history` | Shows past commands. |

---

## Optional Bonus Features

- **Auto-completion**: Press `Tab` to complete filenames/commands.
- **Scripting Mode**: `myshell script.sh` to run commands from a file.
- **Command timing**: Show how long each command took.
- **Colored output**: For errors, warnings, or pipes.
- **Log file**: To track shell usage or errors.

---

## Summary

This shell can be built as a modular system:
1.  Core execution engine (loop + fork/exec + wait).
2.  Parser (tokenization, redirection, pipes).
3.  Built-ins handler.
4.  Signal & background job manager.
5.  Optional add-ons (history, aliases, prompt, scripting).
