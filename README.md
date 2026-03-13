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

## Under the hood

The most surprising discovery was that `fork()` and `exec()` aren't just implementation details - they're the foundation that makes Unix work. When you call `fork()`, you get an exact copy of your process with the same file descriptors, memory mappings, and signal handlers. Then `exec()` replaces the program in that child process while preserving all those file descriptors. This two-step dance is what lets you redirect input/output before the new program even starts - you modify the child's file descriptors, then `exec()` the new program, and it inherits those modifications seamlessly.

Pipes work because file descriptors are just integers referring to entries in a per-process table. When you call `pipe()`, the kernel creates two file descriptors that refer to the same underlying pipe object - one for reading, one for writing. The magic happens with `dup2()`, which can copy one file descriptor over another. So you close stdout (file descriptor 1) and duplicate the pipe's write-end over it. Now when the child process writes to stdout, it's actually writing into the pipe. The parent process reads from the read-end, and you've connected two programs without any temporary files.

The `cd` command has to be built-in because processes can't change each other's working directories. When the shell forks a child process to run `ls`, that child gets a copy of the shell's current directory. If `ls` could change directories, it would only affect itself, not the shell. The shell itself has to call `chdir()` to change its own working directory, which is why `cd` runs in the parent process rather than a child.

## What I discovered

Building this shell wasn't about implementing features - it was about understanding why Unix works the way it does. The elegance isn't in having more functions, it's in how a few simple concepts combine to create something powerful. File descriptors as universal handles, process groups as isolation boundaries, signals as communication channels - these fit together perfectly.

The hardest part wasn't the code, it was unlearning assumptions about how things "should" work and seeing how they actually do work.

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
