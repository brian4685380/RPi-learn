#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h> // 添加 poll 头文件

const char* host = "0.0.0.0";
int port = 5435;

int main() {
    int sock_fd;
    socklen_t addrlen;
    struct sockaddr_in my_addr, client_addr;
    int status;
    char indata[1024] = { 0 }, outdata[1024] = { 0 };
    int on = 1;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Socket creation error");
        exit(1);
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1) {
        perror("Setsockopt error");
        exit(1);
    }

    my_addr.sin_family = AF_INET;
    inet_aton(host, &my_addr.sin_addr);
    my_addr.sin_port = htons(port);

    status = bind(sock_fd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    if (status == -1) {
        perror("Binding error");
        exit(1);
    }
    printf("server start at: %s:%d\n", inet_ntoa(my_addr.sin_addr), port);

    status = listen(sock_fd, 5);
    if (status == -1) {
        perror("Listening error");
        exit(1);
    }
    printf("wait for connection...\n");

    addrlen = sizeof(client_addr);

    struct pollfd fds[1024]; // 定义 pollfd 数组
    fds[0].fd = sock_fd;     // 将监听套接字添加到数组中
    fds[0].events = POLLIN; // 监听可读事件

    int nfds = 1; // 目前监视的文件描述符数量

    while (1) {
        int poll_status = poll(fds, nfds, -1);
        if (poll_status == -1) {
            perror("Poll error");
            exit(1);
        }

        // 遍历检查所有文件描述符
        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == sock_fd) {
                    // 有新的客户端连接
                    int new_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &addrlen);
                    printf("connected by %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    // 将新的客户端连接添加到监视列表
                    fds[nfds].fd = new_fd;
                    fds[nfds].events = POLLIN;
                    nfds++;
                }
                else {
                    // 有数据可读
                    memset(indata, 0, sizeof(indata));
                    int nbytes = recv(fds[i].fd, indata, sizeof(indata), 0);
                    if (nbytes <= 0) {
                        // 客户端关闭连接
                        close(fds[i].fd);
                        printf("client closed connection.\n");
                        // 从监视列表中移除该客户端
                        for (int j = i; j < nfds - 1; j++) {
                            fds[j].fd = fds[j + 1].fd;
                            fds[j].events = fds[j + 1].events;
                        }
                        nfds--;
                    }
                    else {
                        printf("recv: %s\n", indata);
                        snprintf(outdata, sizeof(outdata), "echo %s", indata);
                        send(fds[i].fd, outdata, strlen(outdata), 0);
                    }
                }
            }
        }
    }
}
