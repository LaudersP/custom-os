#include "memory.h"

char* heap = (char*)0x10000;
Header* freeList[HEAP_ORDER+1];

void memory_init() {
    Header* initialBlock = (Header*)heap;
    initHeader(initialBlock, HEAP_ORDER);
    freeList[HEAP_ORDER] = initialBlock;
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

static Header* getPrev(Header* h) {
    unsigned delta = (h->prev) << 6;
    Header* h2 = (Header*)(heap + delta);
    if(h == h2)
        return NULL;
    return h2;
}

static void setPrev(Header* h, Header* prev ) {
    if( prev == NULL )
        prev = h;
    char* c = (char*) prev;
    unsigned delta = c-heap;
    delta >>= 6;
    h->prev = delta;
}

void initHeader(Header* h, unsigned order){
    h->used=0;
    h->order = order;
    setNext(h,NULL);
    setPrev(h,NULL);
}

void addToFreeList(Header *h) {
    // Get the order
    unsigned order = h->order;

    // Set the next pointer
    setNext(h, freeList[order]);

    // Check to see if the order is used
    if(freeList[order] != NULL)
        // Set the previous pointer
        setPrev(freeList[order], h);

    // Set the new head of the free list to h
    freeList[order] = h;

    // Set the previous pointer of h
    setPrev(h, NULL);
}

Header* removeFromFreeList(unsigned i) {
    Header* h = freeList[i];

    // Check if the list is empty
    if (h == NULL) {
        return NULL;
    }

    // Set head of the free list to the next block
    freeList[i] = getNext(h);

    // Check for a new head exists
    if (freeList[i] != NULL) {
        // Set its prev pointer to NULL
        setPrev(freeList[i], NULL);
    }

    // Clear the pointers of the removed block
    setNext(h, NULL);
    setPrev(h, NULL);

    return h;
}

void removeThisNodeFromFreeList(Header* h){
    unsigned order = h->order;

    // Check if the node is the head of the list
    if (freeList[order] == h) {
        freeList[order] = getNext(h);
    }

    // Update next and prev pointers
    if (getNext(h)) {
        setPrev(getNext(h), getPrev(h));
    }
    if (getPrev(h)) {
        setNext(getPrev(h), getNext(h));
    }

    // Clear the pointers
    setNext(h, NULL);
    setPrev(h, NULL);
}

void splitBlockOfOrder(unsigned i) {
    // Take block off freeList[i]
    Header* h = removeFromFreeList(i);

    // Split it in half
    h->order--;
    Header* h2 = (Header *)((char *)h + (1 << (h->order)));
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
        splitBlockOfOrder(i);
        i--;
    }

    Header* h = removeFromFreeList(o);
    h->used=1;
    return ((char*)h + sizeof(Header));
}

Header* getBuddy(Header *h) {
    char* c = (char*) h;
    unsigned offset = c - heap;

    offset = offset ^ (1 << h->order);

    return (Header*)(heap + offset);
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
            removeThisNodeFromFreeList(h);
            removeThisNodeFromFreeList(b);
            if(h > b)
                h=b;

            h->order++;
            addToFreeList(h);
        } else {
            return;     //done
        }
    }
}

// Return 0 if same
// Return -1 if ptr1 < ptr2
// Return 1 if ptr1 > ptr2
int kmemcmp(const void* ptr1, const void* ptr2, const unsigned numOfBytes) {
    const unsigned char* p1 = (const unsigned char*) ptr1;
    const unsigned char* p2 = (const unsigned char*) ptr2;

    // Iterate through each byte in memory
    for(unsigned i = 0; i < numOfBytes; i++) {
        // Check if p1 is less than p2
        if(p1[i] < p2[i])
            return -1;
        // Check if p1 is greater than p2
        else if(p1[i] > p2[i])
            return 1;
    }

    return 0;
}

void* kmemset(void* dest, const int value, const unsigned count) {
    unsigned char* ptr = (unsigned char*)dest;
    
    // Iterate through each byte in memory
    for(unsigned i = 0; i < count; i++) {
        // Set the memory data
        ptr[i] = (unsigned char)value;
    }

    return dest;
}

// -------- Paging Code --------
#define PAGE_DEVICE_MEMORY ( (1<<3) | (1<<4) )
#define PAGE_PRESENT 1
#define PAGE_MUST_BE_ONE (1<<7)
#define PAGE_USER_ACCESS (1<<2)
#define PAGE_WRITEABLE (1<<1)

static struct PageTable kernelPageTable;

void pagingInit(struct MultibootInfo* info) {
    // Setup the page table
    for(int i = 0; i < 1024; i++) {
        // Set the frame number
        unsigned e = (i << 22);

        // Set the required bits
        e |= PAGE_MUST_BE_ONE;

        // Set the writable bit
        e |= PAGE_WRITEABLE;

        // Check if PTE corresponds to memory between 4MB and 8MB mark
        unsigned addr = i*4*1024*1024;
        if(addr >= 4*1024*1024 && addr < 8*1024*1024)
            e |= PAGE_USER_ACCESS;
        
        // Check if the entry is device memory
        if (addr >= 2*1024*1024)
            e |= PAGE_DEVICE_MEMORY;

        // Mark entry as present if it falls within mapped ranges
        if(addr >= 128*1024*1024 && addr < 2*1024*1024) {
            // Empty
        } else {
            e |= PAGE_PRESENT;
        }

        kernelPageTable.table[i] = e;
    }

    // Tell the CPU where the table is
    setPageTable(&kernelPageTable);

    // Enable paging
    enablePaging();
}

void setPageTable(struct PageTable* p) {
    asm volatile( "mov %%eax,%%cr3"
        :
        : "a"( (unsigned)(p->table) )
        : "memory" );
}

struct PageTable* getPageTable() {
    unsigned p;
    asm volatile( "mov %%cr3,%%eax"
        : "=a"(p)
    );
    return (struct PageTable*)p;
}

void enablePaging() {
    asm volatile(
        "mov %%cr4,%%eax\n"     //copy cr4 to eax
        "orl $16,%%eax\n"       //turn on bit 4
        "mov %%eax,%%cr4\n"     //copy eax back to cr4
        "mov %%cr0,%%eax\n"     //copy cr0 to eax
        "orl $0x80010000,%%eax\n"   //turn on bits 16 and 31
        "mov %%eax,%%cr0\n"     //copy eax back to cr0
        "jmp flushqueue%=\n"    //intel says we need this
        "flushqueue%=:\n"
        "nop"                   //no-op
        :                       //no outputs
        :                       //no inputs
        : "eax","memory"        //clobbers
    );
}

unsigned getFaultingAddress() {
    unsigned addr;
    asm volatile(
        "mov %%cr2,%%eax\n"
        : "=a"(addr)
    );
    return addr;
}