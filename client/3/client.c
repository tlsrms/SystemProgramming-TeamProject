#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define TARGET_DIRECTORY "/home/sunflow-er/sharing/"
#define MONITORED_FILE "client_file.txt"

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

void create_and_send_file();
void load_and_send_file(const char *input);
void disconnect_from_server();
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

		// TODO 패킷 정보를 메인스레드의 작업큐로 보내는 코드
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
			create_and_send_file(); // 새로운 파일 생성 및 전송
		}
		else if (strstr(input, "/load") != NULL)
		{
			load_and_send_file(input); // 파일 로드 및 전송
		}
		else if (strcmp(input, "/quit") == 0)
		{
			disconnect_from_server(); // 서버와의 연결 종료
			break;
		}
		else
		{
			send_chat_message(input); // 채팅 메시지 전송
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////// 터미널 입력에 따른 분기별 함수 /////////////////////////////////////////////////////////////////////
void create_and_send_file()
{
	Packet packet;

	// 새로운 파일 이름 입력
	printf("Enter the file name to create: ");
	char filename[BUFFER_SIZE];
	fgets(filename, sizeof(filename), stdin);
	filename[strcspn(filename, "\n")] = '\0'; // 개행 문자 제거

	// 파일 생성
	FILE *file = fopen(filename, "w");
	if (file == NULL)
	{
		perror("[Client] Failed to create file");
		return;
	}

	// 새로운 파일 내용 초기화
	printf("Enter content for the new file (end with an empty line):\n");
	char line[BUFFER_SIZE];
	while (1)
	{
		fgets(line, sizeof(line), stdin);
		if (strcmp(line, "\n") == 0)
			break; // 빈 줄로 종료
		fputs(line, file);
	}
	fclose(file);

	// 파일 읽기
	file = fopen(filename, "r");
	if (file == NULL)
	{
		perror("[Client] Failed to open file for reading");
		return;
	}

	memset(&packet, 0, sizeof(Packet)); // 패킷 초기화
	packet.flag = 2;					// 플래그 설정 (파일 전송)
	strncpy(packet.username, username, sizeof(packet.username));
	fread(packet.file_data, 1, sizeof(packet.file_data) - 1, file);
	fclose(file);

	// 파일 전송
	if (send(client_socket, &packet, sizeof(Packet), 0) < 0)
	{
		perror("[Client] Failed to send file");
	}
	else
	{
		printf("[Client] File '%s' created and sent to server successfully.\n", filename);
	}
}

void load_and_send_file(const char *input)
{
	Packet packet;

	// 입력에서 파일 이름 추출
	char filename[BUFFER_SIZE];
	sscanf(input + 6, "%s", filename); // "/load " 이후 파일 이름 읽기

	// 디폴트 디렉토리 경로와 파일 이름 결합
	char filepath[BUFFER_SIZE];
	snprintf(filepath, sizeof(filepath), "%s%s", TARGET_DIRECTORY, filename);

	printf("[Client] Loading file from path: %s\n", filepath);

	// 파일 열기
	FILE *file = fopen(filepath, "r");
	if (file == NULL)
	{
		perror("[Client] Failed to open file");
		return;
	}

	// 패킷 초기화
	memset(&packet, 0, sizeof(Packet));
	packet.flag = 3; // 플래그 설정 (기존 파일 업로드)
	strncpy(packet.username, username, sizeof(packet.username));

	// 파일 읽기
	size_t bytes_read = fread(packet.file_data, 1, sizeof(packet.file_data) - 1, file);
	fclose(file);

	if (bytes_read == 0)
	{
		printf("[Client] File '%s' is empty.\n", filename);
	}

	// 서버로 패킷 전송
	if (send(client_socket, &packet, sizeof(Packet), 0) < 0)
	{
		perror("[Client] Failed to send file");
	}
	else
	{
		printf("[Client] File '%s' sent to server successfully.\n", filename);
	}
}

void disconnect_from_server()
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

	if (send(client_socket, &packet, sizeof(Packet), 0) < 0)
	{ // 전송
		perror("[Client] Failed to send message");
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
