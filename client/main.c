#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "includes/common.h"

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char username[50];

    // user.txt 확인
    FILE *file = fopen(USER_FILE, "r");
    if (file) { // user.txt 파일이 이미 있으면 저장된 유저 정보 사용
        fgets(username, sizeof(username), file);
        username[strcspn(username, "\n")] = '\0';
        fclose(file);
    } else {  // user.txt 파일이 없음 => 회원가입
        // 사용자 이름 입력
        while (1) {
            printf("Enter your username: ");
            fgets(username, sizeof(username), stdin);
            username[strcspn(username, "\n")] = '\0';

            // 서버에 이름 전송
            send(sockfd, username, strlen(username), 0);

            // 서버 응답 대기
            int bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
            buffer[bytes_received] = '\0';

            if (strcmp(buffer, "REGISTERED") == 0) { // 등록 성공 응답 수신
                printf("Registration successful.\n");
                file = fopen(USER_FILE, "w");
                if (file) {
                    fprintf(file, "%s\n", username);
                    fclose(file);
                }
                break;
            } else if (strcmp(buffer, "DUPLICATE") == 0) { // 등록 반려
                printf("Username already exists. Try another.\n");
            } else {
                printf("Error communicating with server.\n");
            }
        }
    }

    // 서버 연결
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    send(sockfd, username, strlen(username), 0);

    while (1) {
        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);
        send(sockfd, buffer, strlen(buffer), 0);
    }

    close(sockfd);
    return 0;
}
