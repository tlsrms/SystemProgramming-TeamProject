// common.h
#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define USER_FILE "user.txt"
#define SHARED_FILE "./watch/shared_file.txt"
#define FILE_NAME_SIZE 64
#define FILE_PATH_SIZE 128

// 패킷 구조체 정의
typedef struct
{
    int flag;                     // 데이터 유형 플래그: 0 = 로그인, 1 = 채팅, 2 = 파일, 3 = 명령어
    char username[50];            // 사용자 이름
    char message[BUFFER_SIZE];    // 채팅 메시지
    char file_data[BUFFER_SIZE];  // 파일 데이터
} Packet;

// 작업 큐 정의
#define QUEUE_SIZE 100
extern pthread_mutex_t queue_mutex;
extern Packet packet_queue[QUEUE_SIZE];
extern int front;
extern int rear;
void enqueue(Packet packet);
int dequeue(Packet *packet);

// 전역 변수 선언
extern int keep_running;
extern pthread_mutex_t global_mutex;
extern pthread_mutex_t send_mutex;
extern pthread_mutex_t file_mutex;

extern int client_socket;
extern struct sockaddr_in server_addr;
extern char username[50];

#endif