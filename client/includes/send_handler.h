#ifndef SEND_HANDLER_H
#define SEND_HANDLER_H

#include "common.h"

void *send_terminal_packet(void *arg);

void command_new();
void command_load(const char *input);
void command_quit();
void send_chat_message(const char *input);

#endif // SEND_HANDLER_H