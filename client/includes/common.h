#ifndef COMMON_H
#define COMMON_H
#define BUFFER_SIZE 4096

// 데이터 패킷 구조체
typedef struct {
    int flag;                  // 데이터 유형 플래그
    char username[50];         // 사용자 이름
    char message[BUFFER_SIZE]; // 메시지 또는 파일 이름
    char file_data[BUFFER_SIZE]; // 파일 데이터
} Packet;

#endif