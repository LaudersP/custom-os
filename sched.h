#pragma once

#include "utils.h"
#include "memory.h"
#include "interrupt.h"

enum ProcessState {
    VACANT, // PCB entry not used
    READY, // Process able to run
    RUNNING, // Process is on CPU
    SLEEPING, // Process is not runnable (ex: waiting for I/O or external event)
    STARTING // Process is being initialized
};

struct PCB {
    //more about 'state' in a moment
    enum ProcessState state;
    u32 eax,ebx,ecx,edx,esi,edi,ebp,esp;
    u32 ds, es, fs, gs;
    u32 cs;
    u32 ss;
    u32 eip;
    u32 eflags;
    struct PageTable* page_table;
};

typedef void(*spawn_callback_t)(int errorcode, int pid, void* callback_data);

void scheduleInit();
void spawn(const char* path, spawn_callback_t callback, void* callback_data);
void schedule(struct InterruptContext* ctx);
void scheduleEnable();
void sched_save_process_status(int pid, struct InterruptContext* ctx, enum ProcessState newState);
void sched_restore_process_state(int pid, struct InterruptContext* ctx, enum ProcessState newState);