#ifndef DISPLAY_H
#define DISPLAY_H

/* Function prototypes */
void clear_screen(void);
void draw_note(int row, int column);
void draw_strum_zone(int strum_zone);
void draw_score(int score);
void refresh_screen(void);
void display_game_over(int score);

#endif
