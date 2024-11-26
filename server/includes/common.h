#include <pthread.h>
#ifndef COMMON_H
#define COMMON_H

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define PORT 8080
#define USER_FILE "user.txt"

typedef struct {
    int sockfd;
    char username[50];
    pthread_t thread;
} Client;

Client clients[MAX_CLIENTS];      
int client_count;                 
pthread_mutex_t clients_mutex;    

#endif
