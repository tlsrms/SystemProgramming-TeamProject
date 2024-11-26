#include <stdio.h>
#include "includes/common.h"

int is_user_registered(const char *username) { // 이미 등록된 이름인지 확인
    FILE *file = fopen(USER_FILE, "r");
    if (!file) return 0;

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, username) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

void register_user(const char *username) { // 입력한 이름으로 회원가입
    FILE *file = fopen(USER_FILE, "a");
    if (file) {
        fprintf(file, "%s\n", username);
        fclose(file);
    }
}

void remove_client(int sockfd) { // 클라이언트 연결 해제 및 배열 관리
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; i++) {
        if (clients[i].sockfd == sockfd) {
            printf("[Server] Client %s disconnected.\n", clients[i].username);

			close(sockfd);

            // 연결 끊긴 클라이언트 이후의 모든 클라이언트를 앞으로 이동
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }

            client_count--;
            break;
        }
    }

    printf("[Server] Remaining clients: %d\n", client_count);
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) { // 클라이언트 핸들러
	Client client = *(Client *)arg;
	char buffer[BUFFER_SIZE];
	char username[50];

    // 클라이언트로부터 사용자 이름을 등록받음
    while (1) {
        int bytes_received = recv(client.sockfd, username, sizeof(username) - 1, 0);
        if (bytes_received <= 0) {
            close(client.sockfd);
            return NULL;
        }

        username[bytes_received] = '\0'; // 개행 문자 제거

        pthread_mutex_lock(&clients_mutex);
        if (is_user_registered(username)) { // 이미 등록된 이름 입력 시 반려
            send(client.sockfd, "DUPLICATE", 9, 0);
        } else { // 회원가입 후 클라이언트에게 성공 메세지 전송
            strcpy(client.username, username);
            register_user(username);
            send(client.sockfd, "REGISTERED", 10, 0); // 등록 성공 응답
            printf("[Server] New user registered: %s\n", username);
            pthread_mutex_unlock(&clients_mutex);
            break;
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    while (1) { // 연결 종료 감지 시 클라이언트 해제
        int bytes_received = recv(client.sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
			remove_client(client.sockfd);
            break;
        }
    }

    return NULL;
}