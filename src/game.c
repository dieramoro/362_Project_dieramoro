#include "game.h"
#include "display.h"
#include "sound.h"
#include <stdlib.h>

/* Global variables */
NoteGroup noteGroups[MAX_NOTE_GROUPS];
int score = 0;

void init_notes() {
    for (int i = 0; i < MAX_NOTE_GROUPS; i++) {
        noteGroups[i].row = -1;      // Offscreen
        noteGroups[i].played = false; // Not played
        for (int j = 0; j < 4; j++) {
            noteGroups[i].columns[j] = false; // No notes in any lane
        }
    }
}

// this is a rough idea, idk how we will generate based on the song yet
void generate_note() {
    for (int i = 0; i < MAX_NOTE_GROUPS; i++) {
        if (noteGroups[i].row == -1) { // Find an empty slot
            noteGroups[i].row = 0;      // Start at the top
            noteGroups[i].played = false;

            // Randomly activate lanes
            for (int j = 0; j < 4; j++) {
                noteGroups[i].columns[j] = (rand() % 2 == 0); // 50% chance of a note
            }

            break;
        }
    }
}

void update_notes() {
    for (int i = 0; i < MAX_NOTE_GROUPS; i++) {
        if (noteGroups[i].row != -1) { // If the group is active
            noteGroups[i].row++;        // Move down

            // Deactivate group if it moves offscreen
            if (noteGroups[i].row > MAX_ROWS) {
                noteGroups[i].row = -1;
                noteGroups[i].played = false;
                for (int j = 0; j < 4; j++) {
                    noteGroups[i].columns[j] = false;
                }
            }
        }
    }
}

void validate_input(bool button_states[4], bool strum_button_pressed) {
    for (int i = 0; i < MAX_NOTE_GROUPS; i++) {
        if (noteGroups[i].row == STRUM_ZONE && !noteGroups[i].played) { // Group in strum zone
            bool success = true; // Assume success

            for (int j = 0; j < 4; j++) {
                if (noteGroups[i].columns[j]) { // Lane is active
                    if (!button_states[j]) {    // Button not pressed
                        success = false;
                        break;
                    }
                } else { // Lane is inactive
                    if (button_states[j]) { // Button pressed unnecessarily
                        success = false;
                        break;
                    }
                }
            }

            // Validate success
            if (success && strum_button_pressed) {
                noteGroups[i].played = true; // Mark the group as played
                score += 10;                // Increment score
                play_success_sound();
            } else if (strum_button_pressed) {
                play_fail_sound();          // Incorrect input
            }
        }
    }
}
