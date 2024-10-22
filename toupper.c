#include "toupper.h"

char toupper(char c) {
    // Check for valid character
    return (c >= 'a' && c <= 'z') ? c-32 : c;
}