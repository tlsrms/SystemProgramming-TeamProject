#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include "includes/common.h"
#include <sys/socket.h>
#include <unistd.h>

int initialize_server(int port, struct sockaddr_in *server_addr);
void enqueue(Packet packet);
int dequeue(Packet *packet);
void* accept_socket(void *);

Client temp_client;

int main() {
    // 서버 초기화
    server_fd = initialize_server(PORT, &server_addr);
    if (server_fd < 0) {
        fprintf(stderr, "[Error] Server initialization failed.\n");
        return EXIT_FAILURE;
    }
    printf("[Server] Listening on port %d...\n", PORT);

	pthread_t accept_ClientSocket;
	pthread_create(&accept_ClientSocket, NULL, accept_socket, NULL);

    Packet current_work;
    while (1) {
        // 작업 큐에서 패킷을 읽고 채팅인지 파일인지 확인
        if (dequeue(&current_work)) {
			if(current_work.flag == 0){
				//
				//로그인 (중복 체크 후 응답 전송(성공 시 채팅 내역, 공유 파일 전송), Client 구조체 생성 및 배열에 추가) <- 1번
				//
			}
            else if (current_work.flag == 1) { // 채팅 메시지
                // 
                // 채팅 log 저장 + 채팅 broadcast <- 3번
                //
            } else if (current_work.flag == 2) { // 파일 데이터
				//
				// 파일 내역 읽고 shared_file.txt에 반영 + 파일 broadcast <- 2번
				//
        	}
			else if(current_work.flag == 3){
				//
				// '/commit', '/log', '/rebase' 명령어 처리 <- 4번
				//
			}
			else if(NULL){ //ex) 클라이언트가 아무도 없고 일정 시간 경과
    			close(server_fd);
    			return 0;
			}
        usleep(10000); // 잠시 대기하여 CPU 사용률 감소 (GPT 추천인데 실제로 써봐야 알 거 같음)
    	}
	}

}
// 서버 초기화
int initialize_server(int port, struct sockaddr_in *server_addr) {
    int server_fd;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[Server] Socket creation failed");
        return -1;
    }

    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = INADDR_ANY;
    server_addr->sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("[Server] Bind failed");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("[Server] Listen failed");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

void* accept_socket(void *arg){
	int new_sock;
	struct sockaddr_in client_addr;
	socklen_t addr_size = sizeof(client_addr);
	
	while(1){
		new_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
		if (new_sock < 0) {
			// [Server] accept 호출 실패 시 오류 메시지 출력
			perror("[Server] Accept failed");
			break; // accept 실패 시 반복문을 종료함
		}

		// 클라이언트 소켓을 처리하기 위한 새로운 스레드 생성
		pthread_t client_thread;
		if (pthread_create(&client_thread, NULL, receive_packet, (void *)(intptr_t)new_sock) != 0) {
			perror("[Server] Failed to create client thread");
			close(new_sock);
			continue;
		}
		temp_client.thread = client_thread; // 클라이언트 패킷 받음 스레드를 메인으로 넘기기 위해
		pthread_detach(client_thread); // 스레드를 분리하여 리소스를 자동으로 정리할 수 있도록 설정
	}
}

void* receive_packet(void *arg) {
	int client_sock = (int)(intptr_t)arg;
	Packet packet;
	int bytes_received;

	while (1) {
		bytes_received = recv(client_sock, &packet, sizeof(Packet), 0);
		if (bytes_received < 0) {
			// [Server] 수신 실패 시 오류 메시지 출력 후 소켓 닫기
			perror("[Server] Receive failed");
			close(client_sock);
			return NULL;
		} else if (bytes_received == 0) {
			// 수신된 바이트가 없으면 클라이언트가 정상적으로 연결을 종료한 것으로 간주하고 소켓 닫기
			printf("[Server] Client disconnected gracefully");
			close(client_sock);
			return NULL;
		}
		//
		temp_client.sockfd = client_sock; //클라이언트 socketfd main으로 넘기기
		strcncpy(temp_client.username, packet.username, sizeof(packet.username)); // 유저 이름 main으로 넘기기

		// !!! queue 처리 시간대랑 temp_client 설정 시간대가 안 맞으면 로그인 처리가 이상해짐(수정 필요)

		// 수신된 패킷을 작업 큐에 추가
		enqueue(packet);
		printf("[Server] Enqueued packet from client");
	}

	return NULL;
}

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


