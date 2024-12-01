#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define CHAT_LOG "chat_log.txt"
#define FILE_LOG "file_log.txt"
#define BUFFER_SIZE 4096

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

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("클라이언트 연결 종료\n");
            break;
        }
        buffer[bytes_received] = '\0';

        if (strcmp(buffer, "GET_CHAT_LOG") == 0) {
            send_chat_log(client_sock);
        } else if (strcmp(buffer, "GET_FILE_LOG") == 0) {
            send_file_log(client_sock);
        } else {
            pthread_mutex_lock(&file_mutex);
            FILE *log = fopen(CHAT_LOG, "a");
            if (log) {
                fprintf(log, "%s\n", buffer);
                fclose(log);
            }
            pthread_mutex_unlock(&file_mutex);
        }
    }

    close(client_sock);
    return NULL;
}
