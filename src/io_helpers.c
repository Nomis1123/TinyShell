#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "variables.h"
#include "io_helpers.h"
#include <ctype.h>


// ===== Output helpers =====

/* Prereq: str is a NULL terminated string
 */
void display_message(char *str) {
    write(STDOUT_FILENO, str, strnlen(str, MAX_STR_LEN));
}


/* Prereq: pre_str, str are NULL terminated string
 */
void display_error(const char *pre_str, const char *str) {
    write(STDERR_FILENO, pre_str, strnlen(pre_str, MAX_STR_LEN));
    write(STDERR_FILENO, str, strnlen(str, MAX_STR_LEN));
    write(STDERR_FILENO, "\n", 1);
}


// ===== Input tokenizing =====

/* Prereq: in_ptr points to a character buffer of size > MAX_STR_LEN
 * Return: number of bytes read
 */
ssize_t get_input(char *in_ptr) {
    int retval = read(STDIN_FILENO, in_ptr, MAX_STR_LEN+1); // Not a sanitizer issue since in_ptr is allocated as MAX_STR_LEN+1
    int read_len = retval;
    if (retval == -1) {
        read_len = 0;
    }
    if (read_len > MAX_STR_LEN) {
        read_len = 0;
        retval = -1;
        write(STDERR_FILENO, "ERROR: input line too long\n", strlen("ERROR: input line too long\n"));
        int junk = 0;
        while((junk = getchar()) != EOF && junk != '\n');
    }
    in_ptr[read_len] = '\0';
    return retval;
}

/* Prereq: in_ptr is a string, tokens is of size >= len(in_ptr)
 * Warning: in_ptr is modified
 * Return: number of tokens.
 */
size_t tokenize_input(char *in_ptr, char **tokens) {
    // TODO: Remove unused attribute
    char *curr_ptr = strtok (in_ptr, DELIMITERS);
    size_t token_count = 0;

    while (curr_ptr != NULL) {  // TODO: Fix this
        // TODO: Fix this
        
        tokens[token_count] = curr_ptr; // Store token in the array
        token_count++;
        curr_ptr = strtok(NULL, DELIMITERS); 
    }
    tokens[token_count] = NULL;
    return token_count;
}

char *expand_variables(const char *token) {
    char *result = malloc(MAX_STR_LEN + 1);
    if (!result) return NULL;

    size_t len = 0;
    const char *curr = token;

    while (*curr && len < MAX_STR_LEN) {
        if (*curr == '$') {
            curr++;
            char var_name[MAX_STR_LEN] = {0};
            size_t var_len = 0;

            // Handle ${VAR} syntax
            if (*curr == '{') {
                curr++;
                while (*curr && *curr != '}' && var_len < MAX_STR_LEN - 1) {
                    var_name[var_len++] = *curr++;
                }
                if (*curr == '}') curr++;
            } else {
                // Handle $VAR syntax
                while (isalnum(*curr) && var_len < MAX_STR_LEN - 1) {
                    var_name[var_len++] = *curr++;
                }
            }

            const char *value = get_variable(var_name);
            size_t val_len = strlen(value);
            if (len + val_len > MAX_STR_LEN) val_len = MAX_STR_LEN - len;
            strncpy(result + len, value, val_len);
            len += val_len;
        } else {
            result[len++] = *curr++;
        }
    }
    result[len] = '\0';
    return result;
}