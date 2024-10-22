#pragma once

typedef void (*file_open_callback_t)(int fd, void* callback_data);
int file_open(const char* filename, int flags, file_open_callback_t callback, void* callback_data);

typedef void (*file_close_callback_t)( int errorcode, void* callback_data);
void file_close(int fd, file_close_callback_t callback, void* callback_data);