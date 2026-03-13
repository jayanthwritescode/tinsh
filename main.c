/*
 * tinsh - Tiny Shell
 * A minimal Unix shell implementation in C
 * 
 * Main entry point for the shell program
 */

#include "shell.h"

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    /* Initialize shell state and signal handlers */
    initialize_shell();
    
    /* Run the main shell loop */
    shell_loop();
    
    /* Should never reach here */
    return 0;
}
