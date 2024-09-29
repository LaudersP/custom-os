#pragma once

#include "utils.h"

typedef struct Header_{
    u32 used: 1,
        order: 5,
        prev: 13,
        next: 13;
} Header;

// Global variables
static char* heap = (char*) 0x10000;    // Location of beginning of stack
#define HEAP_ORDER 19                   // 2^19 = 512MB
Header* freeList[HEAP_ORDER+1];         // List to track the free list of memory heaps


void memory_init(struct MultibootInfo* info);
static void initHeader(Header* h, unsigned order);
static Header* getNext(Header* h);
static void setNext(Header* h, Header* next);
static Header* getPrev(Header* h);
static void setPrev(Header* h, Header* prev);
static void addToFreeList(Header* h);

void* kmalloc(u32 size);
static unsigned roundUpToPowerOf2(unsigned needed_bytes );
static void splitBlockOfOrder(unsigned i);
static Header* removeFromFreeList(unsigned i);

void kfree(void* v);
static Header* getBuddy(Header* h);
static Header* RemoveThisNodeFromFreeList(Header* h);
static Header* combine(Header* h, Header* b);