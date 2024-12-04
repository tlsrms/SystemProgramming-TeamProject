
#ifndef FILE_MONITOR_H
#define FILE_MONITOR_H

// inotify로 파일 변경 감지 -> 서버로 파일 전체 전송하기
// 서버에서 파일 받으면 기존 파일에 다시 쓰기 (메세지 받은 경우는 chat_handler에서 처리)
// '/new' , '/load' 명령어 입력 처리 (inotify watch 폴더에 파일 생성 또는 파일 복사)
//	 -> 공유 파일 서버로 전송

#include <stdio.h>          // FILE 타입을 사용하기 위해 필요
#include <sys/inotify.h>    // struct inotify_event를 사용하기 위해 필요
#include <sys/socket.h>     // 소켓 관련 함수 사용
#include <string.h>   

void *watch_file(void *arg); //(스레드 핸들러)

// 파일 적용
int apply_to_file(char* save_path, Packet* recieved_packet);


#endif
