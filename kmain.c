__asm__(
    ".global _start\n"
    "_start:\n"
    "mov $0x10000,%esp\n"
    "push %ebx\n"
    "call _kmain"
);

#include "kprintf.h"
#include "utils.h"
#include "console.h"

struct MultibootInfo machineInfo;

void kmain(struct MultibootInfo* mbi) {
    // Desired output message
    char* message = "We the People of the United States";

    // Terminal print the message
    // kprintf("%s", message);
    
    // Setup console data
    kmemcpy(&machineInfo, mbi, sizeof(struct MultibootInfo));

    // Setup console graphics
    console_init(&machineInfo);

    // Print the message graphically
    for(int i = 0; message[i] != '\0'; i++)
        // Draw each individual character
        draw_character(message[i], 100 + (10 * i), 200);
    

    // Serial output
    kprintf("\nDONE\n");

    // Dummy hold
    while(1)
        __asm__("hlt");
}