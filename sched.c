#include "sched.h"
#include "errno.h"
#include "exec.h"
#include "kprintf.h"

#define MAX_PROC 16
#define SKIP_IDLE_ENTRY 1

struct PCB process_table[MAX_PROC];
int current_pid = -1;

struct PageTable page_tables[MAX_PROC];

void scheduleInit() {
    // Set the kernel process table entry to STARTING
    process_table[0].state = STARTING;

    // Set all state fields to VACANT
    for(unsigned i = SKIP_IDLE_ENTRY; i < MAX_PROC; i++) {
        process_table[i].state = VACANT;
    }
}

void initialize_process_page_table(struct PageTable* p, int pid) {
    // Iterate over the table
    for(unsigned i = 0; i < 1024; i++) {
        unsigned e;

        // Check if we are at the first entry
        if(i == 1) {
            // Set the frame number to PID
            e = (pid << 22);
        }
        else {
            // Set the frame number
            e = (i << 22);
        }

        // Set the required bits
        e |= PAGE_MUST_BE_ONE;

        // Set the writable bit
        e |= PAGE_WRITEABLE;

        // Check if PTE corresponds to memory between 4MB and 8MB mark
        unsigned addr = i*4*1024*1024;
        if(addr >= 4*1024*1024 && addr < 8*1024*1024)
            e |= PAGE_USER_ACCESS;
        
        // Check if the entry is device memory
        if (addr >= (unsigned)2*1024*1024*1024)
            e |= PAGE_DEVICE_MEMORY;

        // Mark entry as present if it falls within mapped ranges
        if(addr >= 128*1024*1024 && addr < (unsigned)2*1024*1024*1024) {
            // Empty
        } else {
            e |= PAGE_PRESENT;
        }

        p->table[i] = e;
    }
}

struct SpawnInfo {
    spawn_callback_t callback;
    void* callback_data;
    int pid;
};

void spawn2(int errorcode, unsigned entryPoint, void* callback_data) {
    struct SpawnInfo* si = (struct SpawnInfo*)callback_data;
    if( errorcode ){
        process_table[si->pid].state = VACANT;
        si->callback( errorcode, -1, si->callback_data);
        kfree(si);
        return;
    }

    struct PCB* pcb = &process_table[si->pid];

    // Reset the registers
    pcb->eax = 0;
    pcb->ebx = 0;
    pcb->ecx = 0;
    pcb->edx = 0;
    pcb->esi = 0;
    pcb->edi = 0;
    pcb->ebp = 0;
    pcb->esp = 0;
    pcb->ds = 0;
    pcb->es = 0;
    pcb->fs = 0;
    pcb->gs = 0;

    pcb->cs = 24|3;   //gdt: ring 3 code segment
    pcb->ss = 32|3;   //gdt: ring 3 data segment
    pcb->esp = EXE_STACK;        //defined in exec lecture
    pcb->eip = entryPoint;
    pcb->eflags = 0x202; //see usermode lecture for flag bits
    pcb->page_table = &page_tables[si->pid];

    initialize_process_page_table(&(page_tables[si->pid]), si->pid);
    process_table[si->pid].state = READY;
    si->callback( SUCCESS, si->pid, si->callback_data);
    kfree(si);
}

void spawn(const char* path, spawn_callback_t callback, void* callback_data) {
    struct SpawnInfo* si = kmalloc(sizeof(struct SpawnInfo));
    if(!si){
        callback(ENOMEM,-1,callback_data);
        return;
    }
    
    // Scan the process table looking for a VACANT entry
    int pageTableIndex = -1;
    for(unsigned i = SKIP_IDLE_ENTRY; i < MAX_PROC; i++) {
        // Check the state of the table entry
        if(process_table[i].state == VACANT) {
            // Store the entry index
            pageTableIndex = i;
            break;
        }
    }

    // Check that an entry was found
    if(pageTableIndex == -1) {
        // Try again later
        callback(EAGAIN,-1,callback_data);
        kfree(si);
        return;
    }

    si->pid = pageTableIndex;
    si->callback=callback;
    si->callback_data=callback_data;
    process_table[pageTableIndex].state = STARTING;

    unsigned addr = si->pid*0x400000;
    kmemset( (char*)addr, 0, 4*1024*1024 );
    exec(path, addr, spawn2, si);
}

static volatile int can_schedule=0;

int scheduleSelectProcess() {
    // Iterate through the process table
    unsigned index = current_pid;

    for(index = (current_pid + 1); index < MAX_PROC; index++) {
        // Skip entry 0
        if(index == 0) continue;

        // Check if the current entry is READY or RUNNING
        if(process_table[index].state == READY || process_table[index].state == RUNNING) {
            break;
        }

        // Check for loop
        if(index == (MAX_PROC - 1))
            index = 0;
    }

    // Ensure that index is not the first table entry
    if(index == 0)
        panic("NOT THE FIRST ENTRY!\n");

    return index;
}

void schedule(struct InterruptContext* ctx) {
    if(!can_schedule)
        return;

    int new_pid = scheduleSelectProcess();
    if( new_pid == current_pid ) {
        return;
    }

    sched_save_process_status(current_pid, ctx, READY);
    sched_restore_process_state(new_pid, ctx, RUNNING);
}

void scheduleEnable() {
    can_schedule = 1;
}

static void copyRegisters(struct PCB* pcb, struct InterruptContext* ctx) {
    ctx->eax = pcb->eax;
    ctx->ebx = pcb->ebx;
    ctx->ecx = pcb->ecx;
    ctx->edx = pcb->edx;
    ctx->esi = pcb->esi;
    ctx->edi = pcb->edi;
    ctx->ebp = pcb->ebp;
    ctx->esp = pcb->esp;
    ctx->ds = pcb->ds;
    ctx->es = pcb->es;
    ctx->fs = pcb->fs;
    ctx->gs = pcb->gs;
    ctx->cs = pcb->cs;
    ctx->ss = pcb->ss;
    ctx->eip = pcb->eip;
    ctx->eflags = pcb->eflags;
}

void sched_save_process_status(int pid, struct InterruptContext* ctx, enum ProcessState newState) {
    if(current_pid == -1 )
        return;         //nothing to save

    struct PCB* pcb = &process_table[pid];
    pcb->state = newState;

    // Save registers
    copyRegisters(pcb, ctx);
}

static int sched_first_time = 1;

void sched_restore_process_state(int pid, struct InterruptContext* ctx, enum ProcessState newState) {
    if(current_pid >= MAX_PROC)
        panic("Bad PID");

    struct PCB* pcb = &process_table[pid];
    copyRegisters(pcb, ctx);

    // Set the page table to the pcb page table
    setPageTable(pcb->page_table);

    // Update the process state (pcb->state) to newState
    pcb->state = newState;

    // Update the current PID
    current_pid = pid;

    if(sched_first_time) {
        sched_first_time = 0;
        exec_transfer_control(SUCCESS, pcb->eip, NULL); // trace to spawn 2 and see what eip; 100,000 wrong (kernel load) 400,000 is desired
        panic("we should not get here");
    } else {
        copyRegisters(pcb, ctx);
    }
}