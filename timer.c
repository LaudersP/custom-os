#include "timer.h"

void timer_init(const unsigned rateDivisor) {
    divisor = rateDivisor; // Set the rate via divisor value

    outb(0x70, 0xa);    // Access rate register
    unsigned tmp = inb(0x71);  // Get the value from the RTC chip

    outb(0x70, 0xa);    // Access rate register
    outb(0x71, divisor | (tmp & 0xf0));    // Set the desired rate

    outb(0x70, 11);     // Reset the value of register 11
    unsigned sv = inb(0x71);    // Get the previous value
    outb(0x70, 11);     // Access register 11
    outb(0x71, sv | 0x40);  // Set the falg and enable interrupts
}

void increase_clock_ticks() {
    clockTicks++;
}

unsigned get_rate() {
    switch(divisor) {
        case 3:
            return 8192;
        case 4:
            return 4096;
        case 5:
            return 2048;
        case 6:
            return 1024;
        case 7:
            return 512;
        case 8:
            return 256;
        case 9:
            return 128;
        case 10:
            return 64;
        case 11:
            return 32;
        case 12:
            return 16;
        case 13:
            return 8;
        case 14:
            return 4;
        case 15:
            return 2;
    }

    return 0;
}

unsigned get_uptime() {
    return (clockTicks / get_rate() * 1000);
}