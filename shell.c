#include "shell.h"

/* Global variables */
job_t jobs[MAX_JOBS];
int next_job_id = 1;
char history[MAX_HISTORY][MAX_INPUT_LEN];
int history_count = 0;
volatile sig_atomic_t foreground_pid = 0;
int last_exit_code = 0;
struct termios orig_termios;
int history_position = -1;

/* Initialize shell state */
void initialize_shell(void) {
    /* Clear job table */
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs[i].job_id = 0;
        jobs[i].pid = 0;
        jobs[i].command[0] = '\0';
        jobs[i].running = false;
    }
    
    /* Clear history */
    for (int i = 0; i < MAX_HISTORY; i++) {
        history[i][0] = '\0';
    }
    
    /* Save original terminal settings */
    tcgetattr(STDIN_FILENO, &orig_termios);
    
    /* Setup signal handlers */
    setup_signal_handlers();
    
    /* Print startup banner */
    printf("tinsh 1.0  |  type 'exit' to quit\n");
}

/* Main shell REPL loop */
void shell_loop(void) {
    char *input;
    command_t commands[MAX_ARGS];  /* Support multiple piped commands */
    int num_commands;
    
    while (1) {
        /* Check for completed background jobs */
        update_job_status();
        
        /* Print prompt and read input */
        print_prompt();
        input = read_input();
        
        /* Handle EOF (Ctrl+D) */
        if (input == NULL) {
            printf("\ngoodbye.\n");
            exit(0);
        }
        
        /* Skip empty input */
        if (strlen(input) == 0) {
            free(input);
            continue;
        }
        
        /* Add to history */
        add_to_history(input);
        
        /* Parse input into commands */
        parse_input_advanced(input, commands, &num_commands);
        
        /* Skip if no commands found */
        if (num_commands == 0) {
            free(input);
            continue;
        }
        
        /* Check if command is built-in */
        if (is_builtin(commands[0].args[0])) {
            /* Built-in commands run in parent process */
            if (strcmp(commands[0].args[0], "exit") == 0) {
                builtin_exit(commands[0].args);
            } else if (strcmp(commands[0].args[0], "cd") == 0) {
                builtin_cd(commands[0].args);
            } else if (strcmp(commands[0].args[0], "pwd") == 0) {
                builtin_pwd(commands[0].args);
            } else if (strcmp(commands[0].args[0], "history") == 0) {
                builtin_history(commands[0].args);
            } else if (strcmp(commands[0].args[0], "jobs") == 0) {
                builtin_jobs(commands[0].args);
            }
        } else {
            /* External commands */
            if (num_commands > 1) {
                /* Execute pipeline */
                execute_pipeline(commands, num_commands);
            } else {
                /* Execute single command */
                execute_command(&commands[0]);
            }
        }
        
        free(input);
    }
}

/* Read input from user - replaced by enhanced version below */

/* Parse input string into command structures with quote and escape handling */
void parse_input_advanced(char *input, command_t *commands, int *num_commands) {
    *num_commands = 0;
    char current_arg[MAX_INPUT_LEN];
    int arg_pos = 0;
    bool in_quotes = false;
    bool in_single_quotes = false;
    bool escape_next = false;
    int cmd_arg_index = 0;
    
    /* Initialize first command */
    commands[0].argc = 0;
    commands[0].input_file = NULL;
    commands[0].output_file = NULL;
    commands[0].append_file = NULL;
    commands[0].background = false;
    commands[0].pipe_next = false;
    
    /* Check for background operator at end */
    char *ampersand = strchr(input, '&');
    if (ampersand != NULL && ampersand == input + strlen(input) - 1) {
        commands[0].background = true;
        *ampersand = '\0';  /* Remove & from input */
    }
    
    for (int i = 0; input[i] != '\0'; i++) {
        char c = input[i];
        
        if (escape_next) {
            /* Add escaped character to current argument */
            if (arg_pos < MAX_INPUT_LEN - 1) {
                current_arg[arg_pos++] = c;
            }
            escape_next = false;
        } else if (c == '\\') {
            /* Next character is escaped */
            escape_next = true;
        } else if (c == '"' && !in_single_quotes) {
            /* Toggle double quotes */
            in_quotes = !in_quotes;
        } else if (c == '\'' && !in_quotes) {
            /* Toggle single quotes */
            in_single_quotes = !in_single_quotes;
        } else if ((c == ' ' || c == '\t') && !in_quotes && !in_single_quotes) {
            /* End of argument */
            if (arg_pos > 0) {
                current_arg[arg_pos] = '\0';
                
                /* Handle I/O redirection */
                if (strcmp(current_arg, "<") == 0) {
                    /* Input redirection */
                    i++;
                    while (input[i] == ' ' || input[i] == '\t') i++;
                    if (input[i] != '\0') {
                        commands[*num_commands].input_file = strdup(&input[i]);
                        while (input[i] != '\0' && input[i] != ' ' && input[i] != '\t') i++;
                        i--;
                    }
                } else if (strcmp(current_arg, ">") == 0) {
                    /* Output redirection (truncate) */
                    i++;
                    while (input[i] == ' ' || input[i] == '\t') i++;
                    if (input[i] != '\0') {
                        commands[*num_commands].output_file = strdup(&input[i]);
                        while (input[i] != '\0' && input[i] != ' ' && input[i] != '\t') i++;
                        i--;
                    }
                } else if (strcmp(current_arg, ">>") == 0) {
                    /* Output redirection (append) */
                    i++;
                    while (input[i] == ' ' || input[i] == '\t') i++;
                    if (input[i] != '\0') {
                        commands[*num_commands].append_file = strdup(&input[i]);
                        while (input[i] != '\0' && input[i] != ' ' && input[i] != '\t') i++;
                        i--;
                    }
                } else if (strcmp(current_arg, "|") == 0) {
                    /* Pipe to next command */
                    commands[*num_commands].pipe_next = true;
                    commands[*num_commands].args[cmd_arg_index] = NULL;
                    
                    /* Initialize next command */
                    (*num_commands)++;
                    commands[*num_commands].argc = 0;
                    commands[*num_commands].input_file = NULL;
                    commands[*num_commands].output_file = NULL;
                    commands[*num_commands].append_file = NULL;
                    commands[*num_commands].background = false;
                    commands[*num_commands].pipe_next = false;
                    cmd_arg_index = 0;
                } else {
                    /* Regular argument */
                    commands[*num_commands].args[cmd_arg_index++] = strdup(current_arg);
                    commands[*num_commands].argc++;
                    
                    if (cmd_arg_index >= MAX_ARGS - 1) {
                        print_error("parse", "too many arguments");
                        break;
                    }
                }
                
                arg_pos = 0;
            }
        } else {
            /* Add character to current argument */
            if (arg_pos < MAX_INPUT_LEN - 1) {
                current_arg[arg_pos++] = c;
            }
        }
    }
    
    /* Handle last argument */
    if (arg_pos > 0) {
        current_arg[arg_pos] = '\0';
        commands[*num_commands].args[cmd_arg_index++] = strdup(current_arg);
        commands[*num_commands].argc++;
    }
    
    /* Null-terminate last command's args */
    commands[*num_commands].args[cmd_arg_index] = NULL;
    (*num_commands)++;
}

/* Add command to history */
void add_to_history(const char *command) {
    /* Shift history if full */
    if (history_count >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            strcpy(history[i], history[i + 1]);
        }
        history_count = MAX_HISTORY - 1;
    }
    
    /* Add new command */
    strncpy(history[history_count], command, MAX_INPUT_LEN - 1);
    history[history_count][MAX_INPUT_LEN - 1] = '\0';
    history_count++;
}

/* Execute a single command */
void execute_command(command_t *cmd) {
    pid_t pid = fork();
    
    if (pid < 0) {
        /* Fork failed */
        print_error("fork", strerror(errno));
        return;
    } else if (pid == 0) {
        /* Child process */
        redirect_io(cmd);
        
        /* Execute command */
        execvp(cmd->args[0], cmd->args);
        
        /* If we get here, exec failed */
        print_error(cmd->args[0], strerror(errno));
        exit(1);
    } else {
        /* Parent process */
        foreground_pid = pid;
        
        if (cmd->background) {
            /* Add to job table and don't wait */
            add_job(pid, cmd->args[0]);
            last_exit_code = 0;  /* Background jobs start successfully */
        } else {
            /* Wait for foreground command to complete */
            int status;
            waitpid(pid, &status, 0);
            foreground_pid = 0;
            
            /* Track exit code */
            if (WIFEXITED(status)) {
                last_exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                last_exit_code = 128 + WTERMSIG(status);
            } else {
                last_exit_code = 1;
            }
        }
    }
}

/* Execute pipeline of commands */
void execute_pipeline(command_t *commands, int num_commands) {
    pid_t pids[num_commands];
    int pipes[num_commands - 1][2];
    
    /* Create pipes */
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            print_error("pipe", strerror(errno));
            return;
        }
    }
    
    /* Fork processes for each command */
    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            print_error("fork", strerror(errno));
            return;
        } else if (pids[i] == 0) {
            /* Child process */
            
            /* Redirect stdin if not first command */
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }
            
            /* Redirect stdout if not last command */
            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            
            /* Close all pipes in child */
            for (int j = 0; j < num_commands - 1; j++) {
                if (j != i - 1 && j != i) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }
            
            /* Handle I/O redirection for this command */
            redirect_io(&commands[i]);
            
            /* Execute command */
            execvp(commands[i].args[0], commands[i].args);
            print_error(commands[i].args[0], strerror(errno));
            exit(1);
        }
    }
    
    /* Parent process - close all pipes */
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    /* Wait for all processes to complete */
    int pipeline_status = 0;
    for (int i = 0; i < num_commands; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        
        /* Use the last command's exit status for the pipeline */
        if (i == num_commands - 1) {
            if (WIFEXITED(status)) {
                pipeline_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                pipeline_status = 128 + WTERMSIG(status);
            } else {
                pipeline_status = 1;
            }
        }
    }
    
    last_exit_code = pipeline_status;
}

/* Handle I/O redirection for a command */
void redirect_io(command_t *cmd) {
    /* Input redirection */
    if (cmd->input_file != NULL) {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd == -1) {
            print_error(cmd->input_file, strerror(errno));
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    
    /* Output redirection (truncate) */
    if (cmd->output_file != NULL) {
        int fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            print_error(cmd->output_file, strerror(errno));
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    
    /* Output redirection (append) */
    if (cmd->append_file != NULL) {
        int fd = open(cmd->append_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            print_error(cmd->append_file, strerror(errno));
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

/* Print shell prompt with current directory and exit status */
void print_prompt(void) {
    char cwd[MAX_PATH_LEN];
    char *abbrev_path;
    
    /* Get current working directory */
    if (getcwd(cwd, MAX_PATH_LEN) == NULL) {
        printf("\033[2m[tinsh\033[0m \033[1munknown\033[0m\033[2m]\033[0m$ ");
        return;
    }
    
    /* Abbreviate home directory as ~ */
    abbrev_path = abbreviate_path(cwd);
    
    /* Print prompt with ANSI colors - $ color depends on last exit code */
    printf("\033[2m[tinsh\033[0m \033[1m%s\033[0m\033[2m]\033[0m", abbrev_path);
    
    if (last_exit_code == 0) {
        printf("%s$%s ", COLOR_WHITE, COLOR_RESET);
    } else {
        printf("%s$%s ", COLOR_RED, COLOR_RESET);
    }
    
    fflush(stdout);
    
    free(abbrev_path);
}

/* Convert absolute path to tilde-abbreviated form */
char *abbreviate_path(const char *path) {
    char *home = getenv("HOME");
    char *abbrev = malloc(strlen(path) + 1);
    
    if (home != NULL && strncmp(path, home, strlen(home)) == 0) {
        strcpy(abbrev, "~");
        strcat(abbrev, path + strlen(home));
    } else {
        strcpy(abbrev, path);
    }
    
    return abbrev;
}

/* Print error message to stderr */
void print_error(const char *command, const char *reason) {
    fprintf(stderr, "tinsh: %s: %s\n", command, reason);
}

/* Cleanup and exit */
void cleanup_and_exit(int exit_code) {
    restore_terminal();
    printf("goodbye.\n");
    exit(exit_code);
}

/* Terminal handling functions */

/* Set terminal to raw mode for character-by-character input */
void set_raw_mode(void) {
    struct termios raw = orig_termios;
    
    /* Disable various features */
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(IXON);
    raw.c_cc[VMIN] = 1;   /* Read one character at a time */
    raw.c_cc[VTIME] = 0;  /* No timeout */
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/* Restore original terminal settings */
void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/* Handle backspace in raw mode */
void handle_backspace(void) {
    printf("\b \b");  /* Backspace, space, backspace */
    fflush(stdout);
}

/* Clear current line */
void clear_line(void) {
    printf("\r\033[K");  /* Carriage return and clear to end of line */
    fflush(stdout);
}

/* Enhanced read_input with arrow keys and tab completion */
char *read_input(void) {
    char input[MAX_INPUT_LEN] = {0};
    int pos = 0;
    int history_index = -1;
    
    set_raw_mode();
    
    while (1) {
        int c = getchar();
        
        if (c == EOF) {
            restore_terminal();
            printf("\n");
            return NULL;
        }
        
        if (c == ENTER) {
            printf("\n");
            restore_terminal();
            return strdup(input);
        }
        
        if (c == BACKSPACE) {
            if (pos > 0) {
                input[--pos] = '\0';
                handle_backspace();
            }
            continue;
        }
        
        if (c == ESCAPE) {
            /* Check for arrow keys */
            if (getchar() == '[') {
                int arrow = getchar();
                
                if (arrow == 'A') {  /* Up arrow */
                    if (history_count > 0 && history_index < history_count - 1) {
                        history_index++;
                        clear_line();
                        strcpy(input, history[history_count - 1 - history_index]);
                        pos = strlen(input);
                        printf("%s", input);
                        fflush(stdout);
                    }
                } else if (arrow == 'B') {  /* Down arrow */
                    if (history_index > 0) {
                        history_index--;
                        clear_line();
                        strcpy(input, history[history_count - 1 - history_index]);
                        pos = strlen(input);
                        printf("%s", input);
                        fflush(stdout);
                    } else if (history_index == 0) {
                        history_index--;
                        clear_line();
                        input[0] = '\0';
                        pos = 0;
                        fflush(stdout);
                    }
                }
            }
            continue;
        }
        
        if (c == TAB) {
            handle_tab_completion(input, &pos);
            continue;
        }
        
        /* Regular printable character */
        if (c >= 32 && c < 127 && pos < MAX_INPUT_LEN - 1) {
            input[pos++] = c;
            input[pos] = '\0';
            printf("%c", c);
            fflush(stdout);
        }
    }
}

/* Built-in command implementations */

/* Change directory built-in */
int builtin_cd(char **args) {
    char *target_dir;
    
    if (args[1] == NULL) {
        /* No argument - use HOME */
        target_dir = getenv("HOME");
        if (target_dir == NULL) {
            print_error("cd", "HOME not set");
            last_exit_code = 1;
            return 1;
        }
    } else {
        target_dir = args[1];
    }
    
    /* Change directory - must run in parent process */
    if (chdir(target_dir) == -1) {
        print_error("cd", strerror(errno));
        last_exit_code = 1;
        return 1;
    }
    
    last_exit_code = 0;
    return 0;
}

/* Exit built-in */
int builtin_exit(char **args) {
    int exit_code = 0;
    
    if (args[1] != NULL) {
        exit_code = atoi(args[1]);
    }
    
    cleanup_and_exit(exit_code);
    return 0;  /* Never reached */
}

/* Print working directory built-in */
int builtin_pwd(char **args __attribute__((unused))) {
    char cwd[MAX_PATH_LEN];
    
    if (getcwd(cwd, MAX_PATH_LEN) == NULL) {
        print_error("pwd", strerror(errno));
        last_exit_code = 1;
        return 1;
    }
    
    printf("%s\n", cwd);
    last_exit_code = 0;
    return 0;
}

/* History built-in */
int builtin_history(char **args __attribute__((unused))) {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
    last_exit_code = 0;
    return 0;
}

/* Jobs built-in */
int builtin_jobs(char **args __attribute__((unused))) {
    bool found_jobs = false;
    
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id != 0 && jobs[i].running) {
            printf("[%d] %d %s\n", jobs[i].job_id, jobs[i].pid, jobs[i].command);
            found_jobs = true;
        }
    }
    
    if (!found_jobs) {
        printf("No background jobs.\n");
    }
    
    last_exit_code = 0;
    return 0;
}

/* Check if command is built-in */
bool is_builtin(char *cmd) {
    return (strcmp(cmd, "cd") == 0 ||
            strcmp(cmd, "exit") == 0 ||
            strcmp(cmd, "pwd") == 0 ||
            strcmp(cmd, "history") == 0 ||
            strcmp(cmd, "jobs") == 0);
}

/* Job management functions */

/* Add new background job */
int add_job(pid_t pid, const char *command) {
    /* Find empty slot */
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == 0) {
            jobs[i].job_id = next_job_id++;
            jobs[i].pid = pid;
            strncpy(jobs[i].command, command, MAX_INPUT_LEN - 1);
            jobs[i].command[MAX_INPUT_LEN - 1] = '\0';
            jobs[i].running = true;
            return jobs[i].job_id;
        }
    }
    
    print_error("jobs", "job table full");
    return -1;
}

/* Remove job from table */
void remove_job(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == job_id) {
            jobs[i].job_id = 0;
            jobs[i].pid = 0;
            jobs[i].command[0] = '\0';
            jobs[i].running = false;
            return;
        }
    }
}

/* Update status of background jobs */
void update_job_status(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id != 0 && jobs[i].running) {
            int status;
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
            
            if (result == jobs[i].pid) {
                /* Job finished */
                jobs[i].running = false;
                print_job_completion(jobs[i].job_id);
                remove_job(jobs[i].job_id);
            } else if (result == -1) {
                /* Error or job doesn't exist */
                jobs[i].running = false;
                remove_job(jobs[i].job_id);
            }
        }
    }
}

/* Print job completion message */
void print_job_completion(int job_id) {
    /* Find the job */
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == job_id) {
            printf("\n[%d] %d Done %s\n", jobs[i].job_id, jobs[i].pid, jobs[i].command);
            return;
        }
    }
}

/* Signal handling functions */

/* Setup signal handlers */
void setup_signal_handlers(void) {
    struct sigaction sa_int, sa_tstp, sa_chld;
    
    /* SIGINT handler (Ctrl+C) */
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);
    
    /* SIGTSTP handler (Ctrl+Z) */
    sa_tstp.sa_handler = handle_sigtstp;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);
    
    /* SIGCHLD handler (child process termination) */
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);
}

/* Handle SIGINT (Ctrl+C) */
void handle_sigint(int sig __attribute__((unused))) {
    if (foreground_pid != 0) {
        /* Kill foreground process */
        kill(foreground_pid, SIGINT);
        foreground_pid = 0;
    } else {
        /* No foreground process - restore terminal and show new prompt */
        restore_terminal();
        printf("^C\n");
        print_prompt();
        fflush(stdout);
        set_raw_mode();
    }
}

/* Handle SIGTSTP (Ctrl+Z) */
void handle_sigtstp(int sig __attribute__((unused))) {
    if (foreground_pid != 0) {
        /* Suspend foreground process */
        kill(foreground_pid, SIGTSTP);
        foreground_pid = 0;
    }
    /* Don't suspend the shell itself */
}

/* Handle SIGCHLD (child process termination) */
void handle_sigchld(int sig __attribute__((unused))) {
    /* This handler just ensures we don't become zombies */
    /* Actual job cleanup happens in update_job_status() */
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        /* Reap zombie processes */
    }
}

/* Tab completion functions */

/* Complete commands from PATH */
char **complete_command(const char *token, int *num_matches) {
    static char *matches[MAX_COMPLETIONS];
    char *path_copy = strdup(getenv("PATH"));
    char *dir = strtok(path_copy, ":");
    *num_matches = 0;
    
    while (dir != NULL && *num_matches < MAX_COMPLETIONS - 1) {
        DIR *d = opendir(dir);
        if (d != NULL) {
            struct dirent *entry;
            while ((entry = readdir(d)) != NULL && *num_matches < MAX_COMPLETIONS - 1) {
                if (strncmp(entry->d_name, token, strlen(token)) == 0) {
                    /* Check if it's executable */
                    char full_path[MAX_PATH_LEN];
                    snprintf(full_path, MAX_PATH_LEN, "%s/%s", dir, entry->d_name);
                    if (access(full_path, X_OK) == 0) {
                        matches[*num_matches] = strdup(entry->d_name);
                        (*num_matches)++;
                    }
                }
            }
            closedir(d);
        }
        dir = strtok(NULL, ":");
    }
    
    matches[*num_matches] = NULL;
    free(path_copy);
    return matches;
}

/* Complete files in current directory */
char **complete_files(const char *token, int *num_matches) {
    static char *matches[MAX_COMPLETIONS];
    DIR *d = opendir(".");
    *num_matches = 0;
    
    if (d != NULL) {
        struct dirent *entry;
        while ((entry = readdir(d)) != NULL && *num_matches < MAX_COMPLETIONS - 1) {
            if (strncmp(entry->d_name, token, strlen(token)) == 0) {
                matches[*num_matches] = strdup(entry->d_name);
                (*num_matches)++;
            }
        }
        closedir(d);
    }
    
    matches[*num_matches] = NULL;
    return matches;
}

/* Handle tab completion */
void handle_tab_completion(char *input, int *cursor_pos) {
    /* Find the current token to complete */
    int token_start = *cursor_pos;
    while (token_start > 0 && input[token_start - 1] != ' ' && input[token_start - 1] != '\t') {
        token_start--;
    }
    
    char token[MAX_INPUT_LEN];
    strncpy(token, &input[token_start], *cursor_pos - token_start);
    token[*cursor_pos - token_start] = '\0';
    
    /* Try command completion first (if at start of line) */
    char **matches;
    int num_matches;
    
    if (token_start == 0) {
        matches = complete_command(token, &num_matches);
    } else {
        matches = complete_files(token, &num_matches);
    }
    
    if (num_matches == 0) {
        /* No matches - do nothing */
        return;
    } else if (num_matches == 1) {
        /* Single match - complete it */
        int completion_len = strlen(matches[0]) - strlen(token);
        if (*cursor_pos + completion_len < MAX_INPUT_LEN - 1) {
            strcat(input, matches[0] + strlen(token));
            printf("%s", matches[0] + strlen(token));
            *cursor_pos += completion_len;
            fflush(stdout);
        }
    } else {
        /* Multiple matches - show them */
        printf("\n");
        for (int i = 0; i < num_matches; i++) {
            printf("%s  ", matches[i]);
        }
        printf("\n");
        
        /* Reprint prompt and current input */
        print_prompt();
        printf("%s", input);
        fflush(stdout);
    }
    
    /* Free matches */
    for (int i = 0; i < num_matches; i++) {
        free(matches[i]);
    }
}
