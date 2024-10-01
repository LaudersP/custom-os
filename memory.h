#if __STDC_HOSTED__
    #include <stdint.h>
    #include <stddef.h>
    typedef uint32_t u32;
#else
    #include "utils.h"
    #define NULL ((void*)0)
#endif

#if __STDC_HOSTED__
char heap[524288];
#else
char* heap = (char*)0x10000;
#endif

#define HEAP_ORDER 19

typedef uint32_t u32;

typedef struct Header_{
    u32 used: 1,
        order: 5,
        prev: 13,
        next: 13;
} Header;

struct MB_MemInfo{
    u32 size;
    u32 addr;
    u32 length;
    u32 type;
    u32 attributes;
};
struct MB_MemoryMapping{
    u32 length;
    struct MB_MemInfo* addr;
};
struct MultibootInfo{
    struct MB_MemoryMapping map;
};

Header* freeList[HEAP_ORDER+1];

void memory_init();
void initHeader(Header* h, unsigned order);
void* kmalloc(u32 size);
void addToFreeList(Header* h);
Header* removeFromFreeList(unsigned i);
void removeThisNodeFromFreeList(Header* h);
void splitBlockOfOrder(unsigned i);