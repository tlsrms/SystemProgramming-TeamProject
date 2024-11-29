#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "includes/common.h"

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

int main() {
	struct sockaddr_in server_addr;			// 서버 주소 저장
    int bytes_received;                     // 데이터 수신 크기

    ////////////////////////////////// 1번 소켓 연결 //////////////////////////////////////////
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //임시 IP

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    //////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////// 2번 thread [서버 패킷 수신] 생성 ////////////////////////////////
	pthread_t thread_receive_server_packet; // 서버로부터 패킷을 받는 스레드
    pthread_create(&thread_receive_server_packet, NULL, receive_server_packet, NULL);
    //////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////// 3번 로그인 /////////////////////////////////////////////
    FILE *file = fopen(USER_FILE, "r");
    char buffer[BUFFER_SIZE];
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
            send(client_socket, username, strlen(username), 0);

            // 서버 응답 대기
            bytes_received = recv(client_socket, buffer, sizeof(BUFFER_SIZE), 0);
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
    //////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////// 4번 채팅내역 & 공유파일 불러오기 ///////////////////////////////
    Packet chatLog_and_sharedFile[2];
    // 채팅 내역 수신
    bytes_received = recv(client_socket, &chatLog_and_sharedFile[0], sizeof(Packet), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive chat history");
        exit(EXIT_FAILURE);
    }
    if (chatLog_and_sharedFile[0].flag != 1) { // 채팅 메시지
        printf("Unexpected packet type for chat history\n");
    }

    // 공유 파일 수신
    bytes_received = recv(client_socket, &chatLog_and_sharedFile[1], sizeof(Packet), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive shared file");
        exit(EXIT_FAILURE);
    }
    if (chatLog_and_sharedFile[1].flag == 2) { // 파일 데이터
        printf("[File Update] Received file data:\n%s\n", packet.file_data);
        // 공유 파일에 파일 데이터를 저장
        FILE *shared_file = fopen(SHARED_FILE, "w");
        if (shared_file) {
            fwrite(packet.file_data, sizeof(char), strlen(packet.file_data), shared_file);
            fclose(shared_file);
        } else {
            perror("Failed to open shared file");
        }
    } else {
        printf("Unexpected packet type for shared file\n");
    }
    //////////////////////////////////////////////////////////////////////////////////////////////
    
    //////////////////////// 5번 불러온 채팅내역 & 공유파일 UI로 표시 ////////////////////////////
    //
    // UI 띄우기
    //
    //////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////// 6번 thread [터미널 입력 및 패킷 발신] 생성 /////////////////////////
	pthread_t thread_send_terminal_packet;	// 서버로 터미널 입력 패킷을 보내는 스레드
    pthread_create(&thread_send_terminal_packet, NULL, send_terminal_packet, NULL);
    //////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////// 7번 thread [클라이언트 파일 변경 감지 및 파일 전송] 생성 /////////////////
    //
    // 스레드 생성
    //
    //////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////// 8번 메인 스레드 작업 ////////////////////////////////////
    Packet current_work;
    while (1) {
        // 작업 큐에서 패킷을 읽고 채팅인지 파일인지 확인
        if (dequeue(&current_work)) {
            if (current_work.flag == 1) { // 채팅 메시지
                // 
                // 채팅 UI 띄우기
                //
            } else if (current_work.flag == 2) { // 파일 데이터
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
        usleep(10000); // 잠시 대기하여 CPU 사용률 감소 (GPT 추천인데 실제로 써봐야 알 거 같음)
    }
    //////////////////////////////////////////////////////////////////////////////////////////////

    return 0;
}
