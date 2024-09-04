// Serial.c

#include "serial.h"
#include "utils.h"

// 0x3fd: Tells status of serial port (ready for reads/writes)
#define SERIAL_STATUS 0x3fd

// 0x3f8: Data Transmit
#define SERIAL_DATA 0x3f8

// Bit 5 says "we can send"
#define CLEAR_TO_SEND (1<<5)

void serial_init() {
    // Nothing to do for now...
}

void serial_putc(char c) {
    while(!(inb(SERIAL_STATUS) & CLEAR_TO_SEND)) {
        // Do nothing until ready to send...
    }
    
    outb(SERIAL_DATA, c);
}