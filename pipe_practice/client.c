#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const char* host = "0.0.0.0";
int port = 6000;

int main()
{
    int sock_fd;
    struct sockaddr_in serv_name;
    int status;
    char indata[1024] = { 0 }, outdata[1024] = { 0 };

    // create a socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Socket creation error");
        exit(1);
    }

    // server address
    serv_name.sin_family = AF_INET;
    inet_aton(host, &serv_name.sin_addr);
    serv_name.sin_port = htons(port);

    status = connect(sock_fd, (struct sockaddr*)&serv_name, sizeof(serv_name));
    if (status == -1) {
        perror("Connection error");
        exit(1);
    }

    while (1) {
        printf("please input a string and a number separated by a space to do caesar encryption : \n");
        memset(indata, 0, sizeof(indata));
        memset(outdata, 0, sizeof(outdata));
        fgets(outdata, sizeof(outdata), stdin);
        int i;
        for (i = 0; i < strlen(outdata); i++) {
            if (outdata[i] == ' ') {
                break;
            }
            if (!isalpha(outdata[i])) {
                printf("invalid input alpha\n");
                break;
            }
        }
        for (i++; i < strlen(outdata); i++) {
            if (outdata[i] == '\n') {
                break;
            }
            if (!isnumber(outdata[i])) {
                printf("%c\n", outdata[i]);
                printf("invalid input\n");
                break;
            }
        }
        if (send(sock_fd, outdata, strlen(outdata), 0) < 0) {
            perror("send error");
            exit(1);
        }
        
        int nbytes = recv(sock_fd, indata, sizeof(indata), 0);
        if (nbytes <= 0) {
            close(sock_fd);
            printf("server closed connection.\n");
            break;
        }
        printf("recv: %s\n", indata);
    }

    return 0;
}