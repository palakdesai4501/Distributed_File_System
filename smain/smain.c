#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

// Define constants for port numbers and buffer size
#define PORT 11359            // Port number for Smain server
#define BUFFER_SIZE 1024      // Buffer size for data transfer
#define PDF_SERVER_PORT 13020 // Port number for Spdf server
#define TEXT_SERVER_PORT 6031 // Port number for Stext server

// Function to send a file to the appropriate server (Spdf or Stext)
void send_file_to_server(char *filename, char *dest_path, int server_port, int client_socket)
{
    int server_socket;              // Socket descriptor for the server connection
    struct sockaddr_in server_addr; // Structure to hold server address information
    char buffer[BUFFER_SIZE];       // Buffer to hold data to be sent

    // Create a socket to connect to the target server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the target server
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection to server failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Send the command to the target server to initiate file transfer
    snprintf(buffer, sizeof(buffer), "ufile %s %s", filename, dest_path);
    if (write(server_socket, buffer, strlen(buffer)) < 0)
    {
        perror("Error sending command to server");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Wait for acknowledgment from the server
    bzero(buffer, sizeof(buffer));
    if (read(server_socket, buffer, sizeof(buffer)) < 0)
    {
        perror("Error reading acknowledgment from server");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Check if the server is ready to receive the file
    if (strcmp(buffer, "ACK") != 0)
    {
        printf("Server not ready to receive file\n");
        close(server_socket);
        return;
    }

    // Clear the buffer and start sending file content
    bzero(buffer, sizeof(buffer));
    ssize_t bytes_read;
    while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0)
    {
        if (write(server_socket, buffer, bytes_read) < 0)
        {
            perror("Error sending file content to server");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
    }

    printf("File %s sent to server on port %d\n", filename, server_port);
}

// Function to handle the 'ufile' command from the client
void handle_ufile_command(char *filename, char *dest_path, int client_socket)
{
    // Send acknowledgment to the client to indicate readiness
    if (write(client_socket, "ACK", strlen("ACK")) < 0)
    {
        perror("Error sending acknowledgment to client");
        close(client_socket);
        return;
    }

    char file_type[10];                            // Buffer to hold the file extension/type
    strcpy(file_type, strrchr(filename, '.') + 1); // Extract the file type from the filename

    // Handle .c files: store them locally in Smain
    if (strcmp(file_type, "c") == 0)
    {
        char full_path[BUFFER_SIZE]; // Buffer to hold the full file path on Smain server
        snprintf(full_path, sizeof(full_path), "%s/%s", dest_path, filename);

        // Create the necessary directories
        char mkdir_command[BUFFER_SIZE];
        snprintf(mkdir_command, sizeof(mkdir_command), "mkdir -p %s", dest_path);
        system(mkdir_command);

        // Open the file to write the received content in binary mode
        int fd = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            perror("File open failed");
            close(client_socket);
            return;
        }

        ssize_t bytes_read;
        char buffer[BUFFER_SIZE];
        // Read data from the client and write it to the file
        while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0)
        {
            ssize_t bytes_written = write(fd, buffer, bytes_read);
            if (bytes_written < 0)
            {
                close(fd);
                close(client_socket);
                return;
            }
        }
        // Close the file after writing the content
        close(fd);
        printf("File %s saved to %s\n", filename, full_path);
    }
    // Handle .txt files: transfer them to the Stext server
    else if (strcmp(file_type, "txt") == 0)
    {
        char new_dest_path[BUFFER_SIZE];
        snprintf(new_dest_path, sizeof(new_dest_path), "~Stext%s", dest_path + 6); // Replace ~smain with ~stext
        send_file_to_server(filename, new_dest_path, TEXT_SERVER_PORT, client_socket);
    }
    // Handle .pdf files: transfer them to the Spdf server
    else if (strcmp(file_type, "pdf") == 0)
    {
        char new_dest_path[BUFFER_SIZE];
        snprintf(new_dest_path, sizeof(new_dest_path), "~Spdf%s", dest_path + 6); // Replace ~smain with ~spdf
        send_file_to_server(filename, new_dest_path, PDF_SERVER_PORT, client_socket);
    }
    // Unsupported file types
    else
    {
        printf("Unsupported file type: %s\n", file_type);
    }
}

// Function to receive a file from a specified server (Spdf or Stext)
void receive_file_from_server(char *filename, int server_port, int client_socket)
{
    int server_socket;              // Socket descriptor for the server connection
    struct sockaddr_in server_addr; // Structure to hold server address information
    char buffer[BUFFER_SIZE];       // Buffer to hold data to be sent

    // Create a socket to connect to the target server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the target server
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection to server failed");
        exit(EXIT_FAILURE);
    }

    // Clear the buffer before reading file contents
    bzero(buffer, sizeof(buffer));

    // Send the command to the target server to initiate file transfer
    snprintf(buffer, sizeof(buffer), "dfile %s", filename);
    if (write(server_socket, buffer, strlen(buffer)) < 0)
    {
        perror("Error sending command to server");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Read the file content from the server and write it to the client socket in chunks
    ssize_t bytes_read;
    while ((bytes_read = read(server_socket, buffer, sizeof(buffer))) > 0)
    {
        if (write(client_socket, buffer, bytes_read) < 0)
        {
            perror("Error writing file content to client");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
    }
    // Close the socket after the transfer is complete
    close(server_socket);
    close(client_socket);
    printf("File %s received from server on port %d\n", filename, server_port);
}

// Function to handle the 'dfile' command from the client
void handle_dfile_command(char *filename, int client_socket)
{
    char file_type[10];                            // Buffer to hold the file extension/type
    strcpy(file_type, strrchr(filename, '.') + 1); // Extract the file type from the filename

    // Handle .c files: retrieve them locally from Smain
    if (strcmp(file_type, "c") == 0)
    {
        // Open the file locally and send it to the client
        int fd = open(filename, O_RDONLY);
        if (fd < 0)
        {
            perror("File open failed");
            close(client_socket);
            return;
        }

        // Read the file and send its content to the client in chunks
        ssize_t bytes_read;
        char buffer[BUFFER_SIZE];
        while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
        {
            if (write(client_socket, buffer, bytes_read) < 0)
            {
                perror("Error sending file content to client");
                close(fd);
                close(client_socket);
                return;
            }
        }
        close(fd);
    }
    // Handle .pdf files: retrieve them from Spdf server
    else if (strcmp(file_type, "pdf") == 0)
    {
        char new_file_path[BUFFER_SIZE];
        snprintf(new_file_path, sizeof(new_file_path), "~Spdf%s", filename + 6); // Replace ~smain with ~spdf
        receive_file_from_server(new_file_path, PDF_SERVER_PORT, client_socket);
        // handle_dfile_command(filename, client_socket); // Recursive call to send file to client
    }
    // Handle .txt files: retrieve them from Stext server
    else if (strcmp(file_type, "txt") == 0)
    {
        char new_file_path[BUFFER_SIZE];
        snprintf(new_file_path, sizeof(new_file_path), "~Stext%s", filename + 6); // Replace ~smain with ~stext
        receive_file_from_server(new_file_path, TEXT_SERVER_PORT, client_socket);
        // handle_dfile_command(filename, client_socket); // Recursive call to send file to client
    }
    // Unsupported file types
    else
    {
        printf("Unsupported file type: %s\n", file_type);
    }
}

// Function to send a delete command to a specified server (Spdf or Stext)
void send_delete_command_to_server(char *filename, int server_port)
{
    int server_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create a socket to connect to the target server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        return;
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the target server
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection to server failed");
        close(server_socket);
        return;
    }

    // Send the delete command to the target server
    snprintf(buffer, sizeof(buffer), "rmfile %s", filename);
    if (write(server_socket, buffer, strlen(buffer)) < 0)
    {
        perror("Error sending command to server");
        close(server_socket);
        return;
    }

    close(server_socket);
    printf("Delete command sent to server on port %d for file %s\n", server_port, filename);
}

// Function to handle the 'rmfile' command from the client
void handle_rmfile_command(char *filename)
{
    char *extension = strrchr(filename, '.'); // Extract the file extension
    if (extension)
    {
        // Handle .c files locally
        if (strcmp(extension, ".c") == 0)
        {
            if (remove(filename) == 0)
            {
                printf("Deleted %s locally\n", filename);
            }
            else
            {
                perror("Error deleting file");
            }
        }
        // Handle .pdf files: send delete command to Spdf server
        else if (strcmp(extension, ".pdf") == 0)
        {
            send_delete_command_to_server(filename, PDF_SERVER_PORT);
        }
        // Handle .txt files: send delete command to Stext server
        else if (strcmp(extension, ".txt") == 0)
        {
            send_delete_command_to_server(filename, TEXT_SERVER_PORT);
        }
        // Unsupported file types
        else
        {
            printf("Unsupported file type\n");
        }
    }
    else
    {
        printf("Invalid file name\n");
    }
}

// Function to handle the 'dtar' command for tarball creation and transfer
void handle_dtar_command(int client_socket, const char *file_type)
{
    char buffer[BUFFER_SIZE];      // Buffer to hold data
    char tar_command[BUFFER_SIZE]; // Buffer to hold the tar command

    // Check if the file type is .c
    if (strcmp(file_type, ".c") == 0)
    {
        // Construct the tar command to include files in the ~/smain directory
        snprintf(tar_command, sizeof(tar_command), "find ~/Smain/ -name \"*.c\" -type f | tar -cvf cfiles.tar -T -");

        // Debugging output to verify the tar command
        printf("Executing command: %s\n", tar_command);

        // Execute the tar command to create the tar file
        if (system(tar_command) != 0)
        {
            perror("Failed to create tar file");
            close(client_socket);
            return;
        }

        // Open the tar file and send it to the client
        int fd = open("cfiles.tar", O_RDONLY);
        if (fd < 0)
        {
            perror("Failed to open tar file");
            close(client_socket);
            return;
        }

        ssize_t bytes_read;
        while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
        {
            if (write(client_socket, buffer, bytes_read) < 0)
            {
                perror("Failed to send tar file to client");
                close(fd);
                close(client_socket);
                return;
            }
        }

        close(fd);
        printf("Tar file cfiles.tar sent to the client\n");
    }
    // Handle .pdf files: transfer tarball from Spdf server to client
    else if (strcmp(file_type, ".pdf") == 0)
    {
        // Repeat similar process for pdf files
        int server_socket;
        struct sockaddr_in server_addr;
        char buffer[BUFFER_SIZE];

        // Create a socket to connect to the target server
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            perror("Socket creation failed");
            return;
        }

        // Set up the server address structure
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PDF_SERVER_PORT);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        // Connect to the target server
        if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            perror("Connection to server failed");
            close(server_socket);
            return;
        }

        // Send the dtar command to the target server
        snprintf(buffer, sizeof(buffer), "dtar %s", file_type);
        if (write(server_socket, buffer, strlen(buffer)) < 0)
        {
            perror("Error sending command to server");
            close(server_socket);
            return;
        }

        // Read the tar file content from the server and send it to the client
        char tar_content[BUFFER_SIZE];
        ssize_t bytes_read = 0;
        while ((bytes_read = read(server_socket, tar_content, sizeof(tar_content))) > 0)
        {
            if (write(client_socket, tar_content, bytes_read) < 0)
            {
                perror("Failed to send tar file to client");
                close(client_socket);
                return;
            }
        }
        close(server_socket);
    }
    // Handle .txt files: transfer tarball from Stext server to client
    else if (strcmp(file_type, ".txt") == 0)
    {
        // Repeat similar process for txt files
        int server_socket;
        struct sockaddr_in server_addr;
        char buffer[BUFFER_SIZE];

        // Create a socket to connect to the target server
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            perror("Socket creation failed");
            return;
        }

        // Set up the server address structure
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(TEXT_SERVER_PORT);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        // Connect to the target server
        if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            perror("Connection to server failed");
            close(server_socket);
            return;
        }

        // Send the dtar command to the target server
        snprintf(buffer, sizeof(buffer), "dtar %s", file_type);
        if (write(server_socket, buffer, strlen(buffer)) < 0)
        {
            perror("Error sending command to server");
            close(server_socket);
            return;
        }

        // Read the tar file content from the server and send it to the client
        char tar_content[BUFFER_SIZE];
        ssize_t bytes_read = 0;
        while ((bytes_read = read(server_socket, tar_content, sizeof(tar_content))) > 0)
        {
            if (write(client_socket, tar_content, bytes_read) < 0)
            {
                perror("Failed to send tar file to client");
                close(client_socket);
                return;
            }
        }
        close(server_socket);
    }
    // Unsupported file types
    else
    {
        printf("Unsupported file type: %s\n", file_type);
    }
}

void request_filenames_from_server(const char *server_ip, int server_port, const char *pathname, int client_socket)
{
    int server_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Create a socket to connect to the target server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        return;
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Connect to the target server
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection to server failed");
        close(server_socket);
        return;
    }

    // Send the display command with the pathname to the remote server
    snprintf(buffer, sizeof(buffer), "display %s", pathname);
    if (write(server_socket, buffer, strlen(buffer)) < 0)
    {
        perror("Error sending display command to server");
        close(server_socket);
        return;
    }

    // Read filenames from the remote server and forward them to the client
    while ((bytes_read = read(server_socket, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_read] = '\0'; // Null-terminate the received data
        if (write(client_socket, buffer, bytes_read) < 0)
        {
            perror("Failed to forward filenames to client");
            close(server_socket);
            return;
        }

        // Check for the END_OF_LIST marker to stop reading
        // if (strstr(buffer, "End") != NULL) {
        //     break;
        // }
    }

    // Close the connection to the remote server
    close(server_socket);
}

void handle_display_command(char *pathname, int client_socket)
{
    char buffer[BUFFER_SIZE];
    char *filename;

    // Ensure the path starts with ~Smain
    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "~Smain/%s", pathname + strlen("~Smain/"));

    // Debugging: print the full path to verify it
    printf("Searching for .c files in path: %s\n", full_path);

    // Gather .c files from Smain locally
    FILE *fp;
    snprintf(buffer, sizeof(buffer), "find %s -type f -name '*.c'", full_path);
    fp = popen(buffer, "r");
    if (fp == NULL)
    {
        perror("Failed to run find command");
        return;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        // Extract the filename from the full path
        filename = strrchr(buffer, '/');
        if (filename != NULL)
        {
            filename++; // Move past the last '/'
        }
        else
        {
            filename = buffer; // No '/' found, the whole buffer is the filename
        }
        // Remove any trailing newline characters
        filename[strcspn(filename, "\n")] = 0;

        printf("Found .c file: %s\n", filename); // Debugging: print the filename found

        if (write(client_socket, filename, strlen(filename)) < 0)
        {
            perror("Failed to send .c filenames to client");
            pclose(fp);
            return;
        }

        // Send a newline to separate filenames
        if (write(client_socket, "\n", 1) < 0)
        {
            perror("Failed to send newline to client");
            pclose(fp);
            return;
        }
    }
    pclose(fp);

    // Request .pdf files from Spdf server
    printf("Requesting .pdf files from Spdf server...\n"); // Debugging
    request_filenames_from_server("127.0.0.1", PDF_SERVER_PORT, pathname, client_socket);

    // Request .txt files from Stext server
    printf("Requesting .txt files from Stext server...\n"); // Debugging
    request_filenames_from_server("127.0.0.1", TEXT_SERVER_PORT, pathname, client_socket);

    // Send end of list marker
    // snprintf(buffer, sizeof(buffer), "END_OF_LIST\n");
    if (write(client_socket, buffer, strlen(buffer)) < 0)
    {
        perror("Failed to send END_OF_LIST marker to client");
    }

    printf("END_OF_LIST sent to client.\n"); // Debugging
}

// Function to handle communication with the client
void prcclient(int client_socket)
{
    char buffer[BUFFER_SIZE];    // Buffer to hold data received from the client
    char command[BUFFER_SIZE];   // Buffer to hold the parsed command
    char filename[BUFFER_SIZE];  // Buffer to hold the parsed filename
    char dest_path[BUFFER_SIZE]; // Buffer to hold the parsed destination path

    while (1)
    {
        // Clear the buffer and read data from the client
        bzero(buffer, sizeof(buffer));
        if (read(client_socket, buffer, sizeof(buffer)) <= 0)
        {
            break; // Exit the loop if no more data is read
        }

        // Parse the command, filename, and destination path from the received data
        sscanf(buffer, "%s %s %s", command, filename, dest_path);

        // Check if the received command is "ufile"
        if (strcmp(command, "ufile") == 0)
        {
            handle_ufile_command(filename, dest_path, client_socket);
        }
        // Check if the received command is "dfile"
        else if (strcmp(command, "dfile") == 0)
        {
            handle_dfile_command(filename, client_socket);
        }
        // Check if the received command is "rmfile"
        else if (strcmp(command, "rmfile") == 0)
        {
            handle_rmfile_command(filename);
        }
        // Check if the received command is "dtar"
        else if (strcmp(command, "dtar") == 0)
        {
            handle_dtar_command(client_socket, filename);
        }
        else if (strcmp(command, "display") == 0)
        {
            handle_display_command(filename, client_socket);
        }
        else
        {
            printf("Invalid command received\n");
        }
    }
}

int main()
{
    int server_socket, client_socket;            // Socket descriptors for server and client connections
    struct sockaddr_in server_addr, client_addr; // Structures to hold server and client address information
    socklen_t addr_size;                         // Size of the client address structure
    pid_t childpid;                              // Process ID for child processes

    // Create a socket for the Smain server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    printf("PORT : %d\n", PORT);
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Bind the socket to the specified port and IP address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming client connections
    if (listen(server_socket, 10) == 0)
    {
        printf("Listening...\n");
    }
    else
    {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    // Infinite loop to accept and handle incoming client connections
    while (1)
    {
        // Accept an incoming client connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket < 0)
        {
            exit(EXIT_FAILURE);
        }

        // Fork a new process to handle the client connection
        if ((childpid = fork()) == 0)
        {
            close(server_socket);     // Child process closes the server socket
            prcclient(client_socket); // Process the client's requests
            close(client_socket);
            exit(0); // Child process exits after handling the client
        }
        else
        {
            close(client_socket); // Parent process closes the client socket
        }
    }

    // Close the server socket before exiting the program
    close(server_socket);
    return 0;
}
