#include "kstrcpy.h"

void kstrcpy(char* d, char* s) {
    while((*d++ = *s++));
}