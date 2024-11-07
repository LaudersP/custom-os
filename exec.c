#include "exec.h"
#include "file.h"
#include "errno.h"
#include "memory.h"
#include "kprintf.h"

#define SECTION_CODE (1<<5)
#define SECTION_DATA (1<<6)
#define SECTION_UNINITIALIZED_DATA (1<<7)
#define SECTION_READABLE (1<<30)
#define SECTION_WRITABLE (1<<31)
#define EXE_BASE 0x400000
#define MAX_SECTIONS 32

struct ReadFullyInfo {
    int fd;             //file descriptor
    char* buf;          //destination buffer
    unsigned readSoFar; //how many bytes we've read so far
    unsigned capacity;  //buffer capacity: number to read
    file_read_callback_t callback;  //callback when done reading
    void* callback_data;        //data for callback
};

void file_read_fully2(int errorcode, void* buf, unsigned numread, void* callback_data) {
    struct ReadFullyInfo* rfi = (struct ReadFullyInfo*)callback_data;

    // Check for an error code
    if( errorcode ){
        rfi->callback(errorcode, rfi->buf, rfi->readSoFar, callback_data);
        kfree(rfi);
        return;
    }

    // Check if we reached EOF and have less data then requested
    rfi->readSoFar += numread;
    if( numread == 0 && rfi->readSoFar < rfi->capacity ){
        rfi->callback(ENODATA, rfi->buf, rfi->readSoFar, rfi->callback_data);
        kfree(rfi);
        return;
    }

    // Check if we got as much data as needed
    if( rfi->readSoFar == rfi->capacity ){
        rfi->callback(SUCCESS, rfi->buf, rfi->readSoFar, rfi->callback_data );
        kfree(rfi);
        return;
    }

    // Read more data to the buffer
    file_read(
        rfi->fd,
        rfi->buf+rfi->readSoFar,
        rfi->capacity - rfi->readSoFar,
        file_read_fully2,
        rfi
    );
}

void file_read_fully(int fd, void* buf, unsigned count, file_read_callback_t callback, void* callback_data) {
    struct ReadFullyInfo* rfi = kmalloc(sizeof(struct ReadFullyInfo));
    if(!rfi){
        callback(ENOMEM, buf, 0, callback_data );
        return;
    }

    rfi->fd = fd;
    rfi->buf = (char*) buf;
    rfi->readSoFar = 0;
    rfi->capacity=count;
    rfi->callback=callback;
    rfi->callback_data=callback_data;

    file_read( fd, buf, count, file_read_fully2, rfi );
}

struct ExecInfo {
    int fd;
    unsigned loadAddress;
    exec_callback_t callback;
    void* callback_data;
    struct DOSHeader dosHeader;
    struct PEHeader peHeader;
    struct SectionHeader* sectionHeaders;
    unsigned numSectionsLoaded;
};

void doneLoading(struct ExecInfo* ei) {
    u32 entryPoint = ei->peHeader.optionalHeader.imageBase + ei->peHeader.optionalHeader.entryPoint;
    exec_callback_t callback = ei->callback;

    void* callback_data = ei->callback_data;
    
    file_close(ei->fd, NULL, NULL);
    kfree(ei);
    
    callback(SUCCESS, entryPoint, callback_data );
    
    return;
}

void exec6( int errorcode, void* buf, unsigned nr, void* callback_data);

// Reads the data in each section 
void loadNextSection(struct ExecInfo* ei) {
    //shorten things up
    int i = ei->numSectionsLoaded;

    if( i == ei->peHeader.coffHeader.numSections ){
        doneLoading(ei);
        return;
    }
    
    struct SectionHeader* s = &(ei->sectionHeaders[i]);
    unsigned flags = s->characteristics;
    if( flags & (SECTION_CODE | SECTION_DATA | SECTION_UNINITIALIZED_DATA) ){
        char* p = (char*) (ei->loadAddress);
        p += s->address;

        // Smaller of s->sizeInRAM and s->sizeOnDisk
        int numToLoad = (s->sizeInRAM < s->sizeOnDisk) ? s->sizeInRAM : s->sizeOnDisk;

        //...zero out bytes from p+numToLoad to p+sizeInRAM-1 inclusive...
        if(s->sizeInRAM >= s->sizeOnDisk) {
            kmemset(p + numToLoad, 0, s->sizeInRAM - numToLoad - 1);
        }

        //...file_seek to s->dataPointer...
        unsigned returnedCode = file_seek(ei->fd, s->dataPointer, SEEK_SET);

        //...check for seek error...
        if(returnedCode != 0) {
            ei->callback(returnedCode, 0, ei->callback_data);
            file_close(ei->fd, NULL, NULL);
            kfree(ei);
            return;
        }
        
        file_read_fully( ei->fd, p, numToLoad, exec6, ei );
    } else {
        ei->numSectionsLoaded += 1;
        loadNextSection(ei);
    }
}

//
void exec6( int errorcode, void* buf, unsigned nr, void* callback_data) {
    struct ExecInfo* ei = (struct ExecInfo*) callback_data;

    if( errorcode ){
        ei->callback(errorcode, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL);
        kfree(ei);
        return;
    }

    ei->numSectionsLoaded++;
    loadNextSection(ei);
    return;
}

// Section Headers, setups the loop to read each section 
void exec5( int errorcode, void* buf, unsigned numread, void* callback_data) {
    // Check for errors
     struct ExecInfo* ei = (struct ExecInfo*)callback_data;
     if(errorcode){
         ei->callback(errorcode, 0, ei->callback_data);
         file_close(ei->fd, NULL, NULL);
         kfree(ei);
         return;
     }
     
    ei->numSectionsLoaded = 0;
    loadNextSection(ei);
    //done
}

// PE Header, reads section headers
void exec4( int errorcode, void* buf, unsigned numread, void* callback_data) {
    // Check for error
    struct ExecInfo* ei = (struct ExecInfo*)callback_data;
    if(errorcode){
        ei->callback(errorcode, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL );
        kfree(ei);
        return;
    }
    
    // Make sure its a x86 exe
    if( ei->peHeader.coffHeader.machineType != 0x14c ||
        ei->peHeader.optionalHeader.magic != 0x010b ){
        ei->callback(ENOEXEC, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL);
        kfree(ei);
        return;
    }

    // Check if exe wants loads at 4MB
    if( ei->peHeader.optionalHeader.imageBase != EXE_BASE ){
        ei->callback(EFAULT, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL);
        kfree(ei);
        return;
    }

    // Check there isnt more than 32 sections
    if( ei->peHeader.coffHeader.numSections > MAX_SECTIONS ){
        ei->callback(ENOEXEC, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL);
        kfree(ei);
        return;
    }

    file_read_fully(ei->fd, ei->sectionHeaders, ei->peHeader.coffHeader.numSections * sizeof(struct SectionHeader), exec5, ei );
}

// DOS Header, reads PE Header
void exec3( int errorcode, void* buf, unsigned numread, void* callback_data) {
    // Check for error
    struct ExecInfo* ei = (struct ExecInfo*)callback_data;
    if(errorcode){
        ei->callback(errorcode, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL);
        kfree(ei);
        return;
    }

    // Ensure that the file is actually an exacutable file
    if( 0 != kmemcmp(ei->dosHeader.magic,"MZ",2) ){
        ei->callback(ENOEXEC, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL );
        kfree(ei);
        return;
    }
    
    // Ensure we are in the correct spot
    int rv = file_seek(ei->fd, ei->dosHeader.peOffset, SEEK_SET);
    if(rv<0){
        ei->callback(rv, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL );
        kfree(ei);
        return;
    }

    file_read_fully(ei->fd, &(ei->peHeader), sizeof(ei->peHeader), exec4, ei );
}

// Gets the file, read out the DOS header
void exec2( int fd, void* callback_data) {
    // Check for error
    struct ExecInfo* ei = (struct ExecInfo*)callback_data;
    if(fd < 0){
        ei->callback(fd, 0, callback_data );
        kfree(ei);
        return;
    }
    
    ei->fd=fd;

    file_read_fully(fd, &(ei->dosHeader), sizeof(ei->dosHeader), exec3, ei);
}

// Open the files
void exec(const char* fname, unsigned loadAddress, exec_callback_t callback, void* callback_data) {
    // Check for error
    struct ExecInfo* ei = kmalloc(sizeof(struct ExecInfo));
    if(!ei){
        callback(ENOMEM, (unsigned)NULL, callback_data);
        return;
    }

    ei->callback=callback;
    ei->loadAddress=loadAddress;
    ei->callback_data=callback_data;
    
    file_open(fname, 0, exec2, ei );    //0=flags
}

#define EXE_STACK 0x7FFFFC

void exec_transfer_control(int errorcode, unsigned entryPoint, void* callback_data) {
    // Check for errors
    if( errorcode ){
        kprintf("exec failed: %d\n",errorcode);
        return;
    }

    u32 c = EXE_STACK;
    u32 b = entryPoint;

    // Assembly transfer
    asm volatile(
        "mov $(32|3),%%eax\n"   //ring 3 GDT data segment
        "mov %%ax,%%ds\n"       //ring 3 GDT data segment
        "mov %%ax,%%es\n"       //ring 3 GDT data segment
        "mov %%ax,%%fs\n"       //ring 3 GDT data segment
        "mov %%ax,%%gs\n"       //ring 3 GDT data segment
        "pushl $(32|3)\n"       //ss, ring 3 GDT data segment
        "push %%ecx\n"          //esp
        "push $0x202\n"         //eflags
        "pushl $(24|3)\n"       //cs, ring 3 GDT code segment
        "pushl %%ebx\n"         //eip
        "xor %%eax,%%eax\n"     //clear register
        "xor %%ebx,%%ebx\n"     //clear register
        "xor %%ecx,%%ecx\n"     //clear register
        "xor %%edx,%%edx\n"     //clear register
        "xor %%esi,%%esi\n"     //clear register
        "xor %%edi,%%edi\n"     //clear register
        "xor %%ebp,%%ebp\n"     //clear register
        "iret\n"
        : "+c"(c), "+b"(b)
    );
}