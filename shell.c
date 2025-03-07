#include "simple_shell.h"

// Global variables
char *history[MAX_HISTORY];
int history_count = 0;
char *prompt = "shell> ";


// Read input from the user
char *read_input() {
    char *input = NULL;
    size_t bufsize = 0;
    getline(&input, &bufsize, stdin);
    input[strlen(input) - 1] = '\0'; // Remove newline
    return input;
}

// Split input into arguments
char **parse_input(char *input) {
    int bufsize = MAX_ARGS, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    token = strtok(input, " ");
    while (token != NULL) {
        tokens[position++] = token;
        token = strtok(NULL, " ");
    }
    tokens[position] = NULL;
    return tokens;
}

// Add command to history and log to file
void add_to_history(char *command) {
    if (history_count == MAX_HISTORY) {
        free(history[0]);
        for (int i = 1; i < MAX_HISTORY; i++) {
            history[i - 1] = history[i];
        }
        history_count--;
    }
    history[history_count++] = strdup(command);
    log_history_to_file();
}

// Log history to history.txt
void log_history_to_file() {
    FILE *file = fopen(HISTORY_FILE, "w");
    if (file == NULL) {
        perror("Error opening history.txt");
        return;
    }
    for (int i = 0; i < history_count; i++) {
        fprintf(file, "%d %s\n", i + 1, history[i]);
    }

    fclose(file);
}

// Log environment changes to env.txt
void log_env_change_to_file(const char *operation, const char *var, const char *value) {
    FILE *file = fopen(ENV_FILE, "a"); // Append mode
    if (file == NULL) {
        perror("Error opening env.txt");
        return;
    }
    if (strcmp(operation, "set") == 0) {
        fprintf(file, "SET %s=%s\n", var, value);
    } else if (strcmp(operation, "unset") == 0) {
        fprintf(file, "UNSET %s\n", var); //change
    }
    fclose(file);
}

// Built-in commands
void shell_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("shell");
        }
    }
}

void signal_handler(int signum) {
        printf("Caught Ctrl+C (or exit command). Exiting the shell...\n");
        fflush(stdout);
        exit(1);  // Exit the shell gracefully
}

void shell_echo(char **args) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s ", args[i]);
    }
    printf("\n");  //add $
}

void shell_clear() {
    printf("\033[H\033[J");
}

void shell_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("shell");
    }
}

void shell_setenv(char **args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, "Usage: setenv VAR VALUE\n");
    } else {
        if (setenv(args[1], args[2], 1) != 0) {
            perror("shell");
        } else {
            log_env_change_to_file("set", args[1], args[2]);
        }
    }
}

void shell_unsetenv(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: unsetenv VAR\n");
    } else {
        if (unsetenv(args[1]) != 0) {
            perror("shell");
        } else {
            log_env_change_to_file("unset", args[1], NULL);
        }
    }
}

void shell_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
    
}

void shell_chprompt(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: chprompt NEW_PROMPT\n");
    } else {
        prompt = malloc(strlen(args[1]) + 3);
        if (prompt == NULL) {
            perror("Error allocating memory");
            return;
        }
        strcpy(prompt, args[1]);
        strcat(prompt, "> ");

    }
}

void shell_help() {
    printf("Available commands:\n");
    printf("cd <directory>: Change directory\n");
    printf("pwd: Print current working directory\n");
    printf("echo <text>: Print text\n");
    printf("setenv <var> <value>: Set environment variable\n");
    printf("unsetenv <var>: Unset envirnoment variable\n");
    printf("exit: Exit the shell\n");
    printf("chprompt <prompt>: Change shell prompt\n");
    printf("history: Show command history\n");
    printf("clear: Clear the terminal screen\n");
    return;
}



// Execute external commands, redirecting failed commands to /dev/null
int execute_command(char **args) {
    pid_t pid, wpid;
    int status;
    int dev_null;

    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // Child process
        dev_null = open("/dev/null", O_WRONLY); // Open /dev/null
        if (dev_null == -1) {
            perror("Error opening /dev/null");
            exit(EXIT_FAILURE);
        }

        if (execvp(args[0], args) == -1) {
            perror("Command execution failed");
           // Redirect both stdout and stderr to /dev/null
            dup2(dev_null, STDOUT_FILENO);
            dup2(dev_null, STDERR_FILENO);
            close(dev_null); // Close the file descriptor after redirection
            exit(EXIT_FAILURE); // Exit after failure
            
        } else {
            close(dev_null);
        }
    }
    else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0; // Return if the command was successful
}
// Handle input and execute commands
void handle_input(char *input) {
      if (input == NULL || strlen(input) == 0) {
        return;  // Ignore empty input
    }
    
    // Add to history before processing
    char **args = parse_input(input);
    
    if (args[0] == NULL) {
        // Empty command
        free(args);
        return;
    }

    // Check for built-in commands
    if (strcmp(args[0], "cd") == 0) {
        add_to_history(input);
        shell_cd(args);
    } else if (strcmp(args[0], "exit") == 0) {
        add_to_history(input);
        raise(SIGINT);
    } else if (strcmp(args[0], "echo") == 0) {
        add_to_history(input);
        shell_echo(args);
    } else if (strcmp(args[0], "clear") == 0) {
        add_to_history(input);
        shell_clear();
    } else if (strcmp(args[0], "pwd") == 0) {
        add_to_history(input);
        shell_pwd();
    } else if (strcmp(args[0], "setenv") == 0) {
        add_to_history(input);
        shell_setenv(args);
    } else if (strcmp(args[0], "unsetenv") == 0) {
        add_to_history(input);
        shell_unsetenv(args);
    } else if (strcmp(args[0], "history") == 0) {
        add_to_history(input);
        shell_history();
    } else if (strcmp(args[0], "chprompt") == 0) {
        add_to_history(input);
        shell_chprompt(args);
    } else if (strcmp(args[0], "help") == 0) {
       add_to_history(input);
        shell_help();
    } else {
        // Execute external command and add to history only if valid
        if (execute_command(args)){
            add_to_history(input);
        }
    }
    free(args);
}
 
 // Main shell loop
 void shell_loop() {
    char *input;
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        input = read_input();
        handle_input(input);
        free(input);
    }
 }