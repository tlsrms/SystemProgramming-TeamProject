#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "includes/file_sync.h"
#include "includes/broadcast.h"
#include "includes/common.h"

//shared.txt 열거나 생성 후 packet에 있는 파일 데이터 복사
//받은 packet 그대로 broadcast
//void broadcast_packet(Packet *packet, Client clients[], int client_count, int exclude_sock);


int handle_file_packet(Packet *file_packet, int exclude_sock){
	pthread_mutex_lock(&file_mutex);
	FILE *fp;
	fp = fopen(SHARED_FILE, "w");
	if(fp == NULL){
		perror(SHARED_FILE);
		return -1;
	}

	if(fwrite(file_packet->file_data, 1, BUFFER_SIZE, fp) < 0){
		perror("fwrite");
		fclose(fp);
		pthread_mutex_unlock(&file_mutex);
		return -1;
	}

	fclose(fp);
	pthread_mutex_unlock(&file_mutex);

	broadcast_packet(file_packet, clients, client_count, exclude_sock);
	return 0;
}