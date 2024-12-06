#ifndef FILE_SYCN_H
#define FILE_SYCN_H
#include "includes/broadcast.h"


// 패킷이 파일 패킷이라면 handle_file_packet 함수 호출
// 파일을 shared_file.txt에 반영 -> broadcast

// 파일 패킷 처리 함수
int handle_file_packet(Packet *file_packet, int exclude_sock);


#endif
