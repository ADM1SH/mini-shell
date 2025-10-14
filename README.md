# My Mini Shell Project (in C)

I built this mini shell in C because I wanted a project that would force me to understand how Linux and shells work under the hood. It turned out to be a great way to get hands-on with process control, system calls, pipes, and memory management.

This README documents my journey and the final architecture of the shell.

---

## What I Learned

Building this project taught me how to:
- Use core system calls like `fork()`, `execvp()`, and `waitpid()`.
- Implement a full pipeline of commands with `pipe()` and `dup2()`.
- Manage child processes, including background jobs and zombie processes.
- Handle signals like `SIGINT` (`Ctrl+C`).
- Use external libraries like `readline` to add features like command history.
- Manage shell state for built-in commands like `cd`, `setenv`, and `alias`.
- Read and execute commands from a startup script (`.myshellrc`).

---

## Final Architecture

The initial approach was a simple loop, but to support advanced features, the final architecture became more sophisticated.

1.  **Startup:** The shell first looks for and executes `~/.myshellrc`.
2.  **Main Loop:** The main `while` loop reads a line of input using the `readline()` library, which provides history and editing capabilities.
3.  **Line Processing:** Each line is passed to a central `process_line` function. This function is responsible for:
    -   **Parsing:** Using `strtok()` to split the line into arguments.
    -   **Alias Expansion:** Checking if the command is an alias and replacing it.
    -   **Built-in Handling:** Checking for and executing built-in commands that modify the shell itself (like `cd`, `alias`, `setenv`).
4.  **Execution Pipeline:** If the command is not a state-changing built-in, it's passed to `execute_pipeline`. This function:
    -   Scans for pipes (`|`) and splits the command into a sequence of sub-commands.
    -   Loops through the sub-commands, creating a child process for each one and connecting them with pipes.
5.  **Command Handling:** Each child process calls `handle_command`, which is responsible for:
    -   **Variable Expansion:** Replacing arguments like `$HOME`.
    -   **Redirection:** Setting up I/O redirection (`<`, `>`) using `dup2()`.
    -   **Execution:** Calling `execvp()` to run the final command (or a piped built-in like `printenv`).
6.  **Process Management:** The parent process waits for foreground jobs to complete, but not background jobs (`&`). A `waitpid()` call in the main loop cleans up any zombie processes.

---

## Key Features Implemented

-   **Execution**: Runs external commands and handles the `$PATH`.
-   **Pipes**: Chains multiple commands together (e.g., `ls | grep .c | wc -l`).
-   **I/O Redirection**: Redirects `stdin` and `stdout` using `<`, `>`, and `>>`.
-   **Background Jobs**: Runs commands in the background with `&`.
-   **Command History**: Uses `readline` for arrow-key history and has a `history` command.
-   **Environment Variables**: Supports `setenv`, `unsetenv`, `printenv`, and `$VAR` expansion.
-   **Aliases**: Simple single-word aliases via the `alias` and `unalias` commands.
-   **Startup Script**: Executes commands from `~/.myshellrc` on launch.
-   **Built-ins**: `cd`, `exit`, `help`, `clear`, and more.

---

## Future Extensions

While the shell is substantially complete, here are some features from the specification that could be added:

| Feature             | Description                                                                          |
| ------------------- | ------------------------------------------------------------------------------------ |
| **Advanced Parsing**| Add support for quotes (`"` and `'`) and escape characters (`\`) for complex arguments. |
| **Job Control**     | Implement `jobs`, `fg`, `bg` commands and `Ctrl+Z` support for full job management.    |
| **Custom Prompt**   | Allow the prompt to be customized to show username, directory, etc. (e.g., `PS1`).      |
| **Bug Fixes**       | Investigate the rare pipe bug and potential issues with the test harness.            |