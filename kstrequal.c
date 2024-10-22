#include "kstrequal.h"

int kstrequal(const char* a, const char* b) {
    // 0 if a!=b
    // 1 if a==b
    while(*a && *b && *a == *b) ((void) ((a++) , (b++)));
    return (!*a) && (!*b);
}