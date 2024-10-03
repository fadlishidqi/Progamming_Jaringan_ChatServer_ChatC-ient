// Fadli Shidqi Firdaus
// 21120122140166
// Progjar Kelas D

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

void send_message(int sockfd, char *username) {
    char send_buf[BUFSIZE];
    char full_message[BUFSIZE + 50];

    fgets(send_buf, BUFSIZE, stdin);
    if (strcmp(send_buf, "quit\n") == 0) {
        close(sockfd);
        exit(0);
    }

    snprintf(full_message, sizeof(full_message), "%s: %s", username, send_buf);
    if (send(sockfd, full_message, strlen(full_message), 0) == -1) {
        perror("send");
    }
}

void receive_message(int sockfd) {
    char recv_buf[BUFSIZE];
    int nbytes_recvd = recv(sockfd, recv_buf, BUFSIZE, 0);
    
    if (nbytes_recvd <= 0) {
        if (nbytes_recvd == 0) {
            printf("Server disconnected.\n");
        } else {
            perror("recv");
        }
        close(sockfd);
        exit(1);
    }

    recv_buf[nbytes_recvd] = '\0';
    printf("%s", recv_buf);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    int sockfd;
    struct sockaddr_in server_addr;
    fd_set master_set, read_fds;
    char username[50];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Set up server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5555);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server. Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

    FD_ZERO(&master_set);
    FD_SET(0, &master_set);
    FD_SET(sockfd, &master_set);

    printf("Welcome, %s! You can start chatting. Type 'quit' to exit.\n", username);

    while (1) {
        read_fds = master_set;
        if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        if (FD_ISSET(0, &read_fds)) {
            send_message(sockfd, username);
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            receive_message(sockfd);
        }
    }

    close(sockfd);
    return 0;
}