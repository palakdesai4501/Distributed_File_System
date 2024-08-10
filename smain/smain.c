#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PORT 8080
#define BUFSIZE 1024

void handle_client(int client_sock);
void forward_file_to_server(const char *filename, const char *dest_path, const char *server_ip, int server_port);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 10) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Smain Server is running on port %d\n", PORT);

    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len)) == -1) {
            perror("Accept failed");
            continue;
        }

        if (fork() == 0) {
            close(server_sock);
            handle_client(client_sock);
            close(client_sock);
            exit(0);
        } else {
            close(client_sock);
            waitpid(-1, NULL, WNOHANG); // Clean up child processes
        }
    }

    return 0;
}

void handle_client(int client_sock) {
    char buffer[BUFSIZE];
    ssize_t bytes_read;
    char filename[256], dest_path[256];

    while ((bytes_read = recv(client_sock, buffer, BUFSIZE, 0)) > 0) {
        buffer[bytes_read] = '\0';
        sscanf(buffer, "ufile %s %s", filename, dest_path);

        char *ext = strrchr(filename, '.');
        if (ext) {
            if (strcmp(ext, ".c") == 0) {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s/%s", dest_path, filename);
                int file_fd = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (file_fd == -1) {
                    perror("File open failed");
                    continue;
                }

                // Write the file contents (for simplicity, just a placeholder)
                char file_content[] = "Sample C file content";
                write(file_fd, file_content, strlen(file_content));
                close(file_fd);

                printf("Stored %s in Smain at %s\n", filename, full_path);
            } else if (strcmp(ext, ".pdf") == 0) {
                forward_file_to_server(filename, dest_path, "127.0.0.1", 8081); // Forward to Spdf
            } else if (strcmp(ext, ".txt") == 0) {
                forward_file_to_server(filename, dest_path, "127.0.0.1", 8082); // Forward to Stext
            }
        }
    }
}

void forward_file_to_server(const char *filename, const char *dest_path, const char *server_ip, int server_port) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFSIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connect failed");
        close(sock);
        return;
    }

    snprintf(buffer, sizeof(buffer), "store %s %s", filename, dest_path);
    send(sock, buffer, strlen(buffer), 0);
    close(sock);

    printf("Forwarded %s to %s server at %s\n", filename, server_ip, dest_path);
}
