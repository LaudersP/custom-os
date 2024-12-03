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
#include "sched.h"

struct MultibootInfo machineInfo;

void kmain3(int errorcode, int pid, void* callback_data) {
    if( errorcode != SUCCESS )
        panic("Error spawning task!");

    int* countdown = (int*)callback_data;
    *countdown -= 1;
    if( *countdown == 0 )
        scheduleEnable();
    return;
}

void kmain2() {        //disk_read_metadata callback
    static int countdown=3;
    spawn("A.EXE", kmain3, &countdown);
    spawn("B.EXE", kmain3, &countdown);
    spawn("C.EXE", kmain3, &countdown);
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

    // Enable scheduling
    scheduleInit();

    // Enable paging
    pagingInit(mbi);

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