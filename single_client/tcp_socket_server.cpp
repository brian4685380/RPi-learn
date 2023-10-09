#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


const char* host = "0.0.0.0";
int port = 5435;


int main() { 
    int sock_fd, new_fd;
    socklen_t addrlen;
    struct sockaddr_in my_addr, client_addr;
    int status;
    char indata[1024] = {0}, outdata[1024] = {0};
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
    inet_aton(host, &my_addr.sin_addr); // translate host in 10-base to binary address format store at my_addr.sin_addr
    my_addr.sin_port = htons(port);  // translate the port into my_addr.sin_port

    status = bind(sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr)); // bind the socket with the address
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

    while (1) {
        new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addrlen);
        printf("connected by %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        while (1) {
			memset(indata, 0, sizeof(indata)); // 清空 indata 数组
			memset(indata, 0, sizeof(indata));
            int nbytes = recv(new_fd, indata, sizeof(indata), 0);
            if (nbytes <= 0) {
                close(new_fd);
                printf("client closed connection.\n");
                break;
            }
            printf("recv: %s\n", indata);

            snprintf(outdata, sizeof(outdata), "echo %s", indata);
            send(new_fd, outdata, strlen(outdata), 0);
        }

    }
}