#include <stdio.h>
#include <string.h>
#include <unistd.h>  // recv 함수 사용을 위해 필요
#include <stdlib.h>  // malloc과 free 함수 사용을 위해 필요 (옵션)

#define FILE_NAME_SIZE 64
#define FILE_PATH_SIZE 128

#define CHAT_LOG "chat_log.txt"
#define FILE_LOG "file_log.txt"

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
