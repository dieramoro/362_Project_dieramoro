#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

/* Game settings */
#define MAX_NOTE_GROUPS 10
#define STRUM_ZONE 15
#define MAX_ROWS 20
#define MAX_COLUMNS 4

/* NoteGroup structure */
typedef struct {
    int row; // Vertical position of the group
    bool columns[4]; // Active lanes (true if there's a note in that lane)
    bool played; // Whether the entire group has been played correctly
} NoteGroup;

/* Global variables */
extern int score;
extern NoteGroup noteGroups[MAX_NOTE_GROUPS];

/* Function prototypes */
void init_notes(void);
void generate_note(void);
void update_notes(void);
void validate_strum(bool button_states[4], bool strum_button_pressed);
void update_display(void);

#endif // GAME_H
