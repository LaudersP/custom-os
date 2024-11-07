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
#include "exec.h"

struct MultibootInfo machineInfo;

void kmain2() {
    exec("HELLO.EXE", 0x400000, exec_transfer_control, 0);
}

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

    // Responsible for reading out our VBR
    disk_read_metadata(kmain2);

    while(1){
        halt();
    }
}