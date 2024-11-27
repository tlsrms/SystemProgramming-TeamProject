#ifndef VERSION_CONTROL_H
#define VERSION_CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define VERSION_DIR "version_logs/"  // Directory to store versioned files
#define SHARED_FILE "shared_file.txt" // Current shared file
#define MAX_VERSION_LENGTH 10

// Packet structure for version control commands
typedef struct {
    int flag;                   // Data type flag (for version control)
    char username[50];          // Username of the client
    char message[256];          // Version control message (e.g., commit, rebase)
    char file_data[1024];       // File data (for commit or rebase operations)
    int version;                // Version number (for rebase or commit)
} Packet;

// Function prototypes for version control
void commit_version(int version);
void rebase_version(int version);
void list_versions(void);
void save_version(const char *filename, const char *data, int version);
void restore_version(int version);

// Utility function to generate a version file name based on the version number
char *generate_version_filename(int version);

#endif // VERSION_CONTROL_H
