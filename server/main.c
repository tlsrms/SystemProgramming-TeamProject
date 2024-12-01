// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define BUFFER_SIZE 4096

void *handle_client(void *arg);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 서버 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("소켓 생성 실패");
        return 1;
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 소켓 바인드
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("바인드 실패");
        close(server_sock);
        return 1;
    }

    // 연결 대기
    if (listen(server_sock, 5) == -1) {
        perror("연결 대기 실패");
        close(server_sock);
        return 1;
    }

    printf("서버가 포트 %d에서 실행 중...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock == -1) {
            perror("클라이언트 연결 실패");
            continue;
        }

        pthread_t thread;
        int *pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_create(&thread, NULL, handle_client, pclient);
        pthread_detach(thread);
    }

    close(server_sock);
    return 0;
}
