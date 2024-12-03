#include "syscall.h"

void sleep(unsigned msec){
    do_syscall(SYSCALL_SLEEP, msec, 0, 0);
}
