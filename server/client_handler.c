#include <stdio.h>      // fopen(), fgets(), fclose(), printf() 등의 함수 사용을 위해 필요
#include <string.h>     // strlen() 등의 함수 사용을 위해 필요
#include <unistd.h>     // send() 함수 사용을 위해 필요
#include <pthread.h>    // pthread_mutex_t, pthread_mutex_lock, pthread_mutex_unlock 등을 사용하기 위해 필요

#define FILE_NAME_SIZE 64
#define FILE_PATH_SIZE 128
#define BUFFER_SIZE 4096
#define CHAT_LOG "chat_log.txt"
#define FILE_LOG "file_log.txt"

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_chat_log(int client_sock) {
    pthread_mutex_lock(&file_mutex);

    FILE *log = fopen(CHAT_LOG, "r");  
    char line[BUFFER_SIZE];
    if (log) {
        while (fgets(line, sizeof(line), log)) {
            send(client_sock, line, strlen(line), 0);
        }
        fclose(log);
    }
    send(client_sock, "END_OF_CHAT_LOG\n", 17, 0);

    pthread_mutex_unlock(&file_mutex);
}

void send_file_log(int client_sock) {
    pthread_mutex_lock(&file_mutex);

    FILE *log = fopen(FILE_LOG, "r");  
    char line[BUFFER_SIZE];
    if (log) {
        while (fgets(line, sizeof(line), log)) {
            send(client_sock, line, strlen(line), 0);
        }
        fclose(log);
    }
    send(client_sock, "END_OF_FILE_LOG\n", 17, 0);

    pthread_mutex_unlock(&file_mutex);
}
