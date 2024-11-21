#ifndef TRACK_H
#define TRACK_H

#include <stdint.h>

#define MIDDLE_POS 120
#define LEFT_POS 50
#define RIGHT_POS 190

#define DOwN_NOTE 0
#define UP_NOTE 1

typedef struct {
    int   position;
    uint8_t string; 
    uint8_t dir;
    uint8_t played;
} note;

extern note Track[100];  // Declaration

#endif // TRACK_H