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

#define PORT 8080
#define BUFFER_SIZE 1024

void ufile(int socket, char *filename, char *dest_path) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "ufile %s %s", filename, dest_path);

    // Send the command to the server
    write(socket, buffer, sizeof(buffer));

    // Open the file and send its contents
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("File open failed");
        exit(EXIT_FAILURE);
    }

    // Send the file contents in chunks
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        write(socket, buffer, bytes_read);
    }

    close(fd);
    printf("File %s sent to server\n", filename);
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    while (1) {
        printf("Enter command: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        if (strncmp(buffer, "ufile", 5) == 0) {
            char filename[BUFFER_SIZE], dest_path[BUFFER_SIZE];
            sscanf(buffer, "ufile %s %s", filename, dest_path);
            ufile(client_socket, filename, dest_path);
        } else {
            printf("Invalid command\n");
        }
    }

    close(client_socket);
    return 0;
}
