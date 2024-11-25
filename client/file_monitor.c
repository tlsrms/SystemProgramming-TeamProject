#include "file_monitor.h"
#include "common.h"
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

void handle_inotify_events(int inotify_fd, int socket_fd, char *username) {
    char buffer[EVENT_BUF_LEN];
    int length = read(inotify_fd, buffer, EVENT_BUF_LEN);

    if (length < 0) {
        perror("read");
    }

    int i = 0;
    while (i < length) {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];
        if (event->len) {
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s%s", WATCH_DIRECTORY, event->name);

            if (event->mask & IN_CREATE) {
                printf("New file %s created.\n", event->name);
                send_file_to_server(file_path, socket_fd, username);
            } else if (event->mask & IN_DELETE) {
                printf("File %s deleted.\n", event->name);
            } else if (event->mask & IN_MODIFY) {
                printf("File %s modified.\n", event->name);
				sleep(1); // 잠시 기다리기
                send_file_to_server(file_path, socket_fd,username);
            }
        }
        i += EVENT_SIZE + event->len;
    }
}

int send_file_to_server(const char* file_path, int socket_fd, char *username) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("fopen");
        return -1;
    }

    size_t bytes_read;
	Packet new_packet = {0};
	new_packet.flag = 1;
	strcpy(new_packet.username, username);


    if((bytes_read = fread(new_packet.file_data, 1, sizeof(new_packet.file_data), file)) == 0){
		perror("fread");
		fclose(file);
		return -1;
	}
 
    if (send(socket_fd, &new_packet, sizeof(Packet), 0) < 0) {
            perror("send");
            fclose(file);
            return -1;
    }

    fclose(file);
    return 0;
}

int apply_to_file(const char* save_path, Packet recieved_packet) {
    FILE *file = fopen(save_path, "wb");
    if (!file) {
        perror("fopen");
        return -1;
    }

	if (fwrite(&recieved_packet.file_data, 1,sizeof(recieved_packet.file_data) , file) == 0) {
		perror("fwrite");
		fclose(file);
		return -1;
	}

    fclose(file);
    return 0;
}

int handle_command(const char* command, const char* parameter, char* file_path_buffer){
	char file_name[FILE_NAME_SIZE];

    if (strcmp(command, "/new") == 0) {

        printf("Creating new file: %s\n", parameter);
		snprintf(file_path_buffer, sizeof(file_path_buffer), "%s%s", WATCH_DIRECTORY, parameter);

        FILE *file = fopen(file_path_buffer, "w");
        if (!file) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        fclose(file);

        return 0;

    } else if (strcmp(command, "/load") == 0) {
        printf("Loading file: %s\n", parameter);

		get_filename(parameter, file_name);
		snprintf(file_path_buffer, sizeof(file_path_buffer), "%s%s", WATCH_DIRECTORY, file_name);

        copy_file(parameter, file_path_buffer);
        return 0;

    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        exit(EXIT_FAILURE);
    }
} 

int copy_file(const char* source, const char* destination) {
    FILE *src = fopen(source, "rb");
    if (!src) {
        perror("fopen source");
        return -1;
    }
    
    FILE *dest = fopen(destination, "wb");
    if (!dest) {
        perror("fopen destination");
        fclose(src);
        return -1;
    }

    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dest) != bytes) {
            perror("fwrite");
            fclose(src);
            fclose(dest);
            return -1;
        }
    }

    fclose(src);
    fclose(dest);
    return 0;
}

void get_filename(const char *path, char* file_name_buffer) {
    char *filename = strrchr(path, '/');
    filename ? filename + 1 : (char *)path;
	strcpy(file_name_buffer, filename);
}