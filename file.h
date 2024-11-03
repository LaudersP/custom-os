#include "fs.h"
#include "kstrcpy.h"
#include "kstrlen.h"
#include "kstrequal.h"
#include "errno.h"

typedef void (*file_read_callback_t)(
        int ecode,          //error code
        void* buf,          //buffer with data
        unsigned numread,   //num read
        void* callback_data         //user data
);

void file_read( int fd,         //file to read from
                void* buf,      //buffer for data
                unsigned count, //capacity of buf
                file_read_callback_t callback,
                void* callback_data //passed to callback
);

typedef void (*file_write_callback_t)(
        int error_code,
        unsigned numwritten,    //num written
        void* callback_data    //user data
);

void file_write( int fd,         //file to read from
                void* buf,      //buffer for data
                unsigned count, //capacity of buf
                file_write_callback_t callback,
                void* callback_data //passed to callback
);

int file_seek(int fd, int delta, int whence);
int file_tell(int fd, unsigned* offset);