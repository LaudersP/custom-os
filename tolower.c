#include "tolower.h"

char tolower(char c) {
    // Check for valid character
    return (c >= 'A' && c <= 'Z') ? c+32 : c;
}