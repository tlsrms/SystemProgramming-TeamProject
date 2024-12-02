#ifndef COMMON_H
#define COMMON_H
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define USER_FILE "user.txt"
#define SHARED_FILE "shared_file.txt"

typedef struct
{
	int flag;					 // 데이터 유형 플래그: 0 = 로그인, 1 = 채팅, 2 = 파일 , 3 = 명령어
	char username[50];			 // 사용자 이름
	char message[BUFFER_SIZE];	 // 채팅 메시지
	char file_data[BUFFER_SIZE]; // 파일 데이터
} Packet;

pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER; //공유 변수 접근시 사용
pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER; //send 함수 사용시
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER; // shared_file.txt 사용시

int keep_running = 1; //나중에 모든 무한 반복문 조건을 이 변수로 교체(/quit 호출시 0으로 변경하여 스레드 자원 해제)

// 작업 큐 정의
#define QUEUE_SIZE 100
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
Packet packet_queue[QUEUE_SIZE];
int front = 0;
int rear = 0;

// 전역 변수 선언
extern int client_socket;
extern struct sockaddr_in server_addr;	
extern char username[50];

#endif