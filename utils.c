#include "utils.h"
#include "memory.h"
#include "errno.h"

void outb(u16 port, u8 val) {
    __asm__ volatile("outb %%al,%%dx" : : "a"(val), "d"(port));
}

u8 inb(u16 port) {
    u32 tmp;
    __asm__ volatile("inb %%dx,%%al" : "=a"(tmp) : "d"(port));
    return (u8)tmp;
}

void kmemcpy(void* dest, const void* src, unsigned size) {
    // Get the casted version of the arguments
    char* dp = (char*) dest;
    char* sp = (char*) src;

    // Loop through the memory locations using the size variable
    // ... decreases the size argument once each iteration until size = 0
    while(size--) {
        // Copy the src memory data to the dest memory location
        *dp++ = *sp++;
    }
}

void halt() {
    __asm__ volatile("hlt");
}

u16 inw(u16 port){
    u32 tmp;
    asm volatile ("inw %%dx,%%ax"
        : "=a"(tmp)         //one output
        : "d"(port)         //one input
    );
    return (u16)tmp;
}

void outw(u16 port, u16 value){
    asm volatile("outw %%ax,%%dx"
        :   //no outputs
        : "a"(value), "d"(port) //two inputs
    );
}

u32 inl(u16 port){
    u32 tmp;
    asm volatile ("inl %%dx,%%eax"
        : "=a"(tmp)         //one output
        : "d"(port)         //one input
    );
    return tmp;
}

void outl(u16 port, u32 value){
    asm volatile("outl %%eax,%%dx"
        :   //no outputs
        : "a"(value), "d"(port) //two inputs
    );
}

int queue_put(struct Queue* Q, void* data) {
    // Create a new node
    struct QueueNode *n = (struct QueueNode*)kmalloc(sizeof(struct QueueNode));

    // Check the response from malloc
    if(!n) {
        return ENOMEM;
    }

    // Set new node data
    n->item = data;
    n->next = NULL;

    // Check if queue is empty
    if(Q->head != NULL && Q->tail != NULL) {
        Q->tail->next = n;
        Q->tail = n;
    }
    else {
        Q->head = n;
        Q->tail = n;
    }
    
    return SUCCESS;
}

void* queue_get(struct Queue* Q) {
    // Check that the queue list is not empty
    if (Q->head == NULL)
        return NULL;
    
    // Get the head node
    struct QueueNode* dummy = Q->head->next;

    // Get the head nodes data
    void* tmp = Q->head->item;

    // Free up the head node
    kfree(Q->head);

    // Set the new head
    Q->head = dummy;

    // Check if there is a following node
    if(!dummy)
        // Set tail to null
        Q->tail = NULL;

    // Return the data point
    return tmp;
}