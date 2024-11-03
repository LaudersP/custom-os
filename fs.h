#pragma once

#define MAX_FILES 16        //real OS's use something like 1000 or so...
#define MAX_PATH 64

struct File{
    int in_use;
    int flags;
    char filename[MAX_PATH+1];
    unsigned offset;
    unsigned size;
    unsigned firstCluster;
};

typedef void (*file_open_callback_t)(int fd, void* callback_data);
int file_open(const char* filename, int flags, file_open_callback_t callback, void* callback_data);

typedef void (*file_close_callback_t)( int errorcode, void* callback_data);
void file_close(int fd, file_close_callback_t callback, void* callback_data);