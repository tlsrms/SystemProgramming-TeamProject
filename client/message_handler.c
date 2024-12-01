#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

void receive_log(int sock) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (strcmp(buffer, "END_OF_CHAT_LOG\n") == 0 || strcmp(buffer, "END_OF_FILE_LOG\n") == 0) break;
        printf("%s", buffer);
    }
}
