#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "includes/message_handler.h"

extern int server_sock; // 서버 소켓 파일 디스크립터 (main.c에서 정의)


/**
 * @brief 서버로부터 메시지를 수신하고 처리하는 스레드 함수
 * 
 * 
 * @param arg          스레드 실행 시 전달받는 매개변수. 
 *                     - 이 함수에서는 사용되지 않음(NULL 처리).
 */
void *message_listener(void *arg) {
    Packet packet; // 서버에서 수신할 데이터를 저장할 패킷

    while (1) {
        // 서버로부터 데이터 수신
        int bytes_received = recv(server_sock, &packet, sizeof(Packet), 0);
        if (bytes_received <= 0) {
            // 수신 실패 또는 연결 종료 시 처리
            printf("[Client] Disconnected from server.\n");
            exit(EXIT_FAILURE); // 프로그램 종료
        }

        // 수신한 패킷의 내용을 출력
        printf("[%s]: %s\n", packet.username, packet.message);
    }

    return NULL; // 종료될 일이 없지만 함수의 형태를 맞추기 위해 반환
}


/**
 * @brief 서버로 채팅 메시지를 전송하는 함수.
 *
 * 
 * @param server_sock  서버와 연결된 소켓 파일 디스크립터.
 *                     - 서버로 데이터를 전송하기 위해 사용
 * 
 * @param username     메시지를 전송하는 사용자의 이름.
 *                     - `Packet` 구조체의 `username` 필드에 저장
 * 
 * @param message      사용자가 입력한 채팅 메시지 내용.
 *                     - `Packet` 구조체의 `message` 필드에 저장
 */
void send_chat_message(int server_sock, const char *username, const char *message) {
    Packet packet;
    memset(&packet, 0, sizeof(Packet));
    packet.flag = 1;
    strncpy(packet.username, username, sizeof(packet.username));
    strncpy(packet.message, message, sizeof(packet.message));

    if (send(server_sock, &packet, sizeof(Packet), 0) < 0) {
        perror("[Client] Failed to send message");
    }
}


