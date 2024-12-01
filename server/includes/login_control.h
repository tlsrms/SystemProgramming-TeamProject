#ifndef LOGIN_CONTROL.H
#define LOGIN_CONTROL.H

#include "common.h"

int is_user_registered(const char *username);
void register_user(const char *username);
void remove_client(int sockfd);
void *handle_client(void *arg);

#endif