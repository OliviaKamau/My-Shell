#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // need it for isatty()
#include <fcntl.h>
#include <ctype.h>

#define MAX_LINE 1024   // Maximum length for input command
#define READ_CHUNK_SIZE 128

// Function prototypes
char *read_input(FILE *source);                                         // Reads one full line of input
char **token_input(const char *line, int *token_count);                 // Splits the line into tokens, separating on spaces and <, >, |
int handle_redirection(char **tokens, char **infile, char **outfile);   // Identifies and sets up < and > redirection, removes the tokens
int execute_command(char **tokens, char *infile, char *outfile);        // Decides if the command is built-in or external, applies redirection, forks, and executes
int execute_pipeline(char **left_cmd, char **right_cmd);                // Executes two commands connected by a pipe

// Built-in Commands
int is_builtin_command(const char *cmd);    // Checks for built-in commands (cd, pwd, exit, die)
int execute_builtin(char **tokens);         // Executes built-in commands




int main(int argc, char *argv[]) {
    
    FILE *input_source = stdin;     // file pointer to read input (defaults to stdin but can later point to a file if batch script is provided)
    char *line;                     // stores the full line of input read from the user or file
    char **tokens;                  // stores the array of parsed words from line
    int token_count;                // tells us how many tokens were found in that line


    // checks if we're running in interactive mode (i.e. reading from the terminal)
    int interactive = isatty(fileno(stdin));        // returns 1 if reading from terminal, returns 0 if reading from a file or pipe
                                                    // this is what tells the shell whether we should print the prompts/greetings for interactive mode

    // validates the command-line arguments (expects to receive 0 or 1 from isatty above, prints an error mssg and exits if it receives any other number)
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [batch_file]\n", argv[0]);       // N/B: argv[0] is the program name (to make the error message more clear)
        exit(EXIT_FAILURE);
    }

    // if there's one argument, it will assume that it's a batch file
    if (argc == 2) {                            // switches the shell to batch mode
        input_source = fopen(argv[1], "r");     // opens the file for reading "r"
        if (!input_source) {                    // if the file fails to open, it prints an error message and exits
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        // since we're reading from a file, we're in batch mode, not interactive mode. so we have to set interactive to 0
        interactive = 0;        // setting it to 0 makes sure that it doesn't print the prompts for interactive mode even if the batch file is run from the terminal
    }

    // if we're in interactive mode, print the greeting message
    if (interactive) {
        printf("Welcome to myt shell!\n");
    }


    // Main loop (it will run infinitely until explicity broken by exit, die or Ctrl + D)
    while (1) {      
        if (interactive) {          // if it detects that we're in interactive mode, it prints the shell prompt before each input
            printf("mysh> ");
            fflush(stdout);         // this just makes sure that the prompt prints immediately (even when the terminal buffers the output)
        }   

        line = read_input(stdin);       // calls the read_input helper function to read a full line of user input from stdin (i.e. reading from the keyboard)
                                        // it will return NULL if the user tries to exit the shell e.g. by hitting Ctrl + D
        if (line == NULL) {
            break;                      // Breaks the infinite loop and exits the shell if there's no input
        }

        tokens = token_input(line, &token_count);       /* splits the line into words and operators.
                                                            It returns an array of strings (tokens), and stores how many there are in token_count */


        // this skips empty lines (e.g. when the user just hits enter without typing anything)                                                   
        if (token_count == 0) {
            free(line);
            free(tokens);
            continue;       // frees the memory and goes to the next prompt
        }


        /*

            will add redirection, built-ins, fork/exec functions here later

        
        */

        // checks if the first word is "exit" or "die" (the built-in commands to terminate the shell)
        if (strcmp(tokens[0], "exit") == 0 || strcmp(tokens[0], "die") == 0) {
            free(line);
            free(tokens);
            break;          // frees the memory and breaks out of the loop
        }

        // clean up after each loop iteration to avoid memory leaks
        free(line);
        free(tokens);
    }

    // prints the goodbye message (for interactive mode)
    if(interactive) {
        printf("Exiting my shell.\n");
    }

    // closes any file that was opened earlier (for batch mode)
    if (argc == 2) {
        fclose(input_source);
    }

    return 0;       // return success to the OS
}






// read_input() function (reads a line of input from the given file stream)
char *read_input(FILE *source) {
    static char leftover[READ_CHUNK_SIZE];  // Buffer to store leftover bytes from previous read
    static size_t leftover_size = 0;        // Number of leftover bytes
    static int eof_reached = 0;             // Flag to indicate if EOF was reached
    static FILE *last_file = NULL;          // Track the last file we were reading from
    
    // Reset static variables if we're reading from a different file
    if (last_file != source) {
        leftover_size = 0;
        eof_reached = 0;
        last_file = source;
    }
    
    int fd = fileno(source);                // Convert FILE* to file descriptor
    if (fd == -1) return NULL;
    
    char *buffer = NULL;                    // Result buffer for the line
    size_t size = 0;                        // Current size of the result
    size_t capacity = 0;                    // Allocated capacity
    int found_newline = 0;                  // Flag to indicate if we found a newline
    
    // If we've reached EOF and have no leftover data, return NULL immediately
    if (eof_reached && leftover_size == 0) {
        return NULL;
    }
    
    // If we have leftover data from previous read, process it first
    if (leftover_size > 0) {
        size_t i;
        for (i = 0; i < leftover_size; i++) {
            // Resize buffer if needed
            if (size + 1 >= capacity) {
                capacity = capacity == 0 ? READ_CHUNK_SIZE : capacity * 2;
                buffer = realloc(buffer, capacity);
                if (!buffer) {
                    perror("realloc");
                    return NULL;
                }
            }
            
            // Add byte to result buffer
            buffer[size++] = leftover[i];
            
            // Check for newline
            if (leftover[i] == '\n') {
                found_newline = 1;
                i++; // Move past the newline
                break;
            }
        }
        
        // Update leftover buffer
        if (i < leftover_size) {
            // We still have leftover data
            leftover_size -= i;
            memmove(leftover, &leftover[i], leftover_size);
        } else {
            // We've used all leftover data
            leftover_size = 0;
        }
        
        if (found_newline) {
            goto finish_line;  // We found a complete line in the leftover data
        }
    }
    
    // If we haven't found a newline yet, read more data
    while (!found_newline && !eof_reached) {
        char chunk[READ_CHUNK_SIZE];
        ssize_t bytes_read = read(fd, chunk, READ_CHUNK_SIZE);
        printf("Bytes read: %ld\n", bytes_read);
        
        if (bytes_read < 0) {
            perror("read");
            free(buffer);
            return NULL;
        } else if (bytes_read == 0) {
            eof_reached = 1;
            break;  // EOF reached
        }
        
        // Process the chunk byte by byte
        size_t i;
        for (i = 0; i < bytes_read; i++) {
            // Resize buffer if needed
            if (size + 1 >= capacity) {
                capacity = capacity == 0 ? READ_CHUNK_SIZE : capacity * 2;
                buffer = realloc(buffer, capacity);
                if (!buffer) {
                    perror("realloc");
                    return NULL;
                }
            }
            
            buffer[size++] = chunk[i];
            
            if (chunk[i] == '\n') {
                found_newline = 1;
                i++; // Move past the newline
                break;
            }
        }
        
        // Save any leftover bytes for the next call
        if (i < bytes_read) {
            leftover_size = bytes_read - i;
            memcpy(leftover, &chunk[i], leftover_size);
        }
    }
    
    // If we reached EOF without any data, return NULL
    if (size == 0 && eof_reached) {
        free(buffer);
        return NULL;
    }
    
finish_line:
    // Null-terminate the result
    if (buffer) {
        // Ensure we have space for null terminator
        if (size >= capacity) {
            buffer = realloc(buffer, size + 1);
            if (!buffer) {
                perror("realloc");
                return NULL;
            }
        }
        
        buffer[size] = '\0';
        
        // Remove trailing newline if present
        if (size > 0 && buffer[size - 1] == '\n') {
            buffer[size - 1] = '\0';
        }
    }
    
    return buffer;
}



/*
---------------------------------------- TEST MAIN ------------------------------
// the testbed main that i was using to test the read_input function:
int main() {
    FILE *file = fopen("testfile.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    char *line;
    while ((line = read_input(file)) != NULL) {
        printf("Read from file: %s\n", line);
        free(line); // remember to free each line after you're done
    }

    fclose(file);
    return 0;
}




*/