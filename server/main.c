#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include "includes/common.h"
#include "includes/login_control.h"
#include "includes/broadcast.h"
#include "includes/file_sync.h"
#include "includes/version_control.h"

int initialize_server(int port, struct sockaddr_in *server_addr);
void enqueue(Packet packet);
int dequeue(Packet *packet);
void* accept_socket(void *);
void* receive_packet(void *arg);
void send_initial_data(int client_sock);
int find_current_sockfd(Packet *);

int main() {
    signal(SIGINT, handle_sigint); //ctrl + c 입력이 들어오면 keep_running 값이 0으로 바뀌고 while문이 종료됩니다.

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
    while (!keep_running) {
        // 작업 큐에서 패킷을 읽고 채팅인지 파일인지 확인
        if (dequeue(&current_work)) {
            if (current_work.flag == 1) { // 채팅 메시지
                // 
                // 채팅 log 저장 + 채팅 broadcast <- 3번
                handle_chat_message(&current_work, find_current_sockfd(&current_work));
                //
            } else if (current_work.flag == 2) { // 파일 데이터
				//
				// 파일 내역 읽고 shared_file.txt에 반영 + 파일 broadcast <- 2번
				handle_file_packet(&current_work, find_packet_sock(&current_work));
				//
        	}
			else if(current_work.flag == 3){
				//
				// '/commit', '/log', '/rebase' 명령어 처리 <- 4번
				if (strncmp(current_work.message, "/commit", 7) == 0) {
                    commit_version();
                } else if (strncmp(current_work.message, "/log", 4) == 0) {
                    log_versions();
                } else if (strncmp(current_work.message, "/rebase", 7) == 0) {
                    int version_number = atoi(&current_work.message[8]);
                    rebase_version(version_number);
                } else {
                    printf("[Server] Unknown version control command received: %s\n",current_work.message);
                }
			}
            usleep(10000); // 잠시 대기하여 CPU 사용률 감소 (GPT 추천인데 실제로 써봐야 알 거 같음)
    	}
	}

    //while문 종료 후 자원해제 코드
    close(server_fd); //서버 소켓이 닫기면 accept(), recv() 함수에서 -1이 반환됨 -> 각 스레드 종료됨.

    //모든 스레드가 정상 종료 되기를 기다림
    pthread_join(accept_ClientSocket, NULL);
    for (int i = 0; i < client_count; i++) {
        pthread_join(clients[i].thread, NULL);
    }

    // 뮤텍스 자원 해제
    pthread_mutex_destroy(&send_mutex);
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&clients_mutex);
    printf("[Server] Successfully shut down.\n");

    return 0; //서버 프로그램 종료
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

        int *client_sock_ptr = malloc(sizeof(int));
        if (client_sock_ptr == NULL) {
            perror("[Server] malloc failed");
            free(client_sock_ptr);
            close(new_sock);
            continue;
        }
        *client_sock_ptr = new_sock;


		// 클라이언트 소켓을 처리하기 위한 새로운 스레드 생성
		pthread_t client_thread;
		if (pthread_create(&client_thread, NULL, receive_packet, client_sock_ptr) != 0) {
			perror("[Server] Failed to create client thread");
			close(new_sock); 
			continue;
		}
		pthread_detach(client_thread); // 스레드를 분리하여 리소스를 자동으로 정리할 수 있도록 설정
	}

    return NULL;
}

void* receive_packet(void *arg) {
	int client_sock = *(int*)arg;
    free(arg);
	Packet packet;
	int bytes_received;
	Client temp_client; // 로컬 변수로 선언

    temp_client.sockfd = client_sock;
    temp_client.thread = pthread_self();
	int is_logged_in = 0; // 로그인 상태 변수

    while (1) {
        bytes_received = recv(client_sock, &packet, sizeof(Packet), 0);
		if(bytes_received <= 0) {
			if (bytes_received == 0)
				printf("[Server] Client disconnected gracefully"); // 수신된 바이트가 없으면 클라이언트가 정상적으로 연결을 종료한 것으로 간주하고 소켓 닫기
			else if (bytes_received < 0)
				perror("[Server] Receive failed or Server socket is closed"); // [Server] 수신 실패 / server_sock이 close될 시 오류 메시지 출력 후 소켓 닫기 후 break
            
			if(is_logged_in)
				remove_client(client_sock);
			else 
				close(client_sock);
			break;
		}

        if (!is_logged_in) {
			strncpy(temp_client.username, packet.username, sizeof(temp_client.username));
            if (packet.flag == 0) {
                // 로그인 처리
                // 중복 체크
                pthread_mutex_lock(&clients_mutex);
                int duplicate = is_user_registered(temp_client.username);

                if (duplicate) {
                    // 실패 응답 전송
                    pthread_mutex_lock(&send_mutex);
                    send(client_sock, "DUPLICATE", 9, 0);
                    pthread_mutex_unlock(&send_mutex);
                    pthread_mutex_unlock(&clients_mutex);
                    // 로그인 실패 시 다시 로그인 패킷을 받을 수 있도록 계속 루프 진행
                    continue;
                } else {
					// user.txt에 저장
					register_user(temp_client.username);
                    // 클라이언트 배열에 추가
					clients[client_count++] = temp_client;
					is_logged_in = 1; // 로그인 성공
                	pthread_mutex_unlock(&clients_mutex);
					// 성공 응답 전송
                    pthread_mutex_lock(&send_mutex);
					send(client_sock, "REGISTERED", 10, 0); // 등록 성공 응답
					// 초기 데이터 전송 (채팅 로그와 공유 파일)
                    pthread_mutex_unlock(&send_mutex);
                    send_initial_data(client_sock);
                    printf("[Server] New user registered: %s\n", temp_client.username);
                }
            } else {
                // 로그인 없이 접속한 경우 (user.txt에 이름이 이미 있음)
                pthread_mutex_lock(&clients_mutex);
                // 클라이언트 배열에 추가
                clients[client_count++] = temp_client;
                pthread_mutex_unlock(&clients_mutex);
                is_logged_in = 1;

                // 초기 데이터 전송 (채팅 로그와 공유 파일)
                send_initial_data(client_sock);
                printf("[Server] Existing user connected: %s\n", temp_client.username);
            }
		}
		if(is_logged_in) {
			enqueue(packet);
			printf("[Server] Enqueued packet from client");
		}
    }
    return NULL;
}

// 초기 데이터 전송 함수
void send_initial_data(int client_sock) {
    Packet packet;
    // 채팅 로그 전송
    pthread_mutex_lock(&file_mutex);
    FILE *log_file = fopen(CHAT_LOG, "r");
    if (log_file) {
        fgets(packet.message, sizeof(packet.message), log_file);
		packet.flag = 1; // 채팅 메시지 플래그
        pthread_mutex_lock(&send_mutex);
		send(client_sock, &packet, sizeof(Packet), 0);
        pthread_mutex_unlock(&send_mutex);
        fclose(log_file);
    }
    pthread_mutex_unlock(&file_mutex);

    // 공유 파일 전송
    pthread_mutex_lock(&file_mutex);
    FILE *shared_file = fopen(SHARED_FILE, "r");
    if (shared_file) {
        fgets(packet.file_data, sizeof(packet.file_data), shared_file);
		packet.flag = 2; // 파일 데이터 플래그
        pthread_mutex_lock(&send_mutex);
		send(client_sock, &packet, sizeof(Packet), 0);
        pthread_mutex_unlock(&send_mutex);
        fclose(shared_file);
    }
    pthread_mutex_unlock(&file_mutex);
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

int find_current_sockfd(Packet *current_packet){
	int temp_socketfd;
	pthread_mutex_lock(&clients_mutex);
	for(int i=0; i<client_count; i++){
		if(strcmp(clients[i].username, current_packet->username) == 0)
			temp_socketfd = clients[i].sockfd;
			pthread_mutex_unlock(&clients_mutex);
			return temp_socketfd;
	}

	printf("can't find client_sockfd");
    pthread_mutex_unlock(&clients_mutex);
	return -1;
}

void handle_sigint(int sig) {
    keep_running = 0;
}