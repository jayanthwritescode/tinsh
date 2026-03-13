# tinsh - Tiny Shell

tinsh is a minimal Unix shell implementation written in C that provides the core functionality expected from a command-line interface. It demonstrates fundamental systems programming concepts including process management, inter-process communication, and signal handling.

## Features

- **Basic REPL**: Clean prompt showing current working directory with tilde abbreviation
- **Exit code visualization**: Prompt `$` is bold white for success, bold red for failure
- **Built-in commands**: `cd`, `exit`, `pwd`, `history`, `jobs`
- **Advanced parsing**: Full support for quoted strings (`"hello world"`, `'single quotes'`) and escaped spaces (`hello\ world`)
- **I/O redirection**: Support for `<` (input), `>` (output), and `>>` (append)
- **Pipes**: Chain multiple commands with `|` operator
- **Signal handling**: Proper handling of Ctrl+C and Ctrl+Z without killing the shell
- **Background jobs**: Run commands with `&` and track them with the `jobs` command
- **Arrow key navigation**: Up/down arrows cycle through command history
- **Tab completion**: Complete commands from PATH and local filenames

### Usage Examples

```bash
# Basic commands
ls -la
pwd

# Built-in commands
cd /tmp
history
jobs

# Quoted strings and escaped spaces
echo "hello world"
echo 'single quotes work too'
echo hello\ world

# I/O redirection
ls > file_list.txt
cat file_list.txt >> backup.txt
sort < unsorted.txt

# Pipes
ls -la | grep ".c" | wc -l
ps aux | grep tinsh

# Background jobs
sleep 10 &
find / -name "*.txt" > results.txt &

# Arrow key navigation (use up/down arrows to browse history)
# Tab completion (press Tab to complete commands and files)
```

## Build and Run

### Prerequisites

- macOS with Xcode Command Line Tools (clang compiler)
- Make utility

### Building

```bash
cd tinsh
make
```

### Running

```bash
# Run the shell
./tinsh

# Or use the make target
make run
```

### Cleaning

```bash
make clean
```

## Under the Hood

A Unix shell is essentially a program that creates and manages other programs. At its core, tinsh demonstrates five critical system calls that make this possible:

**fork()** is the foundation of process creation in Unix. When you type a command, tinsh calls `fork()` to create an exact copy of itself. The parent process (the shell) continues running to accept more commands, while the child process becomes the new command. Without `fork()`, we couldn't run external programs at all - every command would have to be built into the shell itself.

**execvp()** transforms a process into a different program. After `fork()` creates the child process, `execvp()` replaces the shell's code in that child with the requested command's code. This is why typing `ls` doesn't give you another shell - the child process completely becomes the `ls` program. If `execvp()` fails, the child would continue running shell code, which is why we exit immediately after a failed exec.

**pipe()** creates communication channels between processes. When you type `cmd1 | cmd2`, `pipe()` creates a pair of file descriptors where one writes and the other reads. The first command's stdout is connected to one end, the second's stdin to the other. Without pipes, you couldn't chain commands - you'd have to save intermediate results to temporary files.

**dup2()** redirects file descriptors, enabling I/O redirection. When you type `cmd > file`, `dup2()` copies the file's file descriptor onto stdout's descriptor (usually 1). After this, anything written to stdout goes to the file instead. The same principle works for input redirection with stdin. Without `dup2()`, commands would be stuck with standard input/output and couldn't read from or write to files.

**sigaction()** provides robust signal handling. When you press Ctrl+C, the kernel sends SIGINT to every process in the foreground process group. tinsh uses `sigaction()` to catch this signal and forward it only to the running child process, not the shell itself. This is why Ctrl+C kills your command but keeps the shell running. Without proper signal handling, Ctrl+C would terminate the entire shell session.

These system calls work together: `fork()` creates the process, `pipe()` and `dup2()` set up its environment, `execvp()` transforms it into the desired program, and `sigaction()` keeps everything under control. The beauty of Unix is that these simple primitives combine to create powerful command-line workflows.

## Known Limitations

- No command line editing (arrow keys, tab completion)
- No job control beyond basic background execution
- Limited error handling for complex command combinations
- No support for environment variable expansion
- Maximum limits on arguments, jobs, and history length
- No quoting or escaping mechanisms for complex arguments

## Future Work

- Implement command line editing using readline library
- Add tab completion for commands and file paths
- Support environment variable expansion ($VAR)
- Add job control commands (fg, bg, kill)
- Implement command substitution with backticks
- Add support for logical operators (&&, ||)
- Implement quoting and escape character handling
