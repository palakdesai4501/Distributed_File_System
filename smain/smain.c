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

#define PORT 8080
#define BUFFER_SIZE 1024

void prcclient(int client_socket) {
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char filename[BUFFER_SIZE];
    char dest_path[BUFFER_SIZE];
    int n;

    while (1) {
        bzero(buffer, sizeof(buffer));
        read(client_socket, buffer, sizeof(buffer));

        printf("Received command: %s\n", buffer);
        sscanf(buffer, "%s %s %s", command, filename, dest_path);

        if (strcmp(command, "ufile") == 0) {
            // Handle file upload
            char file_type[10];
            strcpy(file_type, strrchr(filename, '.') + 1);

            printf("File type detected: %s\n", file_type);

            if (strcmp(file_type, "c") == 0) {
                // Store file locally in ~/smain
                char full_path[BUFFER_SIZE];
                snprintf(full_path, sizeof(full_path), "%s/%s", dest_path, filename);

                // Create directories if they don't exist
                char mkdir_command[BUFFER_SIZE];
                snprintf(mkdir_command, sizeof(mkdir_command), "mkdir -p %s", dest_path);
                system(mkdir_command);

                int fd = open(full_path, O_WRONLY | O_CREAT, 0644);
                if (fd < 0) {
                    perror("File open failed");
                    exit(EXIT_FAILURE);
                }

                // Receive and write the file contents in chunks
                ssize_t bytes_read;
                while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0) {
                    write(fd, buffer, bytes_read);
                }

                close(fd);
                printf("File %s saved to %s\n", filename, full_path);
            } else if (strcmp(file_type, "pdf") == 0) {
                // Transfer file to Spdf
                // (Similar handling, omitted for brevity)
            } else if (strcmp(file_type, "txt") == 0) {
                // Transfer file to Stext
                // (Similar handling, omitted for brevity)
            }
        } else if (strcmp(command, "dfile") == 0) {
            // Handle file download (not implemented here)
        } else if (strcmp(command, "rmfile") == 0) {
            // Handle file removal (not implemented here)
        } else if (strcmp(command, "dtar") == 0) {
            // Handle creating tar file (not implemented here)
        } else if (strcmp(command, "display") == 0) {
            // Handle display (not implemented here)
        } else {
            printf("Invalid command received\n");
        }
    }
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    pid_t childpid;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == 0) {
        printf("Listening...\n");
    } else {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket < 0) {
            exit(EXIT_FAILURE);
        }
        if ((childpid = fork()) == 0) {
            close(server_socket);
            prcclient(client_socket);
        }
    }

    close(server_socket);
    return 0;
}
