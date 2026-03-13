# tinsh - Tiny Shell

tinsh is a minimal Unix shell implementation written in C that demonstrates how shells work at the system level. Built from scratch to understand the fundamental interactions between processes, file descriptors, and the terminal.

## Features

- **Basic REPL**: Clean prompt with exit code visualization (white for success, red for failure)
- **Built-in commands**: `cd`, `exit`, `pwd`, `history`, `jobs` - essential commands that must run in the shell process
- **Advanced parsing**: Full support for quoted strings (`"hello world"`, `'single quotes'`) and escaped spaces (`hello\ world`) with proper tokenization
- **Command substitution**: POSIX style `$(cmd)` and backtick style `` `cmd` `` with nested support and word splitting
- **Logical operators**: Semicolon (`;`), AND (`&&`), OR (`||`) with proper precedence and short-circuit evaluation
- **I/O redirection**: Support for `<` (input), `>` (output), and `>>` (append) using file descriptor manipulation
- **Pipes**: Chain multiple commands with `|` operator using inter-process communication
- **Signal handling**: Proper handling of Ctrl+C and Ctrl+Z without killing the shell using process groups
- **Background jobs**: Run commands with `&` and track them with the `jobs` command
- **Interactive features**: Arrow key history navigation and tab completion using raw terminal mode

### Usage Examples

```bash
# Basic interaction
ls -la
pwd

# Built-in commands (must run in shell process)
cd /tmp
history
jobs

# Shell configuration file (~/.tinshrc)
# Create a config file with commands to run on startup
echo 'echo "Welcome to tinsh"' > ~/.tinshrc
# Commands in config are executed automatically on shell launch

# Quoted strings and escaped spaces
echo "hello world"
echo 'single quotes work too'
echo hello\ world

# I/O redirection
ls > file_list.txt
cat file_list.txt >> backup.txt
sort < unsorted.txt

# Pipes with inter-process communication
ls -la | grep ".c" | wc -l
ps aux | grep tinsh

# Background jobs
sleep 10 &
find / -name "*.txt" > results.txt &

# Logical operators with precedence
true && echo "this will print"
false && echo "this won't print"
false || echo "this will print"
true || echo "this won't print"
cmd1 && cmd2 || cmd3; cmd4

# Command substitution with nesting
echo "today is $(date)"
echo "files: $(ls | head -5)"
echo `pwd`  # Backtick style
echo "nested: $(echo $(date))"

# Word splitting behavior
echo $(echo "a b c")  # Three arguments
echo "$(echo "a b c")"  # One argument

# Interactive features
# Use arrow keys to navigate history
# Press Tab to complete commands and files
```

## Build and Run

```bash
# Clone and build
git clone https://github.com/yourusername/tinsh.git
cd tinsh
make

# Run the shell
./tinsh
```

**Requirements**: clang compiler and Unix system (tested on macOS)  
**Memory safety**: Verified leak-free under AddressSanitizer (`clang -fsanitize=address`)

## Technical Implementation

The core insight from building tinsh is that Unix shells are fundamentally about process choreography and file descriptor manipulation. Every command you type becomes a dance of `fork()`, `exec()`, `pipe()`, and `dup2()` working together.

**Process Creation**: When you type `ls`, tinsh calls `fork()` to create a child process that's an exact copy of the shell. The parent continues waiting for more input while the child transforms itself into `ls` via `execvp()`. This separation is crucial - it lets the shell manage child processes while staying responsive.

**File Descriptors**: Unix treats everything as a file, including your terminal. When you redirect with `> file`, `dup2()` copies the file's descriptor over stdout (descriptor 1). After this, anything the child writes to stdout actually goes to the file. The same principle makes pipes work - `pipe()` creates two descriptors, and connecting one command's stdout to another's stdin is just clever descriptor manipulation.

**Signal Boundaries**: Ctrl+C sends SIGINT to all processes in the foreground process group. tinsh uses `sigaction()` to catch this signal and forward it only to the child process group. The shell ignores the signal itself, which is why Ctrl+C kills your command but keeps the shell running.

**Terminal Interaction**: Arrow keys and tab completion require switching from "cooked" mode (where the terminal handles line editing locally) to "raw" mode (where each keystroke is read individually). Arrow keys send escape sequences like `\x1b[A`, so the shell reads multiple bytes to recognize a single keypress.

**Command Substitution**: This is essentially a reverse pipe. Instead of the parent writing to a child's stdin, the child writes to its stdout which the parent captures through a pipe. The implementation redirects the child's stdout to a pipe before execution, then reads the results back into the parent process for substitution.

**Memory Management**: Every `malloc()` has a corresponding `free()`, and the shell has been verified memory-leak free using AddressSanitizer. The code uses defensive programming practices to prevent buffer overflows and null pointer dereferences.

## Known Limitations

- No command line editing beyond arrow key navigation
- No job control beyond basic background execution (`fg`, `bg` commands missing)
- No environment variable expansion (`$VAR` syntax not supported)
- Maximum limits on arguments, jobs, and history length
- Shell configuration file (`.tinshrc`) supported but limited to command execution only

## Future Work

- Implement command line editing using readline library for better editing experience
- Add job control commands (`fg`, `bg`, `kill` with job IDs)
- Support environment variable expansion with proper escaping
- Add shell configuration file support for persistent settings with variable assignment
- Implement arithmetic expansion for shell scripting
- Add process substitution (`>(process)` and `<(process)`) support

---

*Built to understand how shells work at the system level, not to replace existing ones.*
