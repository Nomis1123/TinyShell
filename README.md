      
# TinyShell - A Minimalist Shell Implementation in C

Welcome to TinyShell! This project is a minimalist shell, similar to `bash` or `zsh`, implemented in C. It's designed to demonstrate core concepts of systems programming, including process management, inter-process communication (IPC) via pipes, signal handling, and basic networking features.

## Features

*   **Command Execution:** Executes external commands found in the system's `PATH` (e.g., `/bin/ls`, `/bin/cat`).
*   **Built-in Commands:**
    *   `echo [args...]`: Prints arguments to standard output.
    *   `ls [--rec] [--d N] [--f filter_string] [path]`: Lists directory contents.
        *   `--rec`: Recursive listing.
        *   `--d N`: Limit recursion depth to N (requires `--rec`).
        *   `--f filter_string`: Filter results by `filter_string`.
        *   `[path]`: Optional path to list (defaults to current directory).
    *   `cd <path>`: Changes the current working directory. Supports `..`, `...`, `....` for navigating up multiple levels.
    *   `cat [file]`: Concatenates and prints the content of a file (or standard input if no file is given).
    *   `wc [file]`: Counts words, characters, and lines in a file (or standard input).
    *   `ps`: Lists currently running background jobs managed by TinyShell.
    *   `kill <pid> [signal_number]`: Sends a signal (default SIGTERM) to a process.
    *   `exit`: Exits the TinyShell.
*   **Variable Assignment & Expansion:**
    *   Set variables: `VARNAME=value` or `VARNAME=value with spaces`.
    *   Expand variables: `$VARNAME` or `${VARNAME}` in command arguments.
*   **Pipes (`|`):** Chains commands, sending the standard output of one command to the standard input of the next.
    *   Example: `ls -l | wc -l`
*   **Background Processes (`&`):** Executes a command in the background.
    *   Example: `sleep 10 &`
*   **Signal Handling:**
    *   Handles `SIGINT` (Ctrl+C), redisplaying the prompt without exiting the shell (for most operations).
    *   Manages background job completion notifications.
*   **Networking (Client/Server Model):**
    *   `start-server <port>`: Starts a simple multi-client chat server in the background on the specified port. The server echoes messages from any client to all other connected clients.
    *   `close-server`: Terminates the running TinyShell server.
    *   `send <port> <hostname> <message>`: Sends a single message to a server.
    *   `start-client <port> <hostname>`: Starts an interactive client that connects to a server, allowing multiple messages to be sent and received. Exits on `Ctrl+C`.
*   **Error Handling:** Provides basic error messages for unknown commands, incorrect arguments, and failed operations.

## Compilation

To compile TinyShell, you'll need a C to run the makefile included: 
`make`

## Usage
After compilation, run the shell: 
`./mysh`

    
