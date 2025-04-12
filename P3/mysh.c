#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_LINE 1024   // Maximum length for input command

// Function prototypes
// Reads one full line of input
char *read_input(FILE *source); 
// Splits the line into tokens, separating on spaces and <, >, |
char **token_input(const char *line, int *token_count); 
// Identifies and sets up < and > redirection, removes the tokens
int handle_redirection(char **tokens, char **infile, char **outfile); 
// Decides if the command is built-in or external, applies redirection, forks, and executes
int execute_command(char **tokens, char *infile, char *outfile); 
// Executes two commands connected by a pipe
int execute_pipeline(char **left_cmd, char **right_cmd);
// Built-in Commands
int is_builtin_command(const char *cmd); // Checks for built-in commands (cd, pwd, exit, die)
int execute_builtin(char **tokens); // Executes built-in commands