#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>

#define BUFSIZE 1024
#define MAX_CLIENTS 10
#define WELCOME_MSG "Welcome to the chat server!\n"

typedef struct {
    int socket;
    char username[50];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
int active_connections = 0;

void print_active_connections() {
    printf("Active connections:\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0) {
            printf("Client %d: Socket descriptor %d, Username: %s\n", 
                   i + 1, clients[i].socket, clients[i].username);
        }
    }
    printf("Total active connections: %d\n", active_connections);
}

void send_to_all(int sender_socket, char *message, int message_size) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && clients[i].socket != sender_socket) {
            if (send(clients[i].socket, message, message_size, 0) == -1) {
                perror("send");
            }
        }
    }
}

void handle_client_message(int client_socket) {
    char recv_buf[BUFSIZE];
    int nbytes_recvd = recv(client_socket, recv_buf, BUFSIZE, 0);
    
    if (nbytes_recvd <= 0) {
        if (nbytes_recvd == 0) {
            printf("Client on socket %d disconnected\n", client_socket);
        } else {
            perror("recv");
        }
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == client_socket) {
                close(clients[i].socket);
                clients[i].socket = 0;
                memset(clients[i].username, 0, sizeof(clients[i].username));
                client_count--;
                active_connections--;
                break;
            }
        }
        print_active_connections();
    } else {
        recv_buf[nbytes_recvd] = '\0';
        
        // Cek apakah ini adalah pesan username
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == client_socket && strlen(clients[i].username) == 0) {
                strncpy(clients[i].username, recv_buf, sizeof(clients[i].username) - 1);
                clients[i].username[strcspn(clients[i].username, "\n")] = 0; // Remove newline
                printf("Username set for socket %d: %s\n", client_socket, clients[i].username);
                print_active_connections();
                
                // Kirim pesan konfirmasi ke client
                char confirm_msg[BUFSIZE];
                snprintf(confirm_msg, BUFSIZE, "Welcome, %s! You're now connected to the chat.\n", clients[i].username);
                send(client_socket, confirm_msg, strlen(confirm_msg), 0);
                
                return;
            }
        }
        
        // Jika bukan username, kirim pesan ke semua client
        printf("Received from socket %d: %s", client_socket, recv_buf);
        send_to_all(client_socket, recv_buf, nbytes_recvd);
    }
}

void connection_accept(int server_socket, fd_set *master_set, int *max_fd) {
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addrlen);
    
    if (new_socket == -1) {
        perror("accept");
    } else if (client_count < MAX_CLIENTS) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == 0) {
                clients[i].socket = new_socket;
                FD_SET(new_socket, master_set);
                if (new_socket > *max_fd) {
                    *max_fd = new_socket;
                }
                client_count++;
                active_connections++;
                printf("New connection from %s on port %d\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                printf("Assigned socket descriptor: %d\n", new_socket);
                send(new_socket, WELCOME_MSG, strlen(WELCOME_MSG), 0);
                
                // Meminta username
                char username_prompt[] = "Please enter your username: ";
                send(new_socket, username_prompt, strlen(username_prompt), 0);
                
                break;
            }
        }
    } else {
        printf("Max clients reached. Connection rejected.\n");
        close(new_socket);
    }
}

int main() {
    int server_socket, max_fd;
    struct sockaddr_in server_addr;
    fd_set master_set, read_fds;

    // Initialize client array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
        memset(clients[i].username, 0, sizeof(clients[i].username));
    }

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket");
        exit(1);
    }

    // Set up server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5555);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("Unable to bind");
        exit(1);
    }

    // Listen
    if (listen(server_socket, 10) == -1) {
        perror("listen");
        exit(1);
    }

    printf("\nTCP Chat Server is waiting for connections on port 5555\n\n");

    FD_ZERO(&master_set);
    FD_SET(server_socket, &master_set);
    max_fd = server_socket;

    while (1) {
        read_fds = master_set;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_socket) {
                    connection_accept(server_socket, &master_set, &max_fd);
                } else {
                    handle_client_message(i);
                }
            }
        }
    }

    close(server_socket);
    return 0;
}