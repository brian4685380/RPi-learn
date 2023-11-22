#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

enum fd_type {
    FROM_CLIENT,
    FROM_CHILD,
    NONE
};

#define OPEN_MAX 1024

const char* host = "0.0.0.0";
int port = 6000;

int main() {
    enum fd_type fd_map[OPEN_MAX] = { NONE };
    int owner_of_pipe[OPEN_MAX] = { 0 };
    int sock_fd, new_fd;
    socklen_t addrlen;
    struct sockaddr_in my_addr, client_addr;
    int status;
    char data_from_client[1024], encrypted_data[1024], data_from_child[1024];
    int on = 1;
    ssize_t n;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed\n");
        exit(1);
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        perror("setsockopt error\n");
        exit(1);
    }

    my_addr.sin_family = AF_INET;
    inet_aton(host, &my_addr.sin_addr);
    my_addr.sin_port = htons(port);

    if ((status = bind(sock_fd, (struct sockaddr*)&my_addr, sizeof(my_addr))) < 0) {
        perror("bind failed\n");
        exit(1);
    }
    printf("server start at %s:%d\n", inet_ntoa(my_addr.sin_addr), port);
    if ((status = listen(sock_fd, 10)) < 0) {
        perror("listen error\n");
        exit(1);
    }
    printf("waiting for connection...\n");

    addrlen = sizeof(client_addr);

    struct pollfd fds[OPEN_MAX];
    fds[0].fd = sock_fd;
    fds[0].events = POLLIN;
    fd_map[0] = FROM_CLIENT;
    for (int i = 1; i < OPEN_MAX; i++) {
        fds[i].fd = -1;
    }
    int nfds = 1;

    while (1) {
        if (poll(fds, OPEN_MAX, -1) < 0) {
            perror("poll error\n");
            exit(1);
        }
        // printf("polling...\n");
        for (int i = 0; i <= nfds; i++) {
            // printf("i = %d\n", i);
            if (fds[i].revents & (POLL_IN | POLLERR)) {
                if (i == 0) {
                    if ((new_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &addrlen)) < 0) {
                        perror("accept error\n");
                        exit(1);
                    }
                    printf("connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    for (int i = 1; i <= OPEN_MAX; i++) {
                        if (i == OPEN_MAX) {
                            perror("too many clients\n");
                            exit(1);
                        }
                        if (fds[i].fd < 0) {
                            printf("client[%d] connected\n", i);
                            fds[i].fd = new_fd;
                            fds[i].events = POLLIN;
                            fd_map[i] = FROM_CLIENT;
                            if (i > nfds) {
                                nfds = i;
                            }
                            break;
                        }
                    }
                }
                else {
                    if (fd_map[i] == FROM_CLIENT) {
                        printf("client[%d] send data\n", i);
                        memset(data_from_client, 0, sizeof(data_from_client));
                        memset(encrypted_data, 0, sizeof(encrypted_data));
                        if ((n = recv(fds[i].fd, data_from_client, sizeof(data_from_client), 0)) < 0) {
                            if (errno == ECONNRESET) {
                                printf("client[%d] aborted connection\n", i);
                                close(fds[i].fd);
                                fds[i].fd = -1;
                                fd_map[i] = NONE;
                            }
                            else {
                                perror("read error\n");
                                exit(1);
                            }
                        }
                        else if (n == 0) {
                            printf("client[%d] closed connection\n", i);
                            close(fds[i].fd);
                            fds[i].fd = -1;
                            fd_map[i] = NONE;
                        }
                        else {
                            printf("recv: %s from client %d\n", data_from_client, i);
                            int data_to_child[2];
                            int data_from_child[2];
                            if (pipe(data_to_child) < 0) {
                                perror("pipe to child error\n");
                                exit(1);
                            }
                            if (pipe(data_from_child) < 0) {
                                perror("pipe from child error\n");
                                exit(1);
                            }
                            write(data_to_child[1], data_from_client, sizeof(data_from_client));
                            pid_t pid;
                            if ((pid = fork()) == 0) {
                                dup2(data_to_child[0], STDIN_FILENO);
                                dup2(data_from_child[1], STDOUT_FILENO);
                                close(data_to_child[0]);
                                close(data_to_child[1]);
                                close(data_from_child[0]);
                                close(data_from_child[1]);
                                if (execl("./encrypt", "encrypt", NULL) < 0) {
                                    perror("execl error\n");
                                    exit(1);
                                }
                                exit(0);
                            }
                            else {
                                close(data_to_child[0]);
                                close(data_from_child[1]);
                                for (int j = 1; j <= OPEN_MAX; j++) {
                                    if (j == OPEN_MAX) {
                                        perror("too many clients\n");
                                        exit(1);
                                    }
                                    if (fds[j].fd < 0) {
                                        fds[j].fd = data_from_child[0];
                                        fds[j].events = POLLIN;
                                        fd_map[j] = FROM_CHILD;
                                        owner_of_pipe[j] = i;
                                        if (j > nfds) {
                                            nfds = j;
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    else if (fd_map[i] == FROM_CHILD) {
                        memset(data_from_child, 0, sizeof(data_from_child));
                        read(fds[i].fd, data_from_child, sizeof(data_from_child));
                        printf("recv: %s from child pipe %d\n", data_from_child, i);
                        char result[1024] = "";
                        snprintf(result, sizeof(result), "encrypted data: %s", data_from_child);
                        write(fds[owner_of_pipe[i]].fd, result, sizeof(result));
                        owner_of_pipe[i] = 0;
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        fd_map[i] = NONE;
                    }
                }
            }
        }
    }
}