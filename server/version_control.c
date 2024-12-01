#include "includes/version_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include "includes/common.h"

#define VERSION_DIR "version_logs/"
#define SHARED_FILE "shared_file.txt"
#define MAX_FILENAME_LEN 100
#define BUFFER_SIZE 4096

pthread_mutex_t version_mutex = PTHREAD_MUTEX_INITIALIZER;

// 버전 저장 경로 초기화
void initialize_version_directory() {
    DIR* dir = opendir(VERSION_DIR);
    if (dir) {
        // 디렉터리가 이미 존재함
        closedir(dir);
    } else {
        // 디렉터리가 없으면 생성
        if (mkdir(VERSION_DIR, 0777) == -1) {
            perror("Failed to create version directory");
            exit(EXIT_FAILURE);
        }
    }
}

// 파일을 새로운 버전으로 커밋
void commit_version() {
    pthread_mutex_lock(&version_mutex);

    // 버전 번호 확인 (현재 디렉토리에 저장된 파일 개수를 기반으로 결정)
    int version_number = 1;
    struct dirent *entry;
    DIR *dp = opendir(VERSION_DIR);
    if (dp) {
        while ((entry = readdir(dp))) {
            if (entry->d_name[0] != '.') { // 숨김 파일 제외
                version_number++;
            }
        }
        closedir(dp);
    }

    // 버전 파일 이름 생성
    char versioned_file[MAX_FILENAME_LEN];
    snprintf(versioned_file, sizeof(versioned_file), "%s%d_shared_file.txt", VERSION_DIR, version_number);

    // 원본 파일 복사
    FILE *src_file = fopen(SHARED_FILE, "r");
    FILE *dest_file = fopen(versioned_file, "w");

    if (!src_file || !dest_file) {
        perror("Failed to open file for commit");
        pthread_mutex_unlock(&version_mutex);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        fwrite(buffer, 1, bytes_read, dest_file);
    }

    fclose(src_file);
    fclose(dest_file);

    printf("[Server] Version %d committed successfully.\n", version_number);
    pthread_mutex_unlock(&version_mutex);
}

// 저장된 모든 버전을 표시 (/log 명령어)
void log_versions() {
    pthread_mutex_lock(&version_mutex);

    struct dirent *entry;
    DIR *dp = opendir(VERSION_DIR);
    if (!dp) {
        perror("Failed to open version directory");
        pthread_mutex_unlock(&version_mutex);
        return;
    }

    printf("\n[Version Log]\n");
    char filename[MAX_FILENAME_LEN];
    time_t t;
    struct tm *tm_info;
    FILE *file;

    // 최신 버전이 먼저 출력되도록 파일 목록을 역순으로 정렬하여 저장
    struct dirent *entries[100];
    int entry_count = 0;

    while ((entry = readdir(dp))) {
        if (entry->d_name[0] != '.') {
            entries[entry_count++] = entry;
        }
    }
    closedir(dp);

    // 파일 목록 역순 출력
    for (int i = entry_count - 1; i >= 0; i--) {
        snprintf(filename, sizeof(filename), "%s%s", VERSION_DIR, entries[i]->d_name);
        file = fopen(filename, "r");
        if (file) {
            fseek(file, 0, SEEK_SET);
            fread(&t, sizeof(time_t), 1, file);
            tm_info = localtime(&t);
            char time_buffer[30];
            strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
            printf("Version: %s\tDate: %s\n", entries[i]->d_name, time_buffer);
            fclose(file);
        }
    }

    pthread_mutex_unlock(&version_mutex);
}

// 특정 버전으로 복원 (/rebase 명령어)
void rebase_version(int version_number) {
    pthread_mutex_lock(&version_mutex);

    char versioned_file[MAX_FILENAME_LEN];
    snprintf(versioned_file, sizeof(versioned_file), "%s%d_shared_file.txt", VERSION_DIR, version_number);

    // 버전 파일이 있는지 확인
    FILE *src_file = fopen(versioned_file, "r");
    if (!src_file) {
        printf("[Server] Version %d not found.\n", version_number);
        pthread_mutex_unlock(&version_mutex);
        return;
    }

    // 원본 파일 덮어쓰기
    FILE *dest_file = fopen(SHARED_FILE, "w");
    if (!dest_file) {
        perror("Failed to open shared file for rebase");
        fclose(src_file);
        pthread_mutex_unlock(&version_mutex);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        fwrite(buffer, 1, bytes_read, dest_file);
    }

    fclose(src_file);
    fclose(dest_file);

    printf("[Server] File rebased to version %d successfully.\n", version_number);
    pthread_mutex_unlock(&version_mutex);
}
