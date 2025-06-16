#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "variables.h"
#include "builtins.h"
#include "io_helpers.h"

int main() {
    char *prompt = "mysh$ ";
    char input_buf[MAX_STR_LEN + 1];
    input_buf[MAX_STR_LEN] = '\0';
    char *token_arr[MAX_STR_LEN] = {NULL};

    while (1) {
        display_message(prompt);
        ssize_t ret = get_input(input_buf);
        size_t token_count = tokenize_input(input_buf, token_arr);

        // Exit conditions
        if ((token_count > 0 && strcmp(token_arr[0], "exit") == 0) ||
            (token_count == 0 && input_buf[0] != '\n' && ret == 0)) {
            break;
        }

        // variable assignments
        size_t assignments_processed = 0;
        while (assignments_processed < token_count) {
            char *token = token_arr[assignments_processed];
            char *equals = strchr(token, '=');

            if (equals && equals != token) {
                *equals = '\0';
                char *name = token;
                char *value = strdup(equals + 1); // Heap allocation

                // Concatenate multi-token values
                for (size_t i = assignments_processed + 1; i < token_count; i++) {
                    if (strchr(token_arr[i], '=') == NULL) {
                        size_t new_len = strlen(value) + strlen(token_arr[i]) + 2;
                        char *new_value = realloc(value, new_len);
                        if (!new_value) break;
                        value = new_value;
                        strcat(value, " ");
                        strcat(value, token_arr[i]);
                        assignments_processed++;
                    } else break;
                }

                
                char *expanded_value = expand_variables(value);
                set_variable(name, expanded_value);
                free(expanded_value);
                free(value);
                assignments_processed++;
            } else break;
        }

        // Process command
        if (assignments_processed < token_count) {
            char **cmd_tokens = token_arr + assignments_processed;
            size_t cmd_count = token_count - assignments_processed;

            // Expand variables in command tokens
            char *expanded[MAX_STR_LEN];
            for (size_t i = 0; i < cmd_count; i++) {
                expanded[i] = expand_variables(cmd_tokens[i]);
            }

            // Build and truncate expanded command
            char line[MAX_STR_LEN + 1] = {0};
            size_t len = 0;
            for (size_t i = 0; i < cmd_count && len < MAX_STR_LEN; i++) {
                const char *token = expanded[i];
                size_t token_len = strlen(token);
                if (i > 0 && len < MAX_STR_LEN) line[len++] = ' ';
                strncpy(line + len, token, MAX_STR_LEN - len);
                len = (len + token_len > MAX_STR_LEN) ? MAX_STR_LEN : len + token_len;
            }
            line[MAX_STR_LEN] = '\0';

            // Re-tokenize and execute
            char new_tokens[MAX_STR_LEN];
            strncpy(new_tokens, line, MAX_STR_LEN);
            char *new_token_arr[MAX_STR_LEN];
            size_t new_count = tokenize_input(new_tokens, new_token_arr);

            if (new_count > 0) {
                bn_ptr builtin = check_builtin(new_token_arr[0]);
                if (builtin) {
                    if (builtin(new_token_arr) == -1) {
                        display_error("ERROR: Builtin failed: ", new_token_arr[0]);
                    }
                } else {
                    display_error("ERROR: Unknown command: ", new_token_arr[0]);
                }
            }

            // Cleanup
            for (size_t i = 0; i < cmd_count; i++) free(expanded[i]);
        }
    }

    cleanup_variables();
    return 0;
}