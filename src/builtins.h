#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include "variables.h"
#include "io_helpers.h"
#include <stdbool.h>
#include "io_helpers.h"
#include <signal.h>

#define MAX_BG_JOBS 100
#define MAX_STR_LEN 128
#define MAX_CLIENTS 100
extern sig_atomic_t the_flag;

// Add BackgroundJob structure declaration
typedef struct {
    int job_id;
    pid_t pid;
    char command[MAX_STR_LEN];
    bool completed;
} BackgroundJob;

// Declare external reference to bg_jobs
extern BackgroundJob bg_jobs[];

// Existing function declarations
typedef ssize_t (*bn_ptr)(char **);
int add_background_job(pid_t pid, const char *command);

typedef ssize_t (*bn_ptr)(char **);
ssize_t bn_echo(char **tokens);
ssize_t bn_ls(char **tokens);
ssize_t bn_cd(char **tokens); 
ssize_t bn_cat(char **tokens); 
ssize_t bn_wc(char **tokens); 
ssize_t bn_ps(char **tokens);
ssize_t bn_kill(char **tokens);
ssize_t bn_start_server(char **tokens);
ssize_t bn_close_server(char **tokens);
ssize_t bn_send(char **tokens);
ssize_t bn_start_client(char **tokens);

bn_ptr check_builtin(const char *cmd);

static const char * const BUILTINS[] = {"echo", "ls", "cd", "cat", "wc", "kill", "ps", "start-server", "close-server", "send", "start-client"};
static const bn_ptr BUILTINS_FN[] = {&bn_echo, &bn_ls, &bn_cd, &bn_cat, &bn_wc, &bn_kill, &bn_ps, &bn_start_server, &bn_close_server, &bn_send, &bn_start_client};
static const ssize_t BUILTINS_COUNT = sizeof(BUILTINS) / sizeof(char *);

/* LS options struct */
typedef struct {
    int recursive;
    int depth;
    char *filter;
    char *path;
} LsOptions;


#endif
