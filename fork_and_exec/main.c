#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    int pid = fork();
    if (pid == 0) {
        // child process
        execl("/bin/ls", "ls", "-l", NULL);
    } else {
        // parent process
        wait(NULL);
    }
    return 0;
}