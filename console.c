#include "utils.h"
#include "serial.h"
#include "console.h"
#include "etec3701_10x20.h"
#include "kprintf.h"

static volatile u8* framebuffer;
static unsigned pitch;
static unsigned width;
static unsigned height;
static u16 foregroundColor = 0xffff;
static u16 backgroundColor = 0x0000;

static void clear_screen() {
    volatile u8* f8 = framebuffer;

    for(unsigned y = 0; y < height; y++) {
        volatile u16* f16 = (volatile u16*) f8;

        for(unsigned x = 0; x < width; x++) {
            f16[x] = backgroundColor;
        }

        f8 += pitch;
    }
}

void console_init(struct MultibootInfo* info){
    framebuffer = (volatile u8*) (info->fb.addr);
    pitch = (unsigned) (info->fb.pitch);
    width = (unsigned) (info->fb.width);
    height = (unsigned) (info->fb.height);
    
    clear_screen();
}

void set_pixel(unsigned x, unsigned y, u16 color) {
    volatile u16* p = ((volatile u16*) framebuffer + y * (pitch / sizeof(u16)) + x);
    *p = color;
}

void draw_character(unsigned char ch, int x, int y) {
    for(unsigned r = 0; r < CHAR_HEIGHT; ++r) {
        for(unsigned c = 0; c < CHAR_WIDTH; ++c) {
            if(font_data[(unsigned) ch][r] & (1 << (CHAR_WIDTH - 1 - c)))
                set_pixel(x + c, y + r, foregroundColor);
            else
                set_pixel(x + c, y + r, backgroundColor);
        }
    }
}

void console_putc(char c) { 
    serial_putc(c);
}