#include "memory.h"

#define NULL ((void*)0)

void memory_init(struct MultibootInfo* info) {
    // Number of memory regions
    u32 nr = info->map.length / sizeof(struct MB_MemInfo);
    kprintf("Num regions: %d\n",nr);

    struct MB_MemInfo* M = info->map.addr;

    for(int i=0;i<nr;++i){
        u32 end = M[i].addr+M[i].length;
        kprintf("Region %d: addr=0x%08x...0x%08x length=%dKB type=%s\n",
            i,
            M[i].addr,
            end-1,
            M[i].length/1024,
            (M[i].type == 1) ? "RAM" : "Reserved"
        );
    }
}

static void initHeader(Header* h, unsigned order){
    h->used=0;
    h->order = order;
    setNext(h,NULL);
    setPrev(h,NULL);
}

static Header* getNext(Header* h) {
    unsigned delta = (h->next) << 6;
    Header* h2 = (Header*)(heap+delta);
    if( h == h2 )
        return NULL;
    return h2;
}

static void setNext(Header* h, Header* next ) {
    if( next == NULL )
        next = h;
    char* c = (char*) next;
    unsigned delta = c-heap;
    delta >>= 6;
    h->next = delta;
}

static void setPrev(Header* h, Header* prev ) {
    if( prev == NULL )
        prev = h;
    char* c = (char*) prev;
    unsigned delta = c-heap;
    delta >>= 6;
    h->prev = delta;
}

Header* removeFromFreeList(unsigned i) {
    // Check if there is an available block of memory
    if (!freeList[i])
        return NULL;

    // Get the first element in the free list 
    Header* h = freeList[i];

    // Remove the element from the free list
    Header* next = getNext(h);
    if(next != NULL)
        setPrev(next, NULL);
    
    freeList[i] = next;

    // Clear the header pointers from the removed block
    setNext(h, NULL);
    setPrev(h, NULL);

    return h;
}

void addToFreeList(Header *h) {
    // Get the order
    unsigned order = h->order;

    // Check if the list is empty
    if(!freeList[order]) {
        freeList[order] = h;
        setNext(h, NULL);
        setPrev(h, NULL);
    } else {
        Header* head = freeList[order];
        setNext(h, head);
        setPrev(head, h);
        setPrev(h, NULL);

        // Update the head of the list
        freeList[order] = h;
    }
}

static void splitBlockOfOrder(unsigned i) {
    // Take block off freeList[i]
    Header* h = removeFromFreeList(i);

    // Split it in half
    h->order--;
    Header* h2 = (Header*)((char*)h) + (1 << (h->order));
    initHeader(h2, h->order);
    
    // Add two to freeList
    addToFreeList(h);
    addToFreeList(h2);
}

unsigned roundUpToPowerOf2(unsigned needed_bytes) {
    unsigned power = 1;

    while(power < needed_bytes)
        power *= 2;

    return power;
}

void* kmalloc(u32 size) {
    // Add the bytes used for the header
    size += sizeof(Header);

    // Round up to the next power of two
    size = roundUpToPowerOf2(size);

    // PSEUDO CODE
    // Order I want
    unsigned o = 6;

    while((1 << o) < size)
        o++;

    // Order I have
    unsigned i = o;

    // Iterate through the freeList checking for free memory blocks
    while(i <= HEAP_ORDER && !freeList[i])
        i++;

    // Check if no block of free memory was found
    if(i > HEAP_ORDER)
        return NULL;

    // Split the freelist map
    while(i > o) {
        splitBlockOfOrder(i--);
    }

    Header* h = removeFromFreeList(o);
    h->used=1;
    return ((char*)h + sizeof(Header));
}


Header* getBuddy(Header *h) {
    u32 delta = ((char*)h) - heap;  // heapStart?

    // Flip bit h->order
    delta = delta^(1 << h->order);
    return (Header*)(heap + delta);
}

Header* combine(Header* h, Header* b) {
    if(h > b)
        h=b;

    h->order++;
    addToFreeList(h);

    return h;
}

void kfree(void* v) {
    char* c = (char*) v;
    Header* h = (Header*)(c-sizeof(Header));
    h->used = 0;
    addToFreeList(h);

    while(1){
        if( h->order == HEAP_ORDER )
            return;     //entire heap is free
        Header* b = getBuddy(h);
        if( b->used == 0 && b->order == h->order ){
            combine(h, b);
        } else {
            return;     //done
        }
    }
}