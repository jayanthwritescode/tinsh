#ifndef TINSH_H
#define TINSH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <termios.h>
#include <dirent.h>

/* Constants */
#define MAX_INPUT_LEN 1024        /* Maximum length of user input */
#define MAX_ARGS 128              /* Maximum number of command arguments */
#define MAX_JOBS 32               /* Maximum number of background jobs */
#define MAX_HISTORY 20            /* Number of commands to keep in history */
#define MAX_PATH_LEN 4096         /* Maximum path length */
#define MAX_COMPLETIONS 100       /* Maximum number of tab completions */
#define MAX_COMMAND_GROUPS 32     /* Maximum number of command groups */
#define MAX_SUBSTITUTION_LEN 2048 /* Maximum length of command substitution output */

/* ANSI color codes */
#define COLOR_RED     "\033[1;31m"
#define COLOR_WHITE   "\033[1;37m"
#define COLOR_RESET   "\033[0m"
#define COLOR_DIM     "\033[2m"

/* Special key codes */
#define ESCAPE        27
#define BACKSPACE     127
#define TAB           9
#define ENTER         13

/* Logical operators for command groups */
typedef enum {
    OP_NONE,        /* No operator (last command) */
    OP_SEMICOLON,   /* ; - always execute next */
    OP_AND,         /* && - execute next if current succeeds */
    OP_OR           /* || - execute next if current fails */
} operator_t;

/* Job structure for background processes */
typedef struct {
    int job_id;                  /* Job ID number */
    pid_t pid;                   /* Process ID */
    char command[MAX_INPUT_LEN]; /* Command string */
    bool running;                /* Whether job is still running */
} job_t;

/* Command structure for parsed input */
typedef struct {
    char *args[MAX_ARGS];        /* Command arguments */
    int argc;                    /* Number of arguments */
    char *input_file;            /* Input redirection file */
    char *output_file;           /* Output redirection file */
    char *append_file;           /* Append output file */
    bool background;             /* Run in background */
    bool pipe_next;              /* Whether command pipes to next */
} command_t;

/* Command group structure for logical operators */
typedef struct {
    command_t *commands;         /* Array of commands (for pipelines) */
    int num_commands;            /* Number of commands in pipeline */
    operator_t operator;         /* Operator that follows this group */
} command_group_t;

/* Global variables */
extern job_t jobs[MAX_JOBS];
extern int next_job_id;
extern char history[MAX_HISTORY][MAX_INPUT_LEN];
extern int history_count;
extern volatile sig_atomic_t foreground_pid;
extern int last_exit_code;
extern struct termios orig_termios;
extern int history_position;

/* Function declarations */

/* Main shell functions */
void initialize_shell(void);
void shell_loop(void);
void cleanup_and_exit(int exit_code);

/* Input parsing */
char *read_input(void);
void parse_input(char *input, command_t *commands, int *num_commands);
void add_to_history(const char *command);
void parse_input_advanced(char *input, command_t *commands, int *num_commands);
void parse_command_groups(char *input, command_group_t *groups, int *num_groups);

/* Command substitution */
char *execute_command_substitution(const char *command);
char *expand_command_substitutions(const char *input);

/* Terminal handling */
void set_raw_mode(void);
void restore_terminal(void);
void handle_backspace(void);
void clear_line(void);

/* Tab completion */
char **complete_command(const char *token, int *num_matches);
char **complete_files(const char *token, int *num_matches);
void handle_tab_completion(char *input, int *cursor_pos);

/* Command execution */
void execute_command(command_t *cmd);
void execute_pipeline(command_t *commands, int num_commands);
void redirect_io(command_t *cmd);
void execute_command_groups(command_group_t *groups, int num_groups);

/* Built-in commands */
int builtin_cd(char **args);
int builtin_exit(char **args);
int builtin_pwd(char **args);
int builtin_history(char **args);
int builtin_jobs(char **args);
bool is_builtin(char *cmd);

/* Job management */
int add_job(pid_t pid, const char *command);
void remove_job(int job_id);
void update_job_status(void);
void print_job_completion(int job_id);

/* Signal handling */
void setup_signal_handlers(void);
void handle_sigint(int sig);
void handle_sigtstp(int sig);
void handle_sigchld(int sig);

/* Utility functions */
void print_prompt(void);
char *abbreviate_path(const char *path);
void print_error(const char *command, const char *reason);

#endif /* TINSH_H */
