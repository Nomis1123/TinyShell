#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "builtins.h"
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>  



struct client_sock {
    int sock_fd;
    int id;
    
};
struct client_sock *clients[MAX_CLIENTS]={0};

int getavailableclient()
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == 0) {
            return i;
        }
    }
    return -1;
}

struct client_sock *findclient(int id)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0 && clients[i]->sock_fd == id) {
            return clients[i];
        }
    }
    return NULL;
}


// ====== Builtin Registry =====
bn_ptr check_builtin(const char *cmd) {
    for (ssize_t i = 0; i < BUILTINS_COUNT; i++) {
        if (strcmp(BUILTINS[i], cmd) == 0) {
            return BUILTINS_FN[i];
        }
    }
    return NULL;
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

// ====== ls =====`
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

ssize_t bn_ls(char **tokens) {
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
        // Read from stdin/pipe
        while ((nread = getline(&line, &size, stdin)) != -1) 
        {
            write(STDOUT_FILENO, line, nread);
            free(line);
            line = NULL;
        }
        free(line);
        return 0;
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
        write(STDOUT_FILENO, line, nread);
        free(line);
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
    char *buffer = NULL;
    size_t buffer_size = 1024;
    size_t total_size = 0;
    ssize_t bytes_read;

   
    
    if (tokens[1] == NULL)
    {
        
        // Reading from pipe/stdin
        buffer = (char *)malloc(buffer_size);
        if (buffer == NULL)
        {
            display_error("ERROR: Cannot allocate memory", "");
            return -1;
        }

        // Read from STDIN_FILENO (which will be the pipe if piped)
        char temp_buf[1024];
        while ((bytes_read = read(STDIN_FILENO, temp_buf, sizeof(temp_buf))) > 0)
        {
            if (total_size + bytes_read >= buffer_size)
            {
                buffer_size = (total_size + bytes_read) * 2;
                char *new_buffer = (char *)realloc(buffer, buffer_size);
                if (new_buffer == NULL)
                {
                    display_error("ERROR: Cannot reallocate memory", "");
                    free(buffer);
                    return -1;
                }
                buffer = new_buffer;
                
            }
            memcpy(buffer + total_size, temp_buf, bytes_read);
            total_size += bytes_read;
            buffer[total_size] = '\0';
            
        }

        if (bytes_read == -1)
        {
            display_error("ERROR: Cannot read from stdin", "");
            free(buffer);
            return -1;
        }
    }
    else
    {
        // Read from file
        FILE* fp = fopen(tokens[1], "r");
        if (fp == NULL)
        {
            display_error("ERROR: Cannot open file", "");
            return -1;
        }
        
        // Get file size
        fseek(fp, 0, SEEK_END);
        total_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
        // Allocate buffer and read file
        buffer = (char *)malloc(total_size + 1);
        if (buffer == NULL)
        {
            display_error("ERROR: Cannot allocate memory", "");
            fclose(fp);
            return -1;
        }
        
        if (fread(buffer, 1, total_size, fp) != total_size)
        {
            display_error("ERROR: Cannot read file", "");
            free(buffer);
            fclose(fp);
            return -1;
        }
        buffer[total_size] = '\0';
        
        fclose(fp);
    }

    

    // Process the buffer contents

    int words = 0;
    int chars = total_size;
    int lines = 0;
    bool in_word = false;

    for (size_t i = 0; i < total_size; i++)
    {
        char c = buffer[i];

        if (c == '\n')
        {
            lines++;
            if (in_word)
            {
                words++;
                in_word = false;
            }
        }
        else if (c == ' ' || c == '\t')
        {
            if (in_word)
            {
                words++;
                in_word = false;
            }
        }
        else
        {
            in_word = true;
        }
    }

    if (buffer[total_size - 1] != '\n')
    {
        lines++;
    }

    // Count last word if it exists
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
    return 0;
}

ssize_t bn_ps(char **tokens) {
    (void)tokens; // Unused parameter
    
    // Iterate backward to show most recent jobs first
    for (int i = MAX_BG_JOBS - 1; i >= 0; i--) {
        if (bg_jobs[i].pid != 0 && !bg_jobs[i].completed) {
            char msg[MAX_STR_LEN];
            // Extract only the first part of the command (before spaces)
            char command_name[MAX_STR_LEN];
            strncpy(command_name, bg_jobs[i].command, sizeof(command_name) - 1);
            command_name[sizeof(command_name) - 1] = '\0'; // Ensure null-termination
            
            // Truncate at the first space to remove arguments
            char *space = strchr(command_name, ' ');
            if (space != NULL) *space = '\0';
            
            // Limit command name to 116 chars to leave room for PID (up to 10 digits) and formatting
            snprintf(msg, sizeof(msg), "%.116s %d\n", command_name, bg_jobs[i].pid);
            display_message(msg);
        }
    }
    return 0;
}

// ====== kill =====
ssize_t bn_kill(char **tokens) {
    if (tokens[1] == NULL) {
        display_error("ERROR: Missing PID", "");
        return -1;
    }

    // Parse PID
    char *endptr;
    pid_t pid = (pid_t) strtol(tokens[1], &endptr, 10);
    if (*endptr != '\0') {
        display_error("ERROR: Invalid PID format", "");
        return -1;
    }

    // Default signal is SIGTERM
    int signum = SIGTERM; 
    
    // Parse optional signal number
    if (tokens[2] != NULL) {
        signum = (int) strtol(tokens[2], &endptr, 10);
        if (*endptr != '\0' || signum < 1 || signum > 64) {
            display_error("ERROR: Invalid signal specified", "");
            return -1;
        }
    }

    // Check if process exists
    if (kill(pid, 0) == -1) {
        display_error("ERROR: The process does not exist", "");
        return -1;
    }

    // Send signal
    if (kill(pid, signum) == -1) {
        perror("kill");
        return -1;
    }

    return 0;
}

ssize_t bn_start_server(char **tokens) {
    if (!tokens[1]) {
        display_error("ERROR: No port provided", "");
        return -1;
    }

    int port = atoi(tokens[1]);
    if (port <= 0) {
        display_error("ERROR: Invalid port", "");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        display_error("ERROR: Fork failed", "");
        return -1;
    }

    if (pid == 0) { // Child process - server
        // Create socket
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            display_error("ERROR: socket creation failed", "");
            _exit(EXIT_FAILURE);
        }

        // Set socket options
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in address = {
            .sin_family = AF_INET,
            .sin_addr.s_addr = INADDR_ANY,
            .sin_port = htons(port)
        };

        // Bind and listen
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0 ||
            listen(server_fd, 5) < 0) {
            display_error("ERROR: bind/listen failed", "");
            close(server_fd);
            _exit(EXIT_FAILURE);
        }

        // Non-blocking I/O setup
        fd_set master_set;
        FD_ZERO(&master_set);
        FD_SET(server_fd, &master_set);
        int max_fd = server_fd;

        while (1) 
        {
            fd_set read_set = master_set;
            select(max_fd+1, &read_set, NULL, NULL, NULL); 
                //break;

            for (int i = 0; i <= max_fd; i++) 
            {
                if (FD_ISSET(i, &read_set)) 
                {
                    
                    if (i == server_fd) 
                    {
                        
                        // Accept new connection
                        struct sockaddr_in client_addr;
                        socklen_t addr_len = sizeof(client_addr);
                        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
                        if (client_fd > 0) {
                            FD_SET(client_fd, &master_set);
                            max_fd = (client_fd > max_fd) ? client_fd : max_fd;
                            int client_id = getavailableclient();
                            
                            clients[client_id] = malloc(sizeof(struct client_sock));
                            clients[client_id]->sock_fd = client_fd;
                            clients[client_id]->id = client_id+1;
                            
                        }
                    } 
                    else 
                    {
                        
                        // Handle client (echo server example)
                        char buf[1024];
                        ssize_t bytes = recv(i, buf, sizeof(buf), 0);
                        if (bytes <= 0) 
                        {
                            //close(i); 
                            //FD_CLR(i, &master_set);
                        } 
                        else 
                        {
                            
                            struct client_sock *client = findclient(i);
                            
                            if (client != NULL) 
                            {
                                char msg[2048];
                                snprintf(msg, sizeof(msg), "client%d: %s", client->id, buf);
                                display_message(msg);

                                for (int c = 0; c < MAX_CLIENTS; c++)
                                {
                                    if (clients[c] != 0 )
                                    {
                                        
                                        send(clients[c]->sock_fd, msg, sizeof(msg), 0);
                                    }
                                }
                                
                                
                            }
                            else
                            {
                                char msg[2048];
                                snprintf(msg, sizeof(msg), "%s", buf);
                                display_message(msg);
                                

                            }
                        } 
                    }
                }
            }
        }
        close(server_fd);
        _exit(EXIT_SUCCESS);
    } 
    else { // Parent process
        char cmd_line[MAX_STR_LEN];
        snprintf(cmd_line, sizeof(cmd_line), "start-server %s", tokens[1]);
        add_background_job(pid, cmd_line);
    }
    return 0;
}

ssize_t bn_close_server(char **tokens) {
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
    display_error("ERROR: No active server", "");
    return -1;
}


// ====== send =====
ssize_t bn_send(char **tokens) {
    if (!tokens[1] || !tokens[2] || !tokens[3]) {
        display_error("ERROR: Usage: send <port> <hostname> <message>", "");
        return -1;
    }

    // Parse port and hostname
    const char *port = tokens[1];
    const char *hostname = tokens[2];
    

    // Resolve hostname
    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *result;
    if (getaddrinfo(hostname, port, &hints, &result)) {
        display_error("ERROR: Host resolution failed", "");
        return -1;
    }

    // Create socket
    int sock_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock_fd < 0) {
        perror("socket");
        freeaddrinfo(result);
        return -1;
    }

    // Connect to server
    if (connect(sock_fd, result->ai_addr, result->ai_addrlen) < 0) {
        perror("connect");
        close(sock_fd);
        freeaddrinfo(result);
        return -1;
    }
    freeaddrinfo(result);

    
    // Format message: "<message>\r\n"
    char full_msg[MAX_STR_LEN];
    int t=3;
    while(1)
    {
        if (!tokens[t])
        {
            break;
        }
        else
        {
            if (t > 3)
            {
                strcat(full_msg, " ");
            }
            strncat(full_msg, tokens[t], sizeof(full_msg) - strlen(full_msg) - 1);
            
        }
        t++;

    }

    strcat(full_msg, "\r\n");
    if (write(sock_fd, full_msg, strlen(full_msg)) < 0) {
        perror("write message");
        close(sock_fd);
        return -1;
    }


    
    close(sock_fd);
    return 0;
}

//fork client!!!

ssize_t bn_start_client(char **tokens)
{
    if (!tokens[1] || !tokens[2] ) {
        display_error("ERROR: Usage: send <port> <hostname> <message>", "");
        return -1;
    }

    // Parse port and hostname
    const char *port = tokens[1];
    const char *hostname = tokens[2];
    

    // Resolve hostname
    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *result;
    if (getaddrinfo(hostname, port, &hints, &result)) {
        display_error("ERROR: Host resolution failed", "");
        return -1;
    }

    // Create socket
    int sock_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock_fd < 0) {
        perror("socket");
        freeaddrinfo(result);
        return -1;
    }

    // Connect to server
    if (connect(sock_fd, result->ai_addr, result->ai_addrlen) < 0) {
        perror("connect");
        close(sock_fd);
        freeaddrinfo(result);
        return -1;
    }
    freeaddrinfo(result);

    //display_message("\n");

    int pid = fork();

    if (pid < 0) {
        display_error("ERROR: Fork failed", "");
        return -1;
    }

    if (pid==0)
    {
        char input_buf[MAX_STR_LEN + 1];
        input_buf[MAX_STR_LEN] = '\0';
        while(1)
        {
            if (the_flag == 1)
            {
                the_flag = 0;
                break;
            }
            ssize_t ret = get_input(input_buf);
            if (ret == 0)
            {
                break;
            }

            strcat(input_buf, "\r\n");
            if (write(sock_fd, input_buf, strlen(input_buf)) < 0) {
                perror("write message");
                close(sock_fd);
                return -1;
            }
            
        }

    }

    else
    {
        while(1)
        {
            char buf[1024];
            ssize_t bytes = recv(sock_fd, buf, sizeof(buf), 0);
            if (the_flag == 1) 
            {
                the_flag = 0;
                break;
            } 

            if (bytes > 0) 
            {
                display_message(buf);
            } 

        }
        
    }

    

    close(sock_fd);
    return 0;
}