#include "stm32f0xx.h"
#include "ili9341.h"

// Buttons and LCD Pin Assignments
#define STRUM_BUTTON_PIN  GPIO_PIN_15
#define NOTE_BUTTONS_MASK (GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14)
#define BUTTONS_GPIO_PORT GPIOB

// LCD Settings
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320
#define NOTE_WIDTH    20
#define NOTE_HEIGHT   20
#define STRUM_ZONE_Y_START (SCREEN_HEIGHT - 70)
#define STRUM_ZONE_HEIGHT 30

// Timing
#define TIMER_PRESCALER 48000 - 1  // Prescaler for 1kHz
#define TIMER_PERIOD     50        // Auto-reload for 50ms

// Colors
#define COLOR_WHITE 0xFFFF
#define COLOR_GREEN 0x07E0
#define COLOR_RED   0xF800
#define COLOR_BLACK 0x0000

// Game Parameters
#define TOTAL_NOTES 25

// Structs
typedef struct {
    uint8_t columns;   // Binary configuration of the note (1–15)
    int y_position;    // Vertical position on screen
    uint16_t color;    // Note color (white, green, or red)
} Note;

// Globals
Note notes[TOTAL_NOTES];
int total_notes_generated = 0;
int active_notes = 0;
int timer_flag = 0;
int notes_played = 0;
int notes_missed = 0;
int score = 0;

// Function Prototypes
void GPIO_Init(void);
void Timer_Init(void);
void generate_note(Note *note);
void draw_note(Note note);
void erase_note(Note note);
void game_loop(void);
void handle_strum(void);
void reset_game(void);

// Main Function
int main(void) {
    // Initialize hardware
    GPIO_Init();
    Timer_Init();
    ILI9341_Init();
    ILI9341_DisplayOn();

    reset_game();

    while (1) {
        game_loop();  // Continuously run the game loop
    }
}

// GPIO Initialization
void GPIO_Init(void) {
    // Enable GPIOB clock
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    // Configure note and strum buttons as inputs
    GPIOB->MODER &= ~(GPIO_MODER_MODER11 | GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER14 | GPIO_MODER_MODER15);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR11 | GPIO_PUPDR_PUPDR12 | GPIO_PUPDR_PUPDR13 | GPIO_PUPDR_PUPDR14 | GPIO_PUPDR_PUPDR15);
}

// Timer Initialization
void Timer_Init(void) {
    // Enable TIM3 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // Configure TIM3 for 50ms intervals
    TIM3->PSC = TIMER_PRESCALER;  // 48 MHz / 48000 = 1 kHz
    TIM3->ARR = TIMER_PERIOD;    // 1 kHz / 50 = 50ms
    TIM3->DIER |= TIM_DIER_UIE;  // Enable update interrupt
    TIM3->CR1 |= TIM_CR1_CEN;    // Start the timer

    // Enable TIM3 interrupt in NVIC
    NVIC_EnableIRQ(TIM3_IRQn);
    NVIC_SetPriority(TIM3_IRQn, 0);
}

// Timer Interrupt Handler
void TIM3_IRQHandler(void) {
    TIM3->SR &= ~TIM_SR_UIF;  // Clear the update interrupt flag
    timer_flag = 1;             // Increment the flag for the main loop
}

// Generate a New Note
// just random generated for now, can easy change
void generate_note(Note *note) {
    note->columns = (rand() % 15) + 1;  // Random binary configuration (1–15)
    note->y_position = 0;              // Start at the top
    note->color = COLOR_WHITE;         // Default white color
}

// Draw a Note
void draw_note(Note note) {
    for (int i = 0; i < 4; i++) {
        if (note.columns & (1 << i)) {  // Check if the bit is set
            int x_position = (60 * i) + (60 - NOTE_WIDTH) / 2;
            ILI9341_FillRectangle(x_position, note.y_position, NOTE_WIDTH, NOTE_HEIGHT, note.color);
        }
    }
}

// Erase a Note
void erase_note(Note note) {
    for (int i = 0; i < 4; i++) {
        if (note.columns & (1 << i)) {
            int x_position = (60 * i) + (60 - NOTE_WIDTH) / 2;
            ILI9341_FillRectangle(x_position, note.y_position, NOTE_WIDTH, NOTE_HEIGHT, COLOR_BLACK);
        }
    }
}

// Game Loop
void game_loop(void) {
    static int note_timer = 0;

    if (timer_flag) {
        timer_flag = 0;  // Reset the flag

        // Update note positions
        for (int i = 0; i < active_notes; i++) {
            erase_note(notes[i]);
            notes[i].y_position++;
            draw_note(notes[i]);

            // Handle notes exiting the screen
            if (notes[i].y_position > SCREEN_HEIGHT) {
                if (notes[i].color == COLOR_WHITE) {
                    notes_missed++;  // Note was never played
                    score -= 5;      // Penalize for unplayed note
                }

                // Shift remaining notes forward in the array
                for (int j = i; j < active_notes - 1; j++) {
                    notes[j] = notes[j + 1];
                }

                active_notes--;  // Decrement active note count
                i--;             // Recheck this index
            }
        }

        // Generate a new note every 2000ms
        note_timer += 50;
        if (note_timer >= 2000 && total_notes_generated < TOTAL_NOTES) {
            generate_note(&notes[active_notes]);
            active_notes++;
            total_notes_generated++;
            note_timer = 0;
        }

        // Handle strum input
        if (GPIOB->IDR & STRUM_BUTTON_PIN) {
            handle_strum();
        }

        // Check if the game has ended
        if (total_notes_generated == TOTAL_NOTES && active_notes == 0) {
            reset_game();
        }
    }
}

// Handle Strum Input
void handle_strum(void) {
    uint8_t player_input = 0;

    // Read GPIO inputs for note buttons
    for (int i = 0; i < 4; i++) {
        if (GPIOB->IDR & (GPIO_PIN_11 << i)) {
            player_input |= (1 << i);  // Set corresponding bit
        }
    }

    // Check if a note is in the strum zone
    for (int i = 0; i < active_notes; i++) {
        if (notes[i].y_position >= STRUM_ZONE_Y_START &&
            notes[i].y_position <= STRUM_ZONE_Y_START + STRUM_ZONE_HEIGHT) {
            if (player_input == notes[i].columns) {
                notes[i].color = COLOR_GREEN;  // Correct input
                score += 10;                  // Increase score
                notes_played++;
            } else {
                notes[i].color = COLOR_RED;   // Incorrect input
                notes_missed++;
                score -= 5;                   // Decrease score
            }
            return;  // Handle one note per strum
        }
    }

    // No notes in the strum zone
    score -= 5;  // Penalize for an incorrect strum
}

// Reset the Game
void reset_game(void) {
    total_notes_generated = 0;
    active_notes = 0;
    notes_played = 0;
    notes_missed = 0;
    score = 0;
    ILI9341_FillScreen(COLOR_BLACK);  // Clear the screen
    ILI9341_DisplayString(20, 150, "Press STRUM to Start", COLOR_WHITE, COLOR_BLACK, 2);
    while (!(GPIOB->IDR & STRUM_BUTTON_PIN));  // Wait for strum button press
    ILI9341_FillScreen(COLOR_BLACK);
}
