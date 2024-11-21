#ifndef TRACK_H
#define TRACK_H

#include <stdint.h>

#define MIDDLE_POS 101
#define LEFT_POS 32
#define RIGHT_POS 171

typedef struct {
    int   position;
    uint8_t string; 
} note;

extern note Track[10];  // Declaration

#endif // TRACK_H