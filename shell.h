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

/* Constants */
#define MAX_INPUT_LEN 1024        /* Maximum length of user input */
#define MAX_ARGS 128              /* Maximum number of command arguments */
#define MAX_JOBS 32               /* Maximum number of background jobs */
#define MAX_HISTORY 20            /* Number of commands to keep in history */
#define MAX_PATH_LEN 4096         /* Maximum path length */

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

/* Global variables */
extern job_t jobs[MAX_JOBS];
extern int next_job_id;
extern char history[MAX_HISTORY][MAX_INPUT_LEN];
extern int history_count;
extern volatile sig_atomic_t foreground_pid;

/* Function declarations */

/* Main shell functions */
void initialize_shell(void);
void shell_loop(void);
void cleanup_and_exit(int exit_code);

/* Input parsing */
char *read_input(void);
void parse_input(char *input, command_t *commands, int *num_commands);
void add_to_history(const char *command);

/* Command execution */
void execute_command(command_t *cmd);
void execute_pipeline(command_t *commands, int num_commands);
void redirect_io(command_t *cmd);

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
