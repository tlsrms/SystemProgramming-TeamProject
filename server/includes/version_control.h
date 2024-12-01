#ifndef VERSION_CONTROL_H
#define VERSION_CONTROL_H

#include <pthread.h>

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

void initialize_version_directory();
void commit_version();
void log_versions();
void rebase_version(int version_number);

#endif // VERSION_CONTROL_H
