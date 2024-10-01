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
#include "interrupt.h"
#include "timer.h"
#include "disk.h"

struct MultibootInfo machineInfo;

void sweet();

void kmain(struct MultibootInfo* mbi) {
    // Setup console data
    kmemcpy(&machineInfo, mbi, sizeof(struct MultibootInfo));

    // Setup console graphics
    console_init(&machineInfo);

    // Initialize interrupts
    interrupt_init();

    // Enable the timer
    timer_init(12);

    // Enable memory
    memory_init();

    // Enable to disk system
    disk_init();

    // Enable interrupts
    interrupt_enable();

    sweet();

    // Dummy hold
    while(1)
        halt();
}