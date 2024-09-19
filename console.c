#include "utils.h"
#include "serial.h"
#include "console.h"
#include "etec3701_10x20.h"
#include "kprintf.h"

#define RGB565(r, g, b) ((r << 11) | (g << 5) | b)

static volatile u8* framebuffer;
static unsigned pitch;
static unsigned width;
static unsigned height;
static u16 foregroundColor = RGB565(21, 42, 21); // Gray
static u16 backgroundColor = RGB565(0, 0, 21); // Royale blue
static int remember;
static const u16 darkColors[8] = {
    RGB565(0, 0, 0),    // Black
    RGB565(21, 0, 0),   // Dark Red
    RGB565(0, 42, 0),   // Dark Green
    RGB565(21, 42, 0),  // Olive
    RGB565(0, 0, 21),   // Royal Blue
    RGB565(21, 0, 21),  // Purple
    RGB565(0, 42, 21),  // Cyan
    RGB565(21, 42, 21)  // Gray
};

static const u16 lightColors[8] = {
    RGB565(10, 20, 10), // Dark Grey
    RGB565(31, 20, 10), // Pink
    RGB565(10, 63, 10), // Lime
    RGB565(31, 63, 10), // Yellow
    RGB565(10, 20, 31), // Light Blue
    RGB565(31, 20, 31), // Magenta
    RGB565(10, 63, 31), // Light Cyan
    RGB565(31, 63, 31)  // White
};

enum StateMachineState {
    NORMAL_CHARS,
    GOT_ESC,
    GOT_LBRACKET,
    GOT_3,
    GOT_3x,     // Must be right after GOT_3
    GOT_4,
    GOT_4x,     // Must be right after GOT_4
    GOT_9,
    GOT_9x,     // Must be right after GOT_9
    GOT_1,
    GOT_1x,     // Must be right after GOT_1
    GOT_1xx
};

static enum StateMachineState currentState = NORMAL_CHARS;

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

    // Output to the terminal (Debugging purposes)
    serial_putc(c);

    // Check if the screen needs to be scrolled
    if(cr == 30) {
        // Scroll the console
        scroll_console();

        // Set the current row to 29;
        cr = 29;
    }

    // Act on state machine state
    if(currentState == NORMAL_CHARS) {
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
                // Change the state to look for remaining escape characters
                currentState = GOT_ESC;
                return;

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
    else {
        // Act on the state machines current state
        switch (currentState) {
            case NORMAL_CHARS:
                return;
            // Act on handeling a escape key
            case GOT_ESC:
                currentState = (c == '[' ? GOT_LBRACKET : NORMAL_CHARS);
                break;

            // Act on handeling a left bracket
            case GOT_LBRACKET:
                switch (c) {
                    case '3':
                        currentState = GOT_3;
                        break;
                    case '4':
                        currentState = GOT_4;
                        break;
                    case '9':
                        currentState = GOT_9;
                        break;
                    case '1':
                        currentState = GOT_1;
                        break;
                    default:
                        currentState = NORMAL_CHARS;
                        break;
                }
                break;

            // Act on handeling the beginning of color sequence
            // Fall-through case statement handeling
            case GOT_3:
            case GOT_4:
            case GOT_9:
            case GOT_1x:
                switch(c) {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                        remember =  c - '0';
                        currentState++; // Increments to either GOT_3x, GOT_4x, GOT_9x, GOT_10x
                        break;
                    default:
                        currentState = NORMAL_CHARS;
                        break;
                }
                break;

            case GOT_3x:
                if(c == 'm') {
                    foregroundColor = darkColors[remember];
                    currentState = NORMAL_CHARS;
                }
                else {
                    currentState = NORMAL_CHARS;
                }
                break;

            case GOT_4x:
                if(c == 'm') {
                    backgroundColor = darkColors[remember];
                    currentState = NORMAL_CHARS;
                }
                else {
                    currentState = NORMAL_CHARS;
                }
                break;

            case GOT_9x:
                if(c == 'm') {
                    foregroundColor = lightColors[remember];
                    currentState = NORMAL_CHARS;
                }
                else {
                    currentState = NORMAL_CHARS;
                }
                break;

            case GOT_1:
                currentState = (c == '0' ? GOT_1x : NORMAL_CHARS);
                break;

            case GOT_1xx:
                if(c == 'm') {
                    backgroundColor = lightColors[remember];
                    currentState = NORMAL_CHARS;
                }
                else {
                    currentState = NORMAL_CHARS;
                }
                break;
        }
    }
}

void scroll_console() {
    // Copy the framebuffer up a single line
    kmemcpy(
        (void*)framebuffer,
        (void*)(framebuffer + CHAR_HEIGHT * pitch),
        pitch * (600 - CHAR_HEIGHT)
    );

    // Clear bottom console row
    for(unsigned y = (height - CHAR_HEIGHT); y < height; y++)
        for(unsigned x = 0; x < width; x++)
            set_pixel(x, y, backgroundColor);
}