#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main () {
    char to_be_separate[1024];
    if (fgets(to_be_separate, 1024, stdin) == NULL) {
        perror("fgets error\n");
        exit(1);
    }
    char * message = strtok(to_be_separate, " ");
    char * key = strtok(NULL, " ");

    int num = atoi(key);
    for (int i = 0; i < strlen(message); i++) {
        if (message[i] >= 'a' && message[i] <= 'z') {
            message[i] = (message[i] - 'a' + num) % 26 + 'a';
        } else if (message[i] >= 'A' && message[i] <= 'Z') {
            message[i] = (message[i] - 'A' + num) % 26 + 'A';
        }
    }
    printf("%s", message);
}