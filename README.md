tinsh
a tiny shell. built to understand how shells actually work.

## What is this?

This is a Unix shell written from scratch in C, built not because bash needed a competitor but because the only way to really understand fork, exec, pipes, and signals is to implement them yourself. It started as a question: what actually happens when you type `ls -la` and press Enter?

## Features

redirect input and output (`>`, `>>`, `<`) - because everything in Unix is a file, including your terminal

pipe commands together (`cmd1 | cmd2`) - connecting stdout of one process to stdin of another using file descriptors

logical operators (`;`, `&&`, `||`) - implementing short-circuit evaluation with proper precedence

command substitution (`$(cmd)` and `` `cmd` ``) - capturing stdout from a child process back into the parent

quoted strings and escaped spaces (`"hello world"`, `'single quotes'`, `hello\ world`) - building a proper tokenizer that respects quote state

background jobs (`cmd &`) - tracking child processes that run without blocking the prompt

arrow key history navigation - using termios raw mode to read escape sequences from the terminal

tab completion - searching PATH and current directory for executable and file matches

signal handling (Ctrl+C, Ctrl+Z) - keeping the shell alive while terminating foreground processes

## Build and run

```bash
git clone https://github.com/yourusername/tinsh.git
cd tinsh
make
./tinsh
```

Requires clang and a Unix system (tested on macOS). Verified memory-leak free under AddressSanitizer (`clang -fsanitize=address`).

---

## Under the hood

The most surprising discovery was that `fork()` and `exec()` aren't just implementation details - they're the foundation that makes Unix work. When you call `fork()`, you get an exact copy of your process with the same file descriptors, memory mappings, and signal handlers. Then `exec()` replaces the program in that child process while preserving all those file descriptors. This two-step dance is what lets you redirect input/output before the new program even starts - you modify the child's file descriptors, then `exec()` the new program, and it inherits those modifications seamlessly.

Pipes work because file descriptors are just integers referring to entries in a per-process table. When you call `pipe()`, the kernel creates two file descriptors that refer to the same underlying pipe object - one for reading, one for writing. The magic happens with `dup2()`, which can copy one file descriptor over another. So you close stdout (file descriptor 1) and duplicate the pipe's write-end over it. Now when the child process writes to stdout, it's actually writing into the pipe. The parent process reads from the read-end, and you've connected two programs without any temporary files.

The `cd` command has to be built-in because processes can't change each other's working directories. When the shell forks a child process to run `ls`, that child gets a copy of the shell's current directory. If `ls` could change directories, it would only affect itself, not the shell. The shell itself has to call `chdir()` to change its own working directory, which is why `cd` runs in the parent process rather than a child.

Signal handling revealed why pressing Ctrl+C doesn't kill your shell. Signals are sent to entire process groups, and the shell puts foreground commands in their own process group. When Ctrl+C sends SIGINT, the shell's signal handler catches it and forwards it to the child process group. The shell ignores the signal itself, so it stays alive while the foreground command terminates. This required careful setup with `sigaction()` and process group management.

Raw terminal mode was the gateway to interactive features. Normally terminals operate in "cooked" mode where they buffer lines and handle backspace, arrow keys, and control characters locally. Raw mode disables all that, letting the shell read each character as it's typed. Arrow keys send escape sequences like `\x1b[A`, so the shell has to read multiple bytes to recognize a single keypress. Tab completion works by temporarily restoring cooked mode to display completion options, then switching back to raw mode.

Command substitution is essentially a reverse pipe. Instead of the parent writing to a child's stdin, the child writes to its stdout which the parent reads through a pipe. The tricky part is that the child's stdout is also connected to the terminal, so you have to carefully manage file descriptors to capture the output without breaking the terminal connection. The solution is to redirect the child's stdout to the pipe before executing the command, then read the pipe contents back into the parent process.

## What I learned

The hardest part wasn't implementing the features, it was understanding why the Unix abstractions are designed the way they are. File descriptors, process groups, signal inheritance - these aren't arbitrary design choices, they're a coherent model that makes powerful composition possible. Building this shell taught me that Unix isn't just a collection of system calls, it's a carefully designed system where each piece builds on the others.

## Limitations and future work

No `$PS1` customization - the prompt format is fixed
No `.tinshrc` configuration file - settings can't be persisted
No job control beyond basic background/foreground - no `fg` or `bg` commands
No arithmetic expansion like `$((2+2))`
No environment variable expansion beyond basic `$HOME`
No command line editing beyond arrow key navigation
