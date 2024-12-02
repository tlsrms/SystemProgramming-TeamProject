#include <pthread.h>
#include "includes/common.h"

/**
 * @brief 모든 클라이언트에게 패킷을 브로드캐스트하는 함수.
 *
 * @param packet       전송할 데이터가 담긴 Packet 구조체의 포인터.
 *                     - flag: 데이터 유형 플래그 (예: 1=채팅, 2=파일 동기화 등)
 *                     - username: 전송자 이름
 *                     - message/file_data: 메시지나 파일 데이터
 * @param clients      현재 연결된 클라이언트 정보를 담은 배열.
 *                     - sockfd: 클라이언트 소켓 파일 디스크립터
 *                     - username: 클라이언트 사용자 이름
 *                     - thread: 클라이언트와 관련된 스레드 정보
 * @param client_count 현재 연결된 클라이언트의 수.
 * @param exclude_sock 브로드캐스트에서 제외할 클라이언트의 소켓 파일 디스크립터
 *                     - 일반적으로 메시지를 보낸 클라이언트의 소켓 번호
 */
void broadcast_packet(Packet *packet, Client clients[], int client_count, int exclude_sock) {
	// 자기 자신을 제외한 연결된 모든 클라이언트에게 데이터 전송
    for (int i = 0; i < client_count; i++) {
        if (clients[i].sockfd != exclude_sock) {
            send(clients[i].sockfd, packet, sizeof(Packet), 0);
        }
    }
}

