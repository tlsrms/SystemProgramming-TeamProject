#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "includes/chat_handler.h"
#include "includes/broadcast.h"

extern pthread_mutex_t file_mutex;


/**
 * @brief 클라이언트로부터 받은 채팅 메시지를 처리하는 함수.
 * 
 * @param packet       클라이언트로부터 수신한 데이터 패킷의 포인터.
 *                     - `username`: 메시지를 보낸 사용자 이름.
 *                     - `message`: 보낸 채팅 메시지.
 * @param sender_sock  메시지를 보낸 클라이언트의 소켓 파일 디스크립터.
 *                     - 이 소켓은 브로드캐스트 시 제외 대상이 됨.
 */
void handle_chat_message(Packet *packet, int sender_sock) {
    save_chat_log(packet->username, packet->message); // 채팅 로그 저장 
    broadcast_packet(packet, clients, client_count, sender_sock); // 다른 클라이언트에게 채팅 보내기
}


/**
 * @brief 채팅 로그를 파일에 저장하는 함수.
 * 
 * @param username 메시지를 보낸 사용자의 이름.
 *                 - 로그에 "[username]: message" 형식으로 기록.
 * @param message  저장할 채팅 메시지 내용.
 */
void save_chat_log(const char *username, const char *message) {
    pthread_mutex_lock(&file_mutex); // lock, 파일 동시 접근 방지
    FILE *log = fopen(CHAT_LOG, "a"); // 추가 모드(a)로 파일 열기
    if (log) {
        fprintf(log, "%s, %s\n", username, message);
        fclose(log);
    }
    pthread_mutex_unlock(&file_mutex); // unlock
}

