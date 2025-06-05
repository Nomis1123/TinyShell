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

size_t bn_ls(char **tokens) {
    LsOptions options = {0, -1, NULL, "."};
    char *path = NULL;
    int path_provided = 0;

    for (int i = 1; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "--rec") == 0) {
            options.recursive = 1;
        } else if (strcmp(tokens[i], "--d") == 0) {
            if (!tokens[i+1]) {
                display_error("ERROR: Missing depth value for --d", "");
                return -1;
            }
            options.depth = atoi(tokens[++i]);
            if (options.depth < 0) return -1;
        } else if (strcmp(tokens[i], "--f") == 0) {
            if (!tokens[i+1]) {
                display_error("ERROR: Missing filter string for --f", "");
                return -1;
            }
            options.filter = tokens[++i];
        } else {
            if (path_provided) {
                display_error("ERROR: Too many arguments: ls takes a single path", "");
                return -1;
            }
            path = tokens[i];
            path_provided = 1;
        }
    }

    if (options.depth != -1 && !options.recursive) {
        display_error("ERROR: --d requires --rec", "");
        return -1;
    }

    if (path) options.path = path;
    list_directory(options.path, options, 0);
    return 0;
}

// ====== cd =====
ssize_t bn_cd(char **tokens)
{
    if (tokens[1] == NULL) {
        display_error("ERROR: Missing path for cd", "");
        return -1;
    }

    else if (strcmp(tokens[1], "...")==0) 
    {
        if (chdir("../..")==-1) {
            display_error("ERROR: Invalid path: ", tokens[1]);
            return -1;
        }  
    }

    else if (strcmp(tokens[1], "....")==0)  
    {
        if (chdir("../../..")==-1) {
            display_error("ERROR: Invalid path: ", tokens[1]);
            return -1;
        }  
    }

    else if (chdir(tokens[1]) == -1) {
        display_error("ERROR: Invalid path: ", tokens[1]);
        return -1;
    }
    
    return 0;
}

// ====== cat =====
ssize_t bn_cat(char **tokens)
{
    char *line = NULL;
    size_t size = 0;
    ssize_t nread;

    if (tokens[1] == NULL)
    {
        display_error("ERROR: No input source provided", "");
        return -1;
    }

    FILE* fp;
    
    fp = fopen(tokens[1], "r");

    if (fp == NULL)
    {
        display_error("ERROR: Cannot open file", "");
        return -1;
    }

    while ((nread = getline(&line, &size, fp)) != -1) 
    {
        display_message(line);
        if (line[nread-1] != '\n') 
            display_message("\n");
        free (line);
        line = NULL;
    }

    free(line);
    fclose(fp);

    return 0;

}

// A utility function to reverse a string
void reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        end--;
        start++;
    }
}

// Implementation of citoa()
char* citoa(int num, char* str, int base)
{
    int i = 0;
    bool isNegative = false;
 
    /* Handle 0 explicitly, otherwise empty string is
     * printed for 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    // In standard itoa(), negative numbers are handled
    // only with base 10. Otherwise numbers are
    // considered unsigned.
    if (num < 0 && base == 10) {
        isNegative = true;
        num = -num;
    }
 
    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }
 
    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0'; // Append string terminator
 
    // Reverse the string
    reverse(str, i);
 
    return str;
}

// ====== wc =====
ssize_t bn_wc(char **tokens)
{
    if (tokens[1] == NULL)
    {
        display_error("ERROR: No input source provided", "");
        return -1;
    }

    FILE* fp;
    
    fp = fopen(tokens[1], "r");

    if (fp == NULL)
    {
        display_error("ERROR: Cannot open file", "");
        return -1;
    }

    char c;
    int words = 0;
    int chars = 0;
    int lines = 0;
    bool in_word = false;

    fseek(fp, 0, SEEK_END); // seek to end of file
    int fsize = ftell(fp); // get current file pointer
    fseek(fp, 0, SEEK_SET); // seek back to beginning of file
    char *buffer = (char *)malloc(fsize + 1);
    fread(buffer, fsize, 1, fp);
    buffer[fsize] = 0;
    int pos = 0;

    while (buffer[pos] != '\0')
    {
        c = buffer[pos];

        pos++;
        if (c != '\n' && c != '\t' && c != ' ' && c != '\r')
        {
            in_word = true;
            
        }
        
        if (c == '\n' || c == '\t' || c == ' ')
        {
            
            if (in_word)
            {
                in_word = false;
                words++;
            }
        }
        if (c == '\n')
        {
            lines++;
        } 
        
        chars++;  
    }

 

    if (in_word)
    {
        words++;
        in_word = false;
    }

    char *buffet = malloc(128);
    buffet = citoa(words, buffet, 10);
    display_message("word count ");
    display_message(buffet);
    display_message("\n");

    buffet = citoa(chars, buffet, 10);
    display_message("character count ");
    display_message(buffet);
    display_message("\n");

    buffet = citoa(lines, buffet, 10);
    display_message("newline count ");
    display_message(buffet);
    display_message("\n");
    

    free(buffet);
    free(buffer);
    fclose(fp);

    return 0;
}

