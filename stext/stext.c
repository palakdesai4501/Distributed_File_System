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

#define PORT 6031        // Port number for Stext server
#define BUFFER_SIZE 1024 // Buffer size for data transfer

// Function to handle the 'ufile' command, which uploads a file from Smain server to the Stext server
void handle_ufile_command(int client_socket, char *filename, char *dest_path)
{
    // Send acknowledgment to the client (Smain server) to indicate readiness to receive the file
    if (write(client_socket, "ACK", strlen("ACK")) < 0)
    {
        perror("Error sending acknowledgment to client");
        close(client_socket);
        return;
    }

    char buffer[BUFFER_SIZE];
    char full_path[BUFFER_SIZE]; // Buffer to hold the full file path on the Stext server

    // Sanitize the destination path by ensuring no trailing slashes
    size_t len = strlen(dest_path);
    if (len > 0 && dest_path[len - 1] == '/')
    {
        dest_path[len - 1] = '\0';
    }

    // Create the necessary directories to store the file
    char mkdir_command[BUFFER_SIZE];
    snprintf(mkdir_command, sizeof(mkdir_command), "mkdir -p \"%s\"", dest_path);
    int ret = system(mkdir_command);
    if (ret != 0)
    {
        perror("Failed to create directory");
        close(client_socket);
        return;
    }

    // Construct the full file path by combining the directory and filename
    snprintf(full_path, sizeof(full_path), "%s/%s", dest_path, filename);

    // Debugging output to verify the full file path
    printf("Full file path: %s\n", full_path);

    // Open the file to write the received content in binary mode
    int fd = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        perror("File open failed");
        close(client_socket);
        return;
    }

    // Ensure the buffer is clean before reading
    bzero(buffer, BUFFER_SIZE);

    // Receive the file content from the Smain server and write it to the file in chunks
    ssize_t bytes_read;
    while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0)
    {
        // Write the buffer content to the file
        if (write(fd, buffer, bytes_read) != bytes_read)
        {
            perror("Error writing file content to file");
            close(fd);
            close(client_socket);
            return;
        }
        // Clear the buffer for the next chunk
        bzero(buffer, BUFFER_SIZE);
    }

    if (bytes_read < 0)
    {
        perror("Error reading from socket");
    }

    // Close the file after writing the content
    close(fd);
    printf("File %s successfully saved to %s\n", filename, full_path);
}

// Function to handle the 'dfile' command, which downloads a file from the Stext server to the Smain server
void handle_dfile_command(int client_socket, char *filename)
{
    char buffer[BUFFER_SIZE];
    char full_path[BUFFER_SIZE];

    // Construct the full file path from the filename
    // snprintf(full_path, sizeof(full_path), "~Stext/%s", filename);

    printf("The path %s", filename);

    // Open the file for reading in binary mode
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        perror("File open failed");
        close(client_socket);
        return;
    }

    // Debugging output to verify the full file path
    printf("Sending file: %s\n", full_path);

    // Read the file content and send it to the Smain server in chunks
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        if (write(client_socket, buffer, bytes_read) != bytes_read)
        {
            perror("Error sending file content");
            close(fd);
            close(client_socket);
            return;
        }
        // Clear the buffer for the next chunk
        bzero(buffer, BUFFER_SIZE);
    }

    if (bytes_read < 0)
    {
        perror("Error reading file");
    }

    // Close the file after sending the content
    close(fd);
    close(client_socket);
    printf("File %s successfully sent\n", filename);
}

// Function to handle the 'rmfile' command, which deletes a file from the Stext server
void handle_rmfile_command(int client_socket, char *filename)
{
    // Construct the full file path by replacing the Smain prefix with Stext
    char text_path[256];
    snprintf(text_path, sizeof(text_path), "%s%s", "~Stext", filename + strlen("~Smain"));

    // Attempt to delete the file
    if (unlink(text_path) == 0)
    {
        printf("File %s removed successfully\n", text_path);
    }
    else
    {
        perror("File removal failed");
    }
}

// Function to handle the 'dtar' command, which creates a tarball of all .txt files and sends it to the Smain server
void handle_dtar_command(int client_socket, char *file_type)
{
    char buffer[BUFFER_SIZE];      // Buffer to hold data
    char tar_command[BUFFER_SIZE]; // Buffer to hold the tar command

    // Construct the tar command to include all .txt files in the ~/Stext directory
    snprintf(tar_command, sizeof(tar_command), "find ~/Stext/ -name \"*.txt\" -type f | tar -cvf text_files.tar -T -");

    // Debugging output to verify the tar command
    printf("Executing command: %s\n", tar_command);

    // Execute the tar command to create the tar file
    if (system(tar_command) != 0)
    {
        perror("Failed to create tar file");
        close(client_socket);
        return;
    }

    // Open the tar file and send it to the Smain server
    int fd = open("text_files.tar", O_RDONLY);
    if (fd < 0)
    {
        perror("Failed to open tar file");
        close(client_socket);
        return;
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        if (write(client_socket, buffer, bytes_read) != bytes_read)
        {
            perror("Failed to send tar file to client");
            close(fd);
            close(client_socket);
            return;
        }
    }

    close(fd);
    printf("Tar file text_files.tar sent to Smain server\n");
}

void handle_display_command(int client_socket, char *pathname)
{
    char buffer[BUFFER_SIZE];
    char *filename;

    // Ensure the path starts with ~Stext
    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "~Stext/%s", pathname + strlen("~Smain/"));

    // Debugging: print the full path to verify it
    printf("Searching for .txt files in path: %s\n", full_path);

    // Find all .txt files in the specified directory
    snprintf(buffer, sizeof(buffer), "find %s -type f -name '*.txt'", full_path);
    FILE *fp = popen(buffer, "r");
    if (fp == NULL)
    {
        perror("Failed to run find command");
        return;
    }

    // Send the list of .txt files to the Smain server
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

        // Debugging: print the filename found
        printf("Found .txt file: %s\n", filename);

        if (write(client_socket, filename, strlen(filename)) < 0)
        {
            perror("Failed to send .txt filenames to Smain server");
            pclose(fp);
            return;
        }

        // Debugging: confirm the filename was sent
        printf(".txt filename sent to Smain server: %s\n", filename);

        // Send a newline to separate filenames
        if (write(client_socket, "\n", 1) < 0)
        {
            perror("Failed to send newline to Smain server");
            pclose(fp);
            return;
        }

        // Debugging: confirm newline was sent
        printf("Newline sent after .txt filename: %s\n", filename);
    }
    pclose(fp);

    // Send end of list marker
    snprintf(buffer, sizeof(buffer), "END_OF_LIST\n");
    if (write(client_socket, buffer, strlen(buffer)) < 0)
    {
        perror("Failed to send END_OF_LIST marker to Smain server");
    }
    pclose(client_socket);
}

// Function to handle communication with the Smain server
void prcclient(int client_socket)
{
    char buffer[BUFFER_SIZE];    // Buffer to hold data received from the Smain server
    char command[BUFFER_SIZE];   // Buffer to hold the parsed command
    char filename[BUFFER_SIZE];  // Buffer to hold the parsed filename
    char dest_path[BUFFER_SIZE]; // Buffer to hold the parsed destination path

    while (1)
    {
        // Clear the buffer and read data from the Smain server
        bzero(buffer, sizeof(buffer));
        if (read(client_socket, buffer, sizeof(buffer)) <= 0)
        {
            break; // Exit the loop if no more data is read
        }

        // Parse the command, filename, and destination path from the received data
        sscanf(buffer, "%s %s %s", command, filename, dest_path);

        // Check if the received command is "ufile" (upload file)
        if (strcmp(command, "ufile") == 0)
        {
            handle_ufile_command(client_socket, filename, dest_path);
        }
        // Check if the received command is "dfile" (download file)
        else if (strcmp(command, "dfile") == 0)
        {
            handle_dfile_command(client_socket, filename);
        }
        // Check if the received command is "rmfile" (remove file)
        else if (strcmp(command, "rmfile") == 0)
        {
            handle_rmfile_command(client_socket, filename);
        }
        // Check if the received command is "dtar" (create tarball)
        else if (strcmp(command, "dtar") == 0)
        {
            handle_dtar_command(client_socket, filename);
        }
        else if (strcmp(command, "display") == 0)
        {
            handle_display_command(client_socket, filename);
        }
        // If the command is not recognized, print an error message
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

    // Create a socket for the Stext server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Bind the socket to the specified port and IP address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming client connections (from Smain server)
    if (listen(server_socket, 10) == 0)
    {
        printf("Listening on port %d...\n", PORT);
    }
    else
    {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    // Infinite loop to accept and handle incoming client connections
    while (1)
    {
        // Accept an incoming connection from Smain server
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket < 0)
        {
            exit(EXIT_FAILURE);
        }
        // Fork a new process to handle the connection
        if ((childpid = fork()) == 0)
        {
            close(server_socket);     // Child process closes the server socket
            prcclient(client_socket); // Process the Smain server's requests
            close(client_socket);     // Close the client socket after processing
            exit(0);                  // Child process exits after handling the Smain server
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
