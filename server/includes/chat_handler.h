#ifndef CHAT_HANDLER_H
#define CHAT_HANDLER_H

#include "common.h"

void handle_chat_message(Packet *packet, int sender_sock);
void save_chat_log(const char *username, const char *message);

#endif
