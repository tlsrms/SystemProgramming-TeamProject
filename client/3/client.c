#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include "../includes/common.h"

#define PORT 8080
#define BUFFER_SIZE 4096
#define WATCH_DIRECTORY "./watch/"
#define SHARED_FILE "shared_file.txt"

// 패킷 구조체
typedef struct
{
	int flag;					 // 데이터 유형 플래그: 1=채팅, 2=파일 변경
	char username[50];			 // 사용자 이름
	char message[BUFFER_SIZE];	 // 채팅 메시지
	char file_data[BUFFER_SIZE]; // 파일 데이터
} Packet;

// 서버로부터 패킷을 수신하는 스레드 함수
void *receive_server_packet(void *arg);

// 터미널 입력을 받고 이를 서버로 보내는 스레드 함수
void *send_terminal_packet(void *arg);

void command_new();
void command_load(const char *input);
void command_quit();
void send_chat_message(const char *input);

int client_socket; // 클라이언트 소켓, 추후 스레드에서 접근
char username[50]; // 현재 사용자(클라이언트) 이름 정보, 회원가입 및 로그인 성공 시 이 전역변수에 저장

int main()
{
	struct sockaddr_in server_addr;			// 서버 주소 저장
	pthread_t thread_receive_server_packet; // 서버로부터 패킷을 받는 스레드
	pthread_t thread_send_terminal_packet;	// 서버로 터미널 입력 패킷을 보내는 스레드

	// 소켓 생성
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket < 0)
	{
		perror("[Client] Socket creation failed");
		return EXIT_FAILURE;
	}

	// 서버 주소 설정
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr("192.168.88.128");

	// 서버 연결
	if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("[Client] Connection to server failed");
		return EXIT_FAILURE;
	}

	////////////////////////////////////////////////////////// 스레드 /////////////////////////////////////////////////////////////////
	// 서버 패킷 수신 스레드 시작
	pthread_create(&thread_receive_server_packet, NULL, receive_server_packet, NULL);

	// 터미널 입력 및 패킷 발신 스레드 시작
	pthread_create(&thread_send_terminal_packet, NULL, send_terminal_packet, NULL);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

/////////////////////////////////////////////////////////// 스레드 함수 ////////////////////////////////////////////////////////////////
void *receive_server_packet(void *arg)
{
	Packet packet;

	while (1)
	{
		int bytes_received = recv(client_socket, &packet, sizeof(Packet), 0);

		if (bytes_received <= 0)
		{
			printf("[Client] Disconnected from server.\n");
			exit(EXIT_FAILURE);
		}

		// 패킷 정보를 메인스레드의 작업큐로 보내는 코드
		enqueue(packet);
	}

	return NULL;
}

void *send_terminal_packet(void *arg)
{
	while (1)
	{
		char input[BUFFER_SIZE];
		printf("> ");
		fgets(input, sizeof(input), stdin); // 터미널 입력 받기
		input[strcspn(input, "\n")] = '\0'; // 개행 문자 제거

		// 입력 내용에 따라 분기
		if (strcmp(input, "/new") == 0)
		{
			command_new(); //shared_file.txt 초기화
		}
		else if (strstr(input, "/load") != NULL)
		{
			command_load(input); // 파일 내용 복사 및 shared_file.txt에 붙여넣기
		}
		else if (strcmp(input, "/quit") == 0)
		{
			command_quit(); // 서버와의 연결 종료
			break;
		}
		else
		{
			send_chat_message(input); // 채팅 메시지 전송
		}
	}

	return NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////// 터미널 입력에 따른 분기별 함수 /////////////////////////////////////////////////////////////////////
void command_new()
{
	// 파일을 쓰기 모드로 열기
    FILE *file = fopen(SHARED_FILE, "w"); // 쓰기 모드는 자동으로 파일 내용을 삭제
    if (file == NULL) {
        perror("[Error] Failed to open file");
        return;
    }

    fclose(file);
	printf("[Client] File '%s' newly created successfully.\n", SHARED_FILE);
}

void command_load(const char *input)
{
	// 입력에서 파일 경로 추출
    char filepath[BUFFER_SIZE];
    sscanf(input + 6, "%s", filepath); // "/load " 이후 파일 이름 읽기

    printf("[Client] Loading file from path: %s\n", filepath);

    // loaded_file.txt 열기 (읽기 모드)
    FILE *loaded_file = fopen(filepath, "r");
    if (loaded_file == NULL)
    {
        perror("[Client] Failed to open file");
        return;
    }

    // shared_file.txt 열기 (쓰기 모드)
    FILE *shared_file = fopen(SHARED_FILE, "w");
    if (shared_file == NULL)
    {
        perror("[Client] Failed to open shared_file.txt");
        fclose(loaded_file);
        return;
    }

    // 파일 내용 복사
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), loaded_file)) > 0)
    {
        fwrite(buffer, 1, bytes_read, shared_file);
    }

    // 파일 스트림 닫기
    fclose(loaded_file);
    fclose(shared_file);

    printf("[Client] File '%s' loaded to 'shared_file.txt' successfully.\n", filepath);

}

void command_quit()
{
	printf("[Client] Exiting...\n");
	close(client_socket);
}

void send_chat_message(const char *input)
{
	Packet packet;

	memset(&packet, 0, sizeof(Packet)); // 패킷 메모리 초기화
	packet.flag = 1;					// 패킷 플래그 설정 (채팅)
	strncpy(packet.username, username, sizeof(packet.username));
	strncpy(packet.message, input, sizeof(packet.message));

	pthread_mutex_lock(&send_mutex);
	if (send(client_socket, &packet, sizeof(Packet), 0) < 0)
	{ // 전송
		perror("[Client] Failed to send message");
	}
	pthread_mutex_unlock(&send_mutex); // unlock
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
