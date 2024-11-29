#ifndef COMMON_H
#define COMMON_H

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


#endif