#include "common.h"
#include <string.h>
#include <stdio.h>

// 전역 변수 초기화
pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

int keep_running = 1;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
Packet packet_queue[QUEUE_SIZE];
int front = 0;
int rear = 0;

// 전역 변수 정의
int client_socket;
struct sockaddr_in server_addr;
char username[50];

