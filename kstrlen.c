#include "kstrlen.h"

unsigned kstrlen(const char* s) {
    unsigned len = 0;

    while(*s++) len++;

    return len;
}