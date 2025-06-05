#include <string.h>

#include "builtins.h"
#include "io_helpers.h"


// ====== Command execution =====

/* Return: index of builtin or -1 if cmd doesn't match a builtin
 */
bn_ptr check_builtin(const char *cmd) {
    ssize_t cmd_num = 0;
    while (cmd_num < BUILTINS_COUNT &&
           strncmp(BUILTINS[cmd_num], cmd, MAX_STR_LEN) != 0) {
        cmd_num += 1;
    }
    return BUILTINS_FN[cmd_num];
}


// ====== echo =====
ssize_t bn_echo(char **tokens) {
    ssize_t index = 1;
    if (tokens[index] != NULL) {
        display_message(tokens[index]);
        index += 1;
    }
    while (tokens[index] != NULL) {
        display_message(" ");
        display_message(tokens[index]);
        index += 1;
    }
    display_message("\n");
    return 0;
}


// ====== ls =====
static void list_directory(const char *path, LsOptions options, int current_depth) {

    DIR *dir = opendir(path);
    if (!dir) {
        display_error("ERROR: Invalid path", "");
        display_error("ERROR: Builtin failed: ", "ls");

        return;
    }

    char *dirs [1024];
    int dir_count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        

        if (options.filter && !strstr(entry->d_name, options.filter)) continue;

        display_message(entry->d_name);
        display_message(" ");
        display_message("\n");
        
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            dirs[dir_count] = entry->d_name;
            dir_count++;
        }

        
    }

    for (int count = 0; count < dir_count; count++) 
    {
        if (options.recursive && (options.depth == -1 || current_depth < options.depth)) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, dirs[count]);

            struct stat st;
            if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                list_directory(full_path, options, current_depth + 1);
            }
        }
    }
        
    closedir(dir);

}