#ifndef SIMPLE_SHELL_H
#define SIMPLE_SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_HISTORY 1000
#define HISTORY_FILE "history.txt"
#define ENV_FILE "env.txt"

// Global variables
extern char *history[MAX_HISTORY];
extern int history_count;
extern char *prompt;

// Function prototypes
char *read_input();
char **parse_input(char *input);
void signal_handler();
void shell_cd(char **args);
void shell_echo(char **args);
void shell_clear();
void shell_pwd();
void shell_setenv(char **args);
void shell_unsetenv(char **args);
void shell_history();
void shell_chprompt(char **args);
void shell_help();
void handle_input(char *input);
int execute_command(char **args);
void add_to_history(char *command);
void log_history_to_file();
void log_env_change_to_file(const char *operation, const char *var, const char *value);
void shell_loop();

#endif //SIMPLE_SHELL_H