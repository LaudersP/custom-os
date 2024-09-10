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
    static int cc = 0;
    static int cr = 0;

    // Output to the terminal
    serial_putc(c);

    // Act on the type of character
    switch(c) {
        case '\n':
            cc = 0;
            cr++;
            break;

        case '\r':
            cc = 0;
            break;\

        case '\t':
            cc += 8 - cc % 8;
            break;

        case '\f':
            clear_screen();
            cc = 0;
            cr = 0;
            break;

        case '\e':
            // TBD
            break;

        case '\x7f':
            // Check that the cursor is not at x = 0, y = 0
            if(cc != 0 && cr != 0)
                cc--;
            // Check if x = 0, return to end of previous line
            else if(cc == 0 && cr != 0) {
                cc = COLUMNS - 1;
                cr--;
            }
            
            draw_character(' ', cc * CHAR_WIDTH, cr * CHAR_HEIGHT);

            break;

        default:
            draw_character(c, cc++ * CHAR_WIDTH, cr * CHAR_HEIGHT);
            break;
    };

    // Check for column overflow
    if(cc == COLUMNS) {
        cc = 0;
        cr++;
    }
}