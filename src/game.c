#include "game.h"
#include "game_display.h"
#include "sound.h"
#include <stdlib.h>
#include "main.h"

/* Global Variables */
NoteGroup noteGroups[MAX_NOTE_GROUPS];
int score = 0;

void init_notes(void) {
    for (int i = 0; i < MAX_NOTE_GROUPS; i++) {
        noteGroups[i].row = -1; // Offscreen
        noteGroups[i].played = false; // Not played
        for (int j = 0; j < 4; j++) {
            noteGroups[i].columns[j] = false; // No notes in any lane
        }
    }
}

void update_notes(void) {
    for (int i = 0; i < MAX_NOTE_GROUPS; i++) {
        if (noteGroups[i].row != -1) {
            noteGroups[i].row++;
            if (noteGroups[i].row > MAX_ROWS) {
                noteGroups[i].row = -1; // Deactivate / move offscreen
                for (int j = 0; j < 4; j++) {
                    noteGroups[i].columns[j] = false;
                }
            }
        }
    }
}

void validate_input(void) {
    uint16_t button_state = GPIOA->IDR & 0x1F; // Read PA0–PA4 (5 bits)
    bool strum_pressed = (button_state >> 4) & 0x1; // PA4
    bool button_states[4] = {
        button_state & 0x1,       // PA0
        (button_state >> 1) & 0x1,// PA1
        (button_state >> 2) & 0x1,// PA2
        (button_state >> 3) & 0x1 // PA3
    };

    for (int i = 0; i < MAX_NOTE_GROUPS; i++) {
        if (noteGroups[i].row == STRUM_ZONE && !noteGroups[i].played) {
            bool success = true;
            for (int j = 0; j < 4; j++) {
                if (noteGroups[i].columns[j] != button_states[j]) {
                    success = false;
                    break;
                }
            }
            if (success && strum_pressed) {
                noteGroups[i].played = true;
                score += 10;
                start_sound(); // Play success sound
            }
        }
    }
}
