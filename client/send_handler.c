#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "includes/receive_handler.h"

/////////////////////////////////////////////////////////// 스레드 함수 ////////////////////////////////////////////////////////////////
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
		}
		else
		{
			send_chat_message(input); // 채팅 메시지 전송
		}

		if (!keep_running) {
			return;
		}
	}

	return NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////// 터미널 입력에 따른 분기별 함수 /////////////////////////////////////////////////////////////////////
// /new
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

// /load
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

// /quit
void command_quit()
{
	keep_running = 0;
	printf("[Client] Exiting...\n");
	close(client_socket);
}

// 채팅 메시지 입력
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
