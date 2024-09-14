__asm__(
    ".global _start\n"
    "_start:\n"
    "mov $0x10000,%esp\n"
    "push %ebx\n"
    "call _kmain"
);

#include "kprintf.h"
#include "utils.h"
#include "serial.h"
#include "console.h"

struct MultibootInfo machineInfo;

void sweet_both();

void kmain(struct MultibootInfo* mbi) {
    // Desired output message
    // char* message = "We \rthe \nPeople \tof the \e[31mU\e[32mn\e[33mi\e[34mt\e[35me\e[36md\e[37m  \e[101mStates";
    
    // Setup console data
    kmemcpy(&machineInfo, mbi, sizeof(struct MultibootInfo));

    // Setup console graphics
    console_init(&machineInfo);

    sweet_both();

    // Print the message graphically
    // for(int i = 0; message[i] != '\0'; i++)
        // // Draw each character using serial_putc
        // console_putc(message[i]);

    // Output done status
    serial_putc('\n');
    serial_putc('D');
    serial_putc('O');
    serial_putc('N');
    serial_putc('E');
    serial_putc('\n');

    // Dummy hold
    while(1)
        __asm__("hlt");
}