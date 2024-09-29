#include "utils.h"

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