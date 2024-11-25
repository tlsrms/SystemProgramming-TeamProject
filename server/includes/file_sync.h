#ifndef FILE_SYCN_H
#define FILE_SYCN_H
#include "broadcast.h"


// 패킷이 파일 패킷이라면
// 파일을 shared_file.txt에 반영 -> broadcast

// 파일 패킷 처리 함수
int handle_file_packet(const char* packet_data, const char* shared_file_path);


#endif
