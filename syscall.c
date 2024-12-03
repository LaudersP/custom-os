#include "syscall.h"
#include "syscalldefs.h"
#include "errno.h"
#include "kprintf.h"
#include "sched.h"

void syscall_handler(struct InterruptContext* ctx) {
    int req = ctx->eax;

    // Check that req is valid
    if(req < 0) {
        ctx->eax = EINVAL;
        return;
    }
    
    switch(req) {
        case SYSCALL_TEST:
            kprintf("Syscall test! %d %d %d\n",
                ctx->ebx, ctx->ecx, ctx->edx );
            ctx->eax = ctx->ebx + ctx->ecx + ctx->edx;
            break;
        case SYSCALL_READ:
            // Look at descriptor
            switch(ctx->ebx) {
                case 0: // Read from keyboard
                    ctx->eax =  ENOSYS;
                    break;
                case 1: 
                case 2:
                    ctx->eax =  EINVAL;
                    break;
                default:    // >= 3 means call file_read()
                    ctx->eax =  ENOSYS;
                    break;
            }
            break;
        case SYSCALL_WRITE:
            // Look at descriptor
            switch(ctx->ebx) {
                case 0: 
                    // Can't write to keyboard
                    ctx->eax = EINVAL;
                    break;
                case 1: 
                case 2: {
                    // Output data to the screen
                    char* buffer = (char*)ctx->ecx;

                    // Iterate through the buffer
                    for(unsigned i = 0; i < ctx->edx; i++) {
                        // Print the current character
                        kprintf("%c", buffer[i]);
                    }
                    
                    // Signal the number of outputted bits
                    ctx->eax = ctx->edx;
                    break;
                }
                default:
                    // Call file_write()
                    ctx->eax = ENOSYS;
                    break;
            }
            break;
        case SYSCALL_SLEEP: {
            unsigned howLong = ctx->ebx;
            ctx->eax = 0;   //return value from syscall
            sched_put_to_sleep_for_duration(howLong, ctx);
            schedule(ctx);
            //we are now in the new process's context
            return;
        }
        default:
            ctx->eax = ENOSYS;
            break;
    }
}