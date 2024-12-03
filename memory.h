#pragma once

#include "utils.h"

#define HEAP_ORDER 19
#define PAGE_DEVICE_MEMORY ( (1<<3) | (1<<4) )
#define PAGE_PRESENT 1
#define PAGE_MUST_BE_ONE (1<<7)
#define PAGE_USER_ACCESS (1<<2)
#define PAGE_WRITEABLE (1<<1)

typedef struct Header_{
    u32 used: 1,
        order: 5,
        prev: 13,
        next: 13;
} Header;

void memory_init();
void initHeader(Header* h, unsigned order);
void* kmalloc(u32 size);
void addToFreeList(Header* h);
Header* removeFromFreeList(unsigned i);
void removeThisNodeFromFreeList(Header* h);
void splitBlockOfOrder(unsigned i);
void kfree(void* v);
int kmemcmp(const void* ptr1, const void* ptr2, const unsigned numOfBytes);
void* kmemset(void* dest, const int value, const unsigned count);

// Paging Functions
extern struct PageTable kernelPageTable;
struct PageTable{
    unsigned table[1024] __attribute__((aligned(4096)));
};

void setPageTable(struct PageTable* p);
struct PageTable* getPageTable();
void enablePaging();
unsigned getFaultingAddress();
void pagingInit(struct MultibootInfo* info);