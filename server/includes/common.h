#include <pthread.h>
#ifndef COMMON_H
#define COMMON_H

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define PORT 8080
#define CHAT_LOG "chat_log.txt"
#define SHARED_FILE "./watch/shared_file.txt"
#define VERSION_DIR "version_logs/"

// 데이터 패킷 구조체
typedef struct {
    int flag;                  // 데이터 유형 플래그
    char username[50];         // 사용자 이름
    char message[BUFFER_SIZE]; // 메시지 또는 파일 이름
    char file_data[BUFFER_SIZE]; // 파일 데이터
} Packet;

// 클라이언트 구조체
typedef struct {
    int sockfd;
    char username[50];
    pthread_t thread;
} Client;

// 작업 큐 정의
#define QUEUE_SIZE 100
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
Packet packet_queue[QUEUE_SIZE];
int front = 0;
int rear = 0;

// 전역 변수 선언
extern Client clients[MAX_CLIENTS];       // 클라이언트 정보 배열
extern int client_count;                  // 연결된 클라이언트 수
extern pthread_mutex_t clients_mutex;     // 클라이언트 배열 보호 뮤텍스
extern pthread_mutex_t file_mutex;        // 파일 작업 보호 뮤텍스
extern int server_fd;
extern struct sockaddr_in server_addr;


#endif
