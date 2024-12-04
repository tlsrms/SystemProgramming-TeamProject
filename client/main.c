#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "includes/common.h"
#include "includes/receive_handler.h"
#include "includes/send_handler.h"
#include "includes/file_monitor.h"

void enqueue(Packet packet);
int dequeue(Packet *packet);

int main() {
    int bytes_received;                

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
    Packet login_packet;
    char buffer[BUFFER_SIZE];
    if (file) { // user.txt 파일이 이미 있으면 저장된 유저 정보 사용
        fgets(username, sizeof(username), file);
        username[strcspn(username, "\n")] = '\0';
        // 서버에 이름 전송
        login_packet.flag = 1; // 로그인 플래그를 사용하지 않음으로 이미 회원가입이 되있음을 표시
        strncpy(login_packet.username, username, sizeof(login_packet.username));
        pthread_mutex_lock(&send_mutex);
        if (send(client_socket, &login_packet, sizeof(Packet), 0) < 0) {
            perror("Failed to send username");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_unlock(&send_mutex);
        fclose(file);
    } else {  // user.txt 파일이 없음 => 회원가입
        // 사용자 이름 입력
        while (1) {
            printf("Enter your username: ");
            fgets(username, sizeof(username), stdin);
            username[strcspn(username, "\n")] = '\0';

            // 서버에 이름 전송
            login_packet.flag = 0; // 로그인 플래그
            strncpy(login_packet.username, username, sizeof(login_packet.username));
            pthread_mutex_lock(&send_mutex);
            if (send(client_socket, &login_packet, sizeof(Packet), 0) < 0) {
                perror("Failed to send username");
                continue;
            }
            pthread_mutex_unlock(&send_mutex);

            // 서버 응답 대기
            bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_received < 0) {
                perror("Failed to receive data");
                exit(EXIT_FAILURE);
            }
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
    Packet received_packet;
    Packet chatLog_packet; // 불러온 채팅로그 저장할 패킷
    Packet sharedFile_packet; // 불러온 공유파일 저장할 패킷
    // 채팅 내역 수신
    bytes_received = recv(client_socket, &received_packet, sizeof(Packet), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive chat history");
        exit(EXIT_FAILURE);
    }
    if (received_packet.flag == 1) { // 채팅 메시지
        chatLog_packet = received_packet;
    }
    else {
        printf("Unexpected packet type for chat history\n");
    }

    // 공유 파일 수신
    bytes_received = recv(client_socket, &received_packet, sizeof(Packet), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive shared file");
        exit(EXIT_FAILURE);
    }
    if (received_packet.flag == 2) { // 파일 데이터
        sharedFile_packet = received_packet;
        printf("[File Update] Received file data\n");
        // 공유 파일에 파일 데이터를 저장
        pthread_mutex_lock(&file_mutex);
        FILE *shared_file = fopen(SHARED_FILE, "w");
        if (shared_file) {
            fwrite(sharedFile_packet.file_data, sizeof(char), strlen(sharedFile_packet.file_data), shared_file);
            fclose(shared_file);
        } else {
            perror("Failed to open shared file");
        }
        pthread_mutex_unlock(&file_mutex);
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
	pthread_t thread_inotify_file_and_send_packet; // 클라이언트 파일 변경을 감지하고 변경 시 파일을 전송하는 스레드
    pthread_create(&thread_inotify_file_and_send_packet, NULL, watch_file, NULL);
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
                apply_to_file(SHARED_FILE, &current_work);
            } else if(current_work.flag == 3) {
                //
                // version log 띄우기
                //
            } else {
                printf("Unknown packet type in queue\n");
            }
        }

        if(!keep_running){
            pthread_join(thread_receive_server_packet, NULL);
            pthread_join(thread_send_terminal_packet, NULL);
            pthread_join(thread_inotify_file_and_send_packet, NULL);
            pthread_mutex_destroy(&global_mutex);
            pthread_mutex_destroy(&send_mutex);
            pthread_mutex_destroy(&file_mutex);
            pthread_mutex_destroy(&queue_mutex);
            return 0;
        }
        usleep(10000); // 잠시 대기하여 CPU 사용률 감소 (GPT 추천인데 실제로 써봐야 알 거 같음)
    }
    //////////////////////////////////////////////////////////////////////////////////////////////

    return 0;
}