#include "file.h"
#include "utils.h"
#include "memory.h"
#include "disk.h"

extern struct File file_table[MAX_FILES];
extern struct VBR vbr;


struct ReadInfo{
    int fd;
    void* buffer;
    unsigned num_requested;     //amount of data requested
    file_read_callback_t callback;
    void* callback_data;
};

void file_read_part_2(int errorcode, void* sector_data, void* callback_data) {
    struct ReadInfo* ri = (struct ReadInfo*)callback_data;
    
    if(errorcode) {
        ri->callback(errorcode, ri->buffer, 0, ri->callback_data);
        return;
    } else {
        unsigned bytesPerCluster = vbr.bytes_per_sector * vbr.sectors_per_cluster;
        unsigned offsetInBuffer = file_table[ri->fd].offset % bytesPerCluster;
        unsigned bytes_available = bytesPerCluster - offsetInBuffer;
        unsigned numToCopyPartA = (ri->num_requested < bytes_available) ? ri->num_requested : bytes_available;
        unsigned fileLeft = file_table[ri->fd].size - file_table[ri->fd].offset;
        unsigned numToCopy = (numToCopyPartA < fileLeft) ? numToCopyPartA : fileLeft;

        kmemcpy(ri->buffer, sector_data + offsetInBuffer, numToCopy);

        file_table[ri->fd].offset += numToCopy;

        ri->callback (
            SUCCESS,
            ri->buffer,
            numToCopy,
            ri->callback_data
        );
    }
    
    kfree(ri);

    return;
}

void file_read( int fd, void* buf, unsigned count, file_read_callback_t callback, void* callback_data) {
    // Verify the fd is valid
    // 1. Valid index
    // 2. File table entry is true
    if(!(fd >= 0 && fd < MAX_FILES) || !(file_table[fd].in_use)) {
        callback(EINVAL, buf, count, callback_data);
        return;
    }

    // Check the count variable
    if(count == 0) {
        callback(EINVAL, buf, 0, callback_data);
        return;
    }

    // Ensure that we are not at EOF
    if(file_table[fd].offset >= file_table[fd].size) {
        callback(SUCCESS, buf, 0, callback_data);
        return;
    }

    struct ReadInfo* ri = kmalloc(sizeof(struct ReadInfo));
    if(!ri) {
        callback(ENOMEM, buf, 0 , callback_data);
        return;
    }

    ri->fd = fd;
    ri->buffer = buf;
    ri->num_requested = count;
    ri->callback = callback;
    ri->callback_data = callback_data;

    // Calculate the number of clusters to skip
    unsigned bytesPerCluster = vbr.bytes_per_sector * vbr.sectors_per_cluster;
    unsigned clustersToSkip = file_table[fd].offset / bytesPerCluster;

    // Get the FAT
    u32* fat = getFAT();

    // Iterate through the FAT to the desired cluster
    unsigned c = file_table[fd].firstCluster;
    for(unsigned i = 0; i < clustersToSkip; i++)
        c = fat[c];
        
    disk_read_sectors(clusterNumberToSectorNumber(c), vbr.sectors_per_cluster, file_read_part_2, ri);
}

void file_write( int fd, void* buf, unsigned count, file_write_callback_t callback, void* callback_data ) {
    //no such system call
    callback( ENOSYS, 0, callback_data );
}

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int file_seek(int fd, int delta, int whence) {
    // Verify the fd is valid
    // 1. Valid index
    // 2. File table entry is true
    if(!(fd >= 0 && fd < MAX_FILES) || !(file_table[fd].in_use))
        return EINVAL;

    // Act on the value of whence
    switch(whence) {
        case SEEK_SET:  // Delta is interpreted as relative to start of file
            // Check if delta is negative
            if(delta < 0)
                return EINVAL;
            
            // Set file table offset to delta
            file_table[fd].offset = delta;

            break;
        case SEEK_CUR:  // Delta is interpreted as relative to current file position
            // Check if delta is negative
            if(delta < 0) {
                // Check if the offset will underflow
                unsigned newOffset = file_table[fd].offset + delta;
                if(newOffset > file_table[fd].offset)
                    return EINVAL;

                // Perform the offset
                file_table[fd].offset += delta;
            }
            // Check if delta is positive
            else {
                // Check that offset is not less than the old value
                unsigned newOffset = file_table[fd].offset + delta;
                if(newOffset < file_table[fd].offset) {
                    return EINVAL;
                }

                file_table[fd].offset = newOffset;
            }
            
            break;
        case SEEK_END:  // Delta is interpreted as relatice to end of file
            // Check if delta is negative
            if(delta < 0){
                // Check if the offset will underflow
                unsigned newOffset = file_table[fd].size + delta;
                if(newOffset > file_table[fd].size)
                    return EINVAL;

                // Set file_table offset
                file_table[fd].offset = newOffset;
            } else {
                // Check that offset is not less than the old value
                unsigned newOffset = file_table[fd].size + delta;
                if(newOffset < file_table[fd].size)
                    return EINVAL;

                file_table[fd].offset = newOffset;
            }
            
            break;
        default:
            return EINVAL;
    }

    // Return that the seek was successful
    return SUCCESS;
}

int file_tell(int fd, unsigned* offset) {
    // Check if fd is valid
    if (!(fd >= 0 && fd < MAX_FILES) || !(file_table[fd].in_use)) {
        return EINVAL;
    }

    // Check if offset is invalid
    if (offset == NULL) {
        return EINVAL;
    }

    // Set the offset if the pointer is valid
    *offset = file_table[fd].offset;

    return SUCCESS;
}
