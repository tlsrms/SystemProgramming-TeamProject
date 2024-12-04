#include "includes/file_monitor.h"
#include "includes/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))
#define WATCH_DIRECTORY "./watch/"
#define FILE_NAME_SIZE 64

void *watch_file(void *arg){
    pthread_mutex_lock(&global_mutex);
    int sockfd = client_socket;
    char name[50];
    snprintf(name, sizeof(name), "%s", username);
    pthread_mutex_unlock(&global_mutex);

    int inotify_fd = init_inotify();
    char buffer[EVENT_BUF_LEN];
    int length;

    while(1){
        if(!keep_running){
            fclose(inotify_fd); //스레드 종료 시 inotify도 자원해제
            return NULL;
        }
        length = read(inotify_fd, buffer, EVENT_BUF_LEN);

        if(length == 0){
            continue;
        }
        else if(length < 0){
            perror("fail to read event");
            exit(EXIT_FAILURE);
        }

        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (event->len) {
                char file_path[512];
                snprintf(file_path, sizeof(file_path), "%s%s", WATCH_DIRECTORY, event->name);

                if (event->mask & IN_CREATE) {
                    //printf("New file %s created.\n", event->name);
                    send_file_to_server(file_path, sockfd, name);
                } else if (event->mask & IN_MODIFY) {
                    //printf("File %s modified.\n", event->name);
                    sleep(1); // 잠시 기다리기
                    send_file_to_server(file_path, sockfd ,name);
                }
            }
            i += EVENT_SIZE + event->len;
        }
        usleep(10000);
    }
    
}

int init_inotify() {
    // watch directory가 없으면 생성
    struct stat st = {0};
    if (stat(WATCH_DIRECTORY, &st) == -1) {
        if (mkdir(WATCH_DIRECTORY, 0700) < 0) {
            perror("mkdir");
            return -1;
        }
    }

    int inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        perror("inotify_init");
        return -1;
    }
    
    int watch_descriptor = inotify_add_watch(inotify_fd, WATCH_DIRECTORY, IN_MODIFY | IN_CREATE | IN_DELETE);
    if (watch_descriptor < 0) {
        perror("inotify_add_watch");
        return -1;
    }

    return inotify_fd;
}

int send_file_to_server(const char* file_path, int socket_fd, char *uname) {
    size_t bytes_read;
	Packet new_packet = {0};
	new_packet.flag = 2;
	snprintf(new_packet.username, sizeof(new_packet.username), "%s", uname);

    pthread_mutex_lock(&file_mutex);
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("fopen");
        pthread_mutex_unlock(&file_mutex);
        return -1;
    }
    
    if((bytes_read = fread(new_packet.file_data, 1, sizeof(new_packet.file_data), file)) == 0){
		perror("fread");
		fclose(file);
        pthread_mutex_unlock(&file_mutex);
		return -1;
	}
    fclose(file);
    pthread_mutex_unlock(&file_mutex);

 
    pthread_mutex_lock(&send_mutex);
    if (send(socket_fd, &new_packet, sizeof(Packet), 0) < 0) {
            perror("send");
            pthread_mutex_unlock(&send_mutex);
            return -1;
    }
    pthread_mutex_unlock(&send_mutex);

    return 0;
}

int apply_to_file(char* save_path, Packet *recieved_packet) {
    pthread_mutex_lock(&file_mutex);
    FILE *file = fopen(save_path, "w");
    if (!file) {
        perror("fopen");
        return -1;
    }

	if (fwrite(&recieved_packet->file_data, 1,sizeof(recieved_packet->file_data) , file) == 0) {
		perror("fwrite");
		fclose(file);
		return -1;
	}

    fclose(file);
    pthread_mutex_unlock(&file_mutex);
    return 0;
}
