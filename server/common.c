#include "common.h"
#include <pthread.h>
#include <netinet/in.h> // sockaddr_in 사용을 위해 필요함

// 작업 큐 정의
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
Packet packet_queue[QUEUE_SIZE];
int front = 0;
int rear = 0;

// 전역 변수 정의
int keep_running = 1;
Client clients[MAX_CLIENTS];         // 클라이언트 정보 배열
int client_count = 0;                // 연결된 클라이언트 수
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // 클라이언트 배열 보호 뮤텍스
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;    // 파일 작업 보호 뮤텍스
pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;    // send()시 소켓 보호 뮤텍스
int server_fd;                       // 서버 소켓 파일 디스크립터
struct sockaddr_in server_addr;      // 서버 주소 구조체
