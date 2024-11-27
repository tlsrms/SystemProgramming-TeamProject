#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include "common.h"

// 메시지 수신
void *message_listener(void *arg);

// 메시지 발신
void send_chat_message(int server_sock, const char *username, const char *message);

#endif // MESSAGE_HANDLER_H
