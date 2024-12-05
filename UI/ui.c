#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>
#include "includes/common.h"  

#define CHAT_LOG_DISPLAY_HEIGHT 15
#define INPUT_FIELD_HEIGHT 3
#define BUFFER_SIZE 4096

// 채팅 로그 출력
void display_chat_log(const char *chat_log) {
    clear();  // 화면을 지우고 새로 시작
    int i = 0;
    int line = 0;
    
    // 채팅 로그를 한 줄씩 출력
    while (chat_log[i] != '\0' && line < CHAT_LOG_DISPLAY_HEIGHT) {
        mvprintw(line++, 0, "%s", &chat_log[i]);
        while (chat_log[i] != '\n' && chat_log[i] != '\0') {
            i++;
        }
        if (chat_log[i] == '\n') {
            i++;
        }
    }

    // 입력 필드 출력
    mvprintw(LINES - INPUT_FIELD_HEIGHT, 0, "Enter your message: ");
    refresh();
}

// 사용자 입력 받기
void get_user_input(char *input) {
    echo();  // 입력을 화면에 표시
    mvgetstr(LINES - 2, 0, input);
    noecho();  // 입력을 화면에 표시하지 않음
}

// 채팅 로그를 `chatLog_packet`에 출력
void update_chat_log(Packet *chatLog_packet) {
    // chatLog_packet->message에는 기존의 채팅 로그 내용이 있어야 합니다.
    display_chat_log(chatLog_packet->message);
}

int main() {
    // ncurses 초기화
    initscr();
    cbreak();
    noecho();
    curs_set(0);  // 커서 숨기기

    char user_input[BUFFER_SIZE];
    Packet chatLog_packet;
    
    // 예시 채팅 로그 데이터를 초기화
    strncpy(chatLog_packet.message, "Welcome to the chat!\n", sizeof(chatLog_packet.message));
    
    // 채팅 로그 UI 표시
    update_chat_log(&chatLog_packet);

    while (1) {
        // 사용자로부터 입력 받기
        get_user_input(user_input);
        
        // 서버로 메시지 전송 (단순히 콘솔에 출력한다고 가정)
        if (strcmp(user_input, "/exit") == 0) {
            break;  // /exit 입력 시 종료
        }

        // 새로운 메시지를 채팅 로그에 추가
        strcat(chatLog_packet.message, user_input);
        strcat(chatLog_packet.message, "\n");
        
        // 채팅 로그 UI 갱신
        update_chat_log(&chatLog_packet);
        
        // 서버로 메시지 전송하는 로직 추가
        // 예: send(client_socket, &chatLog_packet, sizeof(Packet), 0);
        
        // 잠시 대기
        usleep(50000);  // 0.05초 대기
    }

    endwin();  // ncurses 종료
    return 0;
}
