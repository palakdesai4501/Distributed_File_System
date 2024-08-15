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

#define PORT 11359       // Port number for the Smain server
#define BUFFER_SIZE 1024 // Buffer size for data transfer

// Function to handle the 'ufile' command: send a file to the server
void ufile(int client_socket, char *filename, char *dest_path)
{
    char buffer[BUFFER_SIZE];

    // Prepare the command to be sent to the server
    snprintf(buffer, sizeof(buffer), "ufile %s %s", filename, dest_path);

    // Send the command to the server
    if (write(client_socket, buffer, strlen(buffer)) < 0)
    {
        perror("Error sending command to server");
        exit(EXIT_FAILURE);
    }

    // Wait for acknowledgment from the server
    bzero(buffer, sizeof(buffer));
    if (read(client_socket, buffer, sizeof(buffer)) < 0)
    {
        perror("Error reading acknowledgment from server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Check if the server is ready to receive the file
    if (strcmp(buffer, "ACK") != 0)
    {
        printf("Server not ready to receive file\n");
        close(client_socket);
        return;
    }

    // Open the file to read its contents
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        // If the file cannot be opened, display an error and exit
        perror("File open failed");
        exit(EXIT_FAILURE);
    }

    // Read the file and send its content to the server in chunks
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        if (write(client_socket, buffer, bytes_read) < 0)
        {
            perror("Error sending file content to server");
            close(fd);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
    }

    // Close the file after sending its contents
    close(fd);
    close(client_socket);
    printf("File %s sent to server\n", filename);
}

// Function to handle the 'dfile' command: retrieve a file from the server
void dfile(int client_socket, char *filename)
{
    char buffer[BUFFER_SIZE];

    // Prepare the command to be sent to the server
    snprintf(buffer, sizeof(buffer), "dfile %s", filename);

    // Send the command to the server
    if (write(client_socket, buffer, strlen(buffer)) < 0)
    {
        perror("Error sending command to server");
        exit(EXIT_FAILURE);
    }

    // Extract the filename from the full path to use it locally
    char *local_filename = strrchr(filename, '/');
    if (local_filename)
    {
        local_filename++; // Skip the '/'
    }
    else
    {
        local_filename = filename; // No directory structure, use the whole filename
    }

    // Open the file to write its contents in binary mode
    int fd = open(local_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        // If the file cannot be opened, display an error and exit
        perror("File open failed");
        exit(EXIT_FAILURE);
    }

    // Read the file content from the server and write it to the file in chunks
    ssize_t bytes_read;
    while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0)
    {
        if (write(fd, buffer, bytes_read) < 0)
        {
            perror("Error writing file content to file");
            close(fd);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
    }

    // Close the file after writing its contents
    close(fd);
    close(client_socket);
    printf("File %s received from server\n", local_filename);
}

// Function to handle the 'rmfile' command: remove a file from the server
void rmfile(int client_socket, char *filename)
{
    char buffer[BUFFER_SIZE];

    // Prepare the command to be sent to the server
    snprintf(buffer, sizeof(buffer), "rmfile %s", filename);

    // Send the command to the server
    if (write(client_socket, buffer, strlen(buffer)) < 0)
    {
        perror("Error sending command to server");
        exit(EXIT_FAILURE);
    }

    printf("rmfile request sent for %s\n", filename);
}

// Client-side function to handle the 'dtar' command: request a tarball of files from the server
void request_dtar(int client_socket, const char *file_type)
{
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "dtar %s", file_type);

    // Send the dtar command to the server
    if (write(client_socket, command, strlen(command)) < 0)
    {
        perror("Failed to send dtar command to server");
        return;
    }

    // Prepare to receive the tar file
    char buffer[BUFFER_SIZE];
    int fd = open("received_cfiles.tar", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        perror("Failed to open file for receiving tar file");
        return;
    }

    ssize_t bytes_read;
    while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0)
    {
        if (write(fd, buffer, bytes_read) < 0)
        {
            perror("Failed to write tar file to disk");
            close(fd);
            return;
        }
    }

    close(fd);
    printf("Received tar file: received_cfiles.tar\n");
}

void display_filenames(int client_socket, char *pathname)
{
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "display %s", pathname);

    // Send the display command to the server
    if (write(client_socket, command, strlen(command)) < 0)
    {
        perror("Failed to send display command to server");
        return;
    }

    // Receive and print the list of filenames from the server
    char buffer[BUFFER_SIZE];
    int bytes_read;
    while ((bytes_read = read(client_socket, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
        if (strstr(buffer, "End") != NULL)
        {
            break;
        }
    }
    pclose(client_socket);
}

int main()
{
    int client_socket;              // Socket descriptor for client connection
    struct sockaddr_in server_addr; // Structure to hold server address information
    char buffer[BUFFER_SIZE];       // Buffer to hold user input

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    printf("PORT : %d\n", PORT);

    while (1)
    {
        // Create a socket to connect to the Smain server
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket < 0)
        {
            perror("Socket creation failed"); // Display an error if socket creation fails
            exit(EXIT_FAILURE);
        }

        // Connect to the Smain server
        if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            perror("Connection to the server failed");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        // Prompt the user to enter a command
        printf("Enter command: ");
        fgets(buffer, BUFFER_SIZE, stdin); // Read user input into the buffer

        buffer[strcspn(buffer, "\n")] = 0; // Remove trailing newline

        // Check if the command is 'ufile'
        if (strncmp(buffer, "ufile", 5) == 0)
        {
            char filename[BUFFER_SIZE], dest_path[BUFFER_SIZE];

            // Parse the filename and destination path from the command
            sscanf(buffer, "ufile %s %s", filename, dest_path);
            // Call the ufile function to send the file to the server
            ufile(client_socket, filename, dest_path);
        }
        // Check if the command is 'dfile'
        else if (strncmp(buffer, "dfile", 5) == 0)
        {
            char filename[BUFFER_SIZE];

            // Parse the filename from the command
            sscanf(buffer, "dfile %s", filename);
            // Call the dfile function to retrieve the file from the server
            dfile(client_socket, filename);
        }
        // Check if the command is 'rmfile'
        else if (strncmp(buffer, "rmfile", 6) == 0)
        {
            char filename[BUFFER_SIZE];

            // Parse the filename from the command
            sscanf(buffer, "rmfile %s", filename);

            // Call the rmfile function to send the delete request to the server
            rmfile(client_socket, filename);
        }
        // Check if the command is 'dtar'
        else if (strncmp(buffer, "dtar", 4) == 0)
        {
            char file_type[10];
            sscanf(buffer + 5, "%s", file_type); // Extract the file type from the command
            request_dtar(client_socket, file_type);
        }
        else if (strncmp(buffer, "display", 7) == 0)
        {
            char pathname[BUFFER_SIZE];

            // Parse the pathname from the command
            sscanf(buffer, "display %s", pathname);

            // Call the display_filenames function to retrieve and print the filenames
            display_filenames(client_socket, pathname);
        }
        else
        {
            printf("Invalid command\n"); // Inform the user if the command is not recognized
        }
    }

    // Close the client socket before exiting the program
    close(client_socket);
    return 0;
}
