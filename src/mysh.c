#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>
#include "variables.h"
#include "builtins.h"
#include "io_helpers.h"

#define MAX_BG_JOBS 100
#define MAX_STR_LEN 128
#define MAX_CLIENTS 100


struct termios termios_save;



BackgroundJob bg_jobs[MAX_BG_JOBS]; 
static int next_job_id = 1;
sig_atomic_t the_flag=0;

int check_pipe(char *cline, char **lline, char **rline) {
    for (size_t i = 0; i < strlen(cline); i++) {
        if (cline[i] == '|') {
            cline[i] = '\0';
            *lline = cline;
            *rline = cline + i + 1;
            return 1;
        }
    }
    *lline = cline;
    *rline = NULL;
    return 0;
}

void reset_the_terminal(void) {
    tcsetattr(0, 0, &termios_save);
}

void sigint_handler(int sig) {
    if (sig == SIGINT) {
        the_flag = 1;
        char *prompt = "\nmysh$ ";
        display_message(prompt);
    }
}

int add_background_job(pid_t pid, const char *command) {
    for (int i = 0; i < MAX_BG_JOBS; i++) {
        if (bg_jobs[i].pid == 0) {
            bg_jobs[i].job_id = next_job_id++;
            bg_jobs[i].pid = pid;
            strncpy(bg_jobs[i].command, command, MAX_STR_LEN - 1);
            bg_jobs[i].command[MAX_STR_LEN - 1] = '\0';
            return bg_jobs[i].job_id;
        }
    }
    return -1;
}

void check_background_jobs() {
    for (int i = 0; i < MAX_BG_JOBS; i++) {
        if (bg_jobs[i].pid != 0) {
            int status;
            pid_t result = waitpid(bg_jobs[i].pid, &status, WNOHANG);
            if (result > 0) {
                char msg[MAX_STR_LEN];
                // Truncate command to 100 chars to avoid overflow (PID + formatting uses ~20 chars)
                snprintf(msg, sizeof(msg), "[%d]+  Done %.100s\n", bg_jobs[i].job_id, bg_jobs[i].command);
                display_message(msg);
                bg_jobs[i].pid = 0;
            }
        }
    }
}

ssize_t cleanup_server(char **tokens) {
    (void)tokens; // Unused
    for (int i = 0; i < MAX_BG_JOBS; i++) {
        if (bg_jobs[i].pid != 0 && strstr(bg_jobs[i].command, "start-server")) {
            kill(bg_jobs[i].pid, SIGTERM);
            waitpid(bg_jobs[i].pid, NULL, 0);
            bg_jobs[i].pid = 0;
            display_message("Server terminated\n");
            return 0;
        }
    }
    
    return -1;
}

int main() {
    signal(SIGINT, sigint_handler);

    char *prompt = "mysh$ ";
    char input_buf[MAX_STR_LEN + 1];
    input_buf[MAX_STR_LEN] = '\0';
    char *token_arr[MAX_STR_LEN] = {NULL};

    while (1) {
        check_background_jobs();
        display_message(prompt);
        ssize_t ret = get_input(input_buf);

        size_t token_count = tokenize_input(input_buf, token_arr);

        int background = 0;
        if (token_count > 0 && strcmp(token_arr[token_count - 1], "&") == 0) {
            background = 1;
            token_arr[token_count - 1] = NULL;
            token_count--;
        }

        if ((token_count > 0 && strcmp(token_arr[0], "exit") == 0) ||
            (token_count == 0 && input_buf[0] != '\n' && ret == 0)) {
            break;
        }

        size_t assignments_processed = 0;
        while (assignments_processed < token_count) {
            char *token = token_arr[assignments_processed];
            char *equals = strchr(token, '=');

            if (equals && equals != token) {
                *equals = '\0';
                char *name = token;
                char *value = strdup(equals + 1);

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
                if (expanded_value == NULL) {
                    display_error("ERROR: Memory allocation failed", "");
                    free(value);
                    continue;
                }
                set_variable(name, expanded_value);
                free(expanded_value);
                free(value);
                assignments_processed++;
            } else break;
        }

        if (assignments_processed < token_count) {
            char **cmd_tokens = token_arr + assignments_processed;
            size_t cmd_count = token_count - assignments_processed;

            char *expanded[MAX_STR_LEN];
            for (size_t i = 0; i < cmd_count; i++) {
                expanded[i] = expand_variables(cmd_tokens[i]);
            }

            char line[MAX_STR_LEN + 1] = {0};
            size_t len = 0;
            for (size_t i = 0; i < cmd_count && len < MAX_STR_LEN; i++) {
                if (expanded[i] == NULL) continue;
                if (i > 0 && len < MAX_STR_LEN) line[len++] = ' ';
                strncpy(line + len, expanded[i], MAX_STR_LEN - len);
                len += strnlen(expanded[i], MAX_STR_LEN - len);
            }
            line[MAX_STR_LEN] = '\0';

            char *left = NULL;
            char *right = NULL;
            int is_piped = check_pipe(line, &left, &right);

            if (!is_piped) {
                char *expanded_tokens[cmd_count + 1];
                for (size_t i = 0; i < cmd_count; i++) {
                    expanded_tokens[i] = expanded[i];
                }
                expanded_tokens[cmd_count] = NULL;

                bn_ptr builtin = check_builtin(expanded_tokens[0]);
                if (builtin) {
                    if (builtin(expanded_tokens) == -1) {
                        display_error("ERROR: Builtin failed: ", expanded_tokens[0]);
                    }
                    for (size_t i = 0; i < cmd_count; i++) free(expanded[i]);
                    continue;
                }

                pid_t child_pid = fork();
                if (child_pid == -1) {
                    display_error("ERROR: Fork failed", "");
                    for (size_t i = 0; i < cmd_count; i++) free(expanded[i]);
                    continue;
                }

                if (child_pid == 0) {
                    char cmd[MAX_STR_LEN] = {0};
                    strncat(cmd, "/bin/", MAX_STR_LEN - 1);
                    strncat(cmd, expanded_tokens[0], MAX_STR_LEN - strlen(cmd) - 1);

                    execv(cmd, expanded_tokens);
                    display_error("ERROR: Unknown command: ", expanded_tokens[0]);
                    _exit(EXIT_FAILURE);
                } else {
                    if (background) {
                        int job_id = add_background_job(child_pid, line);
                        if (job_id != -1) {
                            char msg[MAX_STR_LEN];
                            snprintf(msg, sizeof(msg), "[%d] %d\n", job_id, child_pid);
                            display_message(msg);
                        }
                    } else {
                        int status;
                        waitpid(child_pid, &status, 0);
                    }
                }
            } else {
                int pipefd[2];
                if (pipe(pipefd) == -1) {
                    display_error("ERROR: Pipe creation failed", "");
                    continue;
                }

                pid_t child1 = fork();
                if (child1 == 0) {
                    close(pipefd[0]);
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[1]);

                    char left_tokens[MAX_STR_LEN];
                    strncpy(left_tokens, left, MAX_STR_LEN);
                    char *left_token_arr[MAX_STR_LEN];
                    size_t left_count = tokenize_input(left_tokens, left_token_arr);

                    if (left_count > 0) {
                        bn_ptr builtin = check_builtin(left_token_arr[0]);
                        if (builtin) {
                            builtin(left_token_arr);
                            _exit(EXIT_SUCCESS);
                        }
                        char cmd[MAX_STR_LEN] = {0};
                        strncat(cmd, "/bin/", MAX_STR_LEN - 1);
                        strncat(cmd, left_token_arr[0], MAX_STR_LEN - strlen(cmd) - 1);
                        execv(cmd, left_token_arr);
                        display_error("ERROR: Unknown command: ", left_token_arr[0]);
                    }
                    _exit(EXIT_FAILURE);
                }

                pid_t child2 = fork();
                if (child2 == 0) {
                    close(pipefd[1]);
                    dup2(pipefd[0], STDIN_FILENO);
                    close(pipefd[0]);

                    char right_tokens[MAX_STR_LEN];
                    strncpy(right_tokens, right, MAX_STR_LEN);
                    char *right_token_arr[MAX_STR_LEN];
                    size_t right_count = tokenize_input(right_tokens, right_token_arr);

                    if (right_count > 0) {
                        bn_ptr builtin = check_builtin(right_token_arr[0]);
                        if (builtin) {
                            builtin(right_token_arr);
                            _exit(EXIT_SUCCESS);
                        }
                        char cmd[MAX_STR_LEN] = {0};
                        strncat(cmd, "/bin/", MAX_STR_LEN - 1);
                        strncat(cmd, right_token_arr[0], MAX_STR_LEN - strlen(cmd) - 1);
                        execv(cmd, right_token_arr);
                        display_error("ERROR: Unknown command: ", right_token_arr[0]);
                    }
                    _exit(EXIT_FAILURE);
                }

                close(pipefd[0]);
                close(pipefd[1]);
                if (!background) {
                    waitpid(child1, NULL, 0);
                    waitpid(child2, NULL, 0);
                } else {
                    int job_id1 = add_background_job(child1, left);
                    int job_id2 = add_background_job(child2, right);
                    if (job_id1 != -1 && job_id2 != -1) {
                        char msg1[MAX_STR_LEN], msg2[MAX_STR_LEN];
                        snprintf(msg1, sizeof(msg1), "[%d] %d\n", job_id1, child1);
                        snprintf(msg2, sizeof(msg2), "[%d] %d\n", job_id2, child2);
                        display_message(msg1);
                        display_message(msg2);
                    }
                }
            }

            for (size_t i = 0; i < cmd_count; i++) {
                free(expanded[i]);
            }
        }
    }

    cleanup_variables();
    cleanup_server(NULL);
    return 0;
}