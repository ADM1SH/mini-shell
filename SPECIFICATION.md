# Mini Shell in C — Full Feature Specification

---

## 1. Core Shell Loop ✅

**Purpose**: The main loop that keeps the shell running continuously until the user exits.

**Features**:
- ✅ Display a prompt (e.g. `myshell> `).
- ✅ Wait for user input using `readline()`.
- ✅ Remove the newline character from input.
- ✅ Parse the command into arguments.
- ✅ Execute built-in or external commands.
- ✅ Repeat indefinitely until `exit` command or EOF.

---

## 2. Command Parsing ⚠️

**Purpose**: Convert a string like "ls -l /home" into an array usable by `execvp()`.

**Features**:
- ✅ Tokenize input string using spaces as delimiters (`strtok`).
- ✅ Store tokens into a `char *args[]` array.
- ⚠️ Handle multiple spaces between tokens.
- ❌ Handle Quotes (`"` or `'`) for multi-word arguments.
- ❌ Handle Escaped characters (`\`).
- ✅ Ensure `args` is `NULL`-terminated for `execvp()`.

---

## 3. Built-in Commands ✅

**Purpose**: Handle commands that affect the shell’s own process.

**Supported Built-ins**:
- ✅ `cd <directory>`
- ✅ `exit`
- ✅ `help`
- ✅ `clear`
- ✅ `pwd` (via `execvp`)
- ✅ `alias` (simple version)
- ✅ `unalias`
- ✅ `printenv`
- ✅ `setenv`
- ✅ `unsetenv`
- ✅ `history`

---

## 4. External Command Execution ✅

**Purpose**: Run programs found in the system’s `$PATH`.

**Features**:
- ✅ Use `fork()` to create a child process.
- ✅ Use `execvp()` in the child to execute the command.
- ✅ Use `waitpid()` in the parent to wait for completion.
- ✅ Print an error message if command not found or fails to execute.
- ✅ Support relative and absolute paths.

---

## 5. Input & Output Redirection ✅

**Purpose**: Support shell operators `>`, `<`, `>>`.

**Features**:
- ✅ Detect and parse redirection symbols in input.
- ✅ For `>` (overwrite output).
- ✅ For `>>` (append output).
- ✅ For `<` (input redirection).
- ✅ Close file descriptors after redirection.
- ✅ Ensure redirection doesn’t interfere with normal args.

---

## 6. Pipes (|) ⚠️

**Purpose**: Allow output of one command to be input of another.

**Features**:
- ✅ Detect pipe symbols (`|`) and split commands.
- ✅ Create a pipe using `pipe(fd)`.
- ✅ `fork()` each command and redirect I/O.
- ✅ Support multiple pipes.
- ⚠️ Has a difficult-to-reproduce bug with specific `grep` arguments.

---

## 7. Background Processes (&) ✅

**Purpose**: Run commands without blocking the shell.

**Features**:
- ✅ Detect if command ends with `&`.
- ✅ If yes, run `fork()` but don’t call `waitpid()`.
- ✅ Print background process PID.
- ❌ Store running background processes in a list (not implemented for `jobs` command).
- ✅ Use `waitpid(-1, NULL, WNOHANG)` periodically to reap finished processes.

---

## 8. Command History ✅

**Purpose**: Allow users to view and re-run previous commands.

**Features**:
- ✅ Store commands in a history list.
- ✅ Use up/down arrows via `readline()` library.
- ✅ Add a built-in command: `history`.
- ❌ Save history to `~/.myshell_history` file on exit, and load it on startup.

---

## 9. Signal Handling ⚠️

**Purpose**: Prevent the shell from closing unexpectedly (like on `Ctrl+C`).

**Features**:
- ✅ Ignore `SIGINT` (`Ctrl+C`) in parent shell.
- ✅ Restore default `SIGINT` handling in child processes.
- ✅ Handle `SIGCHLD` to reap zombie processes (via `waitpid` in main loop).
- ❌ Optional: Handle `Ctrl+Z` (`SIGTSTP`) for stopping foreground jobs.

---

## 10. Environment Variables ✅

**Purpose**: Let users view and set shell environment variables.

**Features**:
- ✅ Built-in command: `setenv VAR value` and `unsetenv VAR`.
- ✅ Built-in command: `printenv`.
- ✅ Allow `$VAR` expansion in command input.
- ✅ Pass environment variables to `execvp()` calls automatically.

---

## 11. Custom Prompt ❌

**Purpose**: Make the shell look personalized and informative.

**Features**:
- ❌ Display username, hostname, and current directory.
- ❌ Support ANSI colors.
- ❌ Optionally show exit code of last command.

---

## 12. Error Handling ⚠️

**Purpose**: Graceful handling of all failure cases.

**Features**:
- ✅ Detect empty input and skip.
- ✅ Handle invalid commands ("Command not found").
- ✅ Handle invalid paths on `cd`.
- ✅ Handle missing files on redirection.
- ✅ Catch `fork()` or `pipe()` errors.
- ❌ Don’t crash on malformed input (like `| |` or `>` with no filename).

---

## 13. Configuration File (Optional) ✅

**Purpose**: Run startup commands or define aliases automatically.

**Features**:
- ✅ Load `.myshellrc` from home directory on startup.
- ✅ Execute each line as a shell command.
- ✅ Allow custom prompt settings, aliases, and environment setup.

---

## 14. Aliases (Optional Advanced Feature) ⚠️

**Purpose**: Let users define shortcuts for commands.

**Features**:
- ⚠️ `alias name cmd` (simple, single-word command only).
- ❌ `alias ll='ls -l'` (multi-word not supported).
- ✅ `unalias ll`.
- ✅ Store aliases in memory.

---

## 15. Job Control (Advanced) ❌

**Purpose**: Manage multiple foreground/background tasks.

**Features**:
- ❌ Use `kill()`, `fg`, `bg`, and `jobs` commands.

---

## 16. Performance Improvements (Optional) ❌

**Purpose**: Optimize for efficiency and responsiveness.

**Features**:
- ❌ Use dynamic memory (`malloc`/`realloc`) for parsing long commands.
- ❌ Use non-blocking I/O for background jobs.
- ❌ Cache `$PATH` lookup results to speed up execution.

---

## 17. Cross-Platform Notes ✅

**Purpose**: Make shell compile on both Linux and macOS.

**Features**:
- ✅ Use standard POSIX functions only.

---

## 18. Exit Behavior ⚠️

**Purpose**: Graceful shutdown and cleanup.

**Features**:
- ❌ Save history before exit.
- ❌ Print farewell message.
- ⚠️ Free *some* allocated memory (`readline` buffer), but not all (e.g. aliases).