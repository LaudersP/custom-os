#include "interrupt.h"
#include "kprintf.h"

// Midlevel handler assembly code
asm (
    ".global _midlevel_handler\n"
    "_midlevel_handler:\n"
    "   push %ebp\n"
    "   push %edi\n"
    "   push %esi\n"
    "   push %edx\n"
    "   push %ecx\n"
    "   push %ebx\n"
    "   push %eax\n"
    "   push %esp\n"        //address of stack top = addr of eax
    "   cld\n"      //clear direction flag; C expects this
    "   call _highlevel_handler\n"
    "   addl $4, %esp\n"    //discard parameter for C
    "   pop %eax\n"
    "   pop %ebx\n"
    "   pop %ecx\n"
    "   pop %edx\n"
    "   pop %esi\n"
    "   pop %edi\n"
    "   pop %ebp\n"
    "   add $8,%esp\n"
    "   iret\n"
);

// Initializes all of the handler items
void interrupt_init() {
    gdt_init();

    idt_inti();

    struct LIDT lidt;
    lidt.size = sizeof(idt);
    lidt.addr = idt;
    asm volatile("lidt (%%eax)" : : "a"(&lidt));
}

// Initializes the Global Descriptor Table
void gdt_init() {
    // Set GDT index 0
    gdt[0].limitBits0to15 = 0;
    gdt[0].baseBits0to15 = 0;
    gdt[0].baseBits16to23 = 0;
    gdt[0].flags = 0;
    gdt[0].flagsAndLimitBits16to19 = 0;
    gdt[0].baseBits24to31 = 0;

    // Set code segment entry
    gdt[1].baseBits0to15=0;
    gdt[1].baseBits16to23=0;
    gdt[1].baseBits24to31=0;
    gdt[1].limitBits0to15=0xffff;
    gdt[1].flags = SEGMENT_TYPE_CODE |
                   SEGMENT_IS_CODE_OR_DATA |
                   SEGMENT_RING_0 |
                   SEGMENT_PRESENT ;
    gdt[1].flagsAndLimitBits16to19 =
                    SEGMENT_LIMIT_HIGH_NIBBLE |
                    SEGMENT_LIMIT_32_BIT |
                    SEGMENT_32_BIT;

    // Set data segment entry
    gdt[2].baseBits0to15=0;
    gdt[2].baseBits16to23=0;
    gdt[2].baseBits24to31=0;
    gdt[2].limitBits0to15=0xffff;
    gdt[2].flags = SEGMENT_TYPE_DATA |
                   SEGMENT_IS_CODE_OR_DATA |
                   SEGMENT_RING_0 |
                   SEGMENT_PRESENT ;
    gdt[2].flagsAndLimitBits16to19 =
                    SEGMENT_LIMIT_HIGH_NIBBLE |
                    SEGMENT_LIMIT_32_BIT |
                    SEGMENT_32_BIT;

    // Set the table and initialize the segment registers
    struct LGDT lgdt;
    lgdt.size = sizeof(gdt);
    lgdt.addr = &gdt[0];
    void* tmp = &lgdt;
    asm volatile (
         "lgdt (%%eax)\n"       //load gdt register
         "mov $16,%%eax\n"      //set eax to 16: Offset to gdt[2]
         "mov %%eax,%%ds\n"     //store 16 to ds
         "mov %%eax,%%es\n"     //store 16 to es
         "mov %%eax,%%fs\n"     //store 16 to fs
         "mov %%eax,%%gs\n"     //store 16 to gs
         "mov %%eax,%%ss\n"     //store 16 to ss
         "jmpl $0x8,$reset_cs\n"    //Intel says we must do a jmp here
         "reset_cs:\n"
         "nop\n"                //no operation
         : "+a"(tmp)
         :
         : "memory"
    );
}

// Intitialize the Interrupt Descriptor Table
void idt_inti() {
    for(unsigned i = 0; i < NUM_INTERRUPTS; i++) {
        u32 a = (u32)(lowlevel_addresses[i]);
        idt[i].addrLow = (u16)(a & u32_LOW_MASK);
        idt[i].addrHigh = (u16)((a & u32_HIGH_MASK) >> 16);
        idt[i].selector = 8;
        idt[i].zero = 0;
        idt[i].flags = 0x8e;
    }
}

void highlevel_handler(struct InterruptContext* ctx) {
    switch (ctx->interruptNumber) {
        case 0:
            kprintf("Divide by zero\n");
            break;
        case 1:
            kprintf("Debug\n");
            break;
        case 2:
            kprintf("Hardware\n");
            break;
        case 3:
            kprintf("Breakpoint\n");
            break;
        case 4:
            kprintf("Overflow\n");
            break;
        case 5:
            kprintf("Bound exceeded\n");
            break;
        case 6:
            kprintf("Undefined opcode\n");
            break;
        case 7:
            kprintf("No coprocessor\n");
            break;
        case 8:
            kprintf("Double fault\n");
            break;
        case 10:
            kprintf("Invalid TSS\n");
            break;
        case 11:
            kprintf("Segment absent\n");
            break;
        case 12:
            kprintf("Stack fault\n");
            break;
        case 13:
            kprintf("General fault\n");
            break;
        case 14:
            kprintf("Page fault\n");
            break;
        case 16:
            kprintf("Math fault\n");
            break;
        case 17:
            kprintf("Alignment error\n");
            break;
        case 18:
            kprintf("Machine check\n");
            break;
        case 19:
            kprintf("SIMD error\n");
            break;
        case 20:
            kprintf("Virtual error\n");
            break;
        case 21:
            kprintf("Control error\n");
            break;
        case 9:
        case 15:    
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
            kprintf("Reserved\n");
            break;
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
        case 48:
        case 49:
            kprintf("Hardware/User\n");
            break;
    }
}