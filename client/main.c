#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "includes/common.h"
#include "includes/version_control.h"

#define PORT 8080
#define USER_FILE "user.txt"

int client_socket;
char username[50];

// 작업 큐 정의
#define QUEUE_SIZE 100
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
Packet packet_queue[QUEUE_SIZE];
int front = 0;
int rear = 0;

void enqueue(Packet packet) {
    pthread_mutex_lock(&queue_mutex);
    if ((rear + 1) % QUEUE_SIZE == front) {
        printf("Queue is full\n");
    } else {
        packet_queue[rear] = packet;
        rear = (rear + 1) % QUEUE_SIZE;
    }
    pthread_mutex_unlock(&queue_mutex);
}

int dequeue(Packet *packet) {
    pthread_mutex_lock(&queue_mutex);
    if (front == rear) {
        pthread_mutex_unlock(&queue_mutex);
        return 0;
    }
    *packet = packet_queue[front];
    front = (front + 1) % QUEUE_SIZE;
    pthread_mutex_unlock(&queue_mutex);
    return 1;
}

void *receive_server_packet(void *arg) {
    Packet packet;
    while (1) {
        int bytes_received = recv(client_socket, &packet, sizeof(Packet), 0);
        if (bytes_received <= 0) {
            printf("[Client] Disconnected from server.\n");
            exit(EXIT_FAILURE);
        }
        enqueue(packet);
    }
    return NULL;
}

void *send_terminal_packet(void *arg) {
    Packet packet;
    while (1) {
        char input[BUFFER_SIZE];
        printf("> ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        if (strncmp(input, "/commit", 7) == 0) {
            commit_version();
        } else if (strncmp(input, "/log", 4) == 0) {
            log_versions();
        } else if (strncmp(input, "/rebase", 7) == 0) {
            int version_number = atoi(&input[8]);
            rebase_version(version_number);
        } else {
            packet.flag = 1; // 채팅 메시지 플래그
            strncpy(packet.username, username, sizeof(packet.username));
            strncpy(packet.message, input, sizeof(packet.message));
            send(client_socket, &packet, sizeof(Packet), 0);
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;
    int bytes_received;

    // 1번 소켓 연결
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 임시 IP

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // 2번 thread [서버 패킷 수신] 생성
    pthread_t thread_receive_server_packet;
    pthread_create(&thread_receive_server_packet, NULL, receive_server_packet, NULL);

    // 3번 로그인
    FILE *file = fopen(USER_FILE, "r");
    Packet login_packet;
    char buffer[BUFFER_SIZE];
    if (file) {
        fgets(username, sizeof(username), file);
        username[strcspn(username, "\n")] = '\0';
        fclose(file);
    } else {
        while (1) {
            printf("Enter your username: ");
            fgets(username, sizeof(username), stdin);
            username[strcspn(username, "\n")] = '\0';

            login_packet.flag = 0;
            strncpy(login_packet.username, username, sizeof(login_packet.username));
            if (send(client_socket, &login_packet, sizeof(Packet), 0) < 0) {
                perror("Failed to send username");
                continue;
            }

            bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_received < 0) {
                perror("Failed to receive data");
                exit(EXIT_FAILURE);
            }
            buffer[bytes_received] = '\0';

            if (strcmp(buffer, "REGISTERED") == 0) {
                printf("Registration successful.\n");
                file = fopen(USER_FILE, "w");
                if (file) {
                    fprintf(file, "%s\n", username);
                    fclose(file);
                }
                break;
            } else if (strcmp(buffer, "DUPLICATE") == 0) {
                printf("Username already exists. Try another.\n");
            } else {
                printf("Error communicating with server.\n");
            }
        }
    }

    // 4번 채팅내역 & 공유파일 불러오기
    Packet received_packet;
    Packet chatLog_packet;
    Packet sharedFile_packet;

    // 채팅 내역 수신
    bytes_received = recv(client_socket, &received_packet, sizeof(Packet), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive chat history");
        exit(EXIT_FAILURE);
    }
    if (received_packet.flag == 1) {
        chatLog_packet = received_packet;
    } else {
        printf("Unexpected packet type for chat history\n");
    }

    // 공유 파일 수신
    bytes_received = recv(client_socket, &received_packet, sizeof(Packet), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive shared file");
        exit(EXIT_FAILURE);
    }
    if (received_packet.flag == 2) {
        sharedFile_packet = received_packet;
        printf("[File Update] Received file data\n");
        FILE *shared_file = fopen(SHARED_FILE, "w");
        if (shared_file) {
            fwrite(sharedFile_packet.file_data, sizeof(char), strlen(sharedFile_packet.file_data), shared_file);
            fclose(shared_file);
        } else {
            perror("Failed to open shared file");
        }
    } else {
        printf("Unexpected packet type for shared file\n");
    }

    // 6번 thread [터미널 입력 및 패킷 발신] 생성
    pthread_t thread_send_terminal_packet;
    pthread_create(&thread_send_terminal_packet, NULL, send_terminal_packet, NULL);

    // 8번 메인 스레드 작업
    Packet current_work;
    while (1) {
        if (dequeue(&current_work)) {
            if (current_work.flag == 1) {
                printf("[%s]: %s\n", current_work.username, current_work.message);
            } else if (current_work.flag == 2) {
                printf("[File Update] Applying file data update\n");
                FILE *shared_file = fopen(SHARED_FILE, "w");
                if (shared_file) {
                    fwrite(current_work.file_data, sizeof(char), strlen(current_work.file_data), shared_file);
                    fclose(shared_file);
                } else {
                    perror("Failed to open shared file");
                }
            } else {
                printf("Unknown packet type in queue\n");
            }
        }
        usleep(10000);
    }

    return 0;
}
