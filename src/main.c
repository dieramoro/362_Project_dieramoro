#include "main.h"
#include "game.h"
#include "game_display.h"
#include "score_display.h"
#include "sound.h"

int main(void) {
    SystemClock_Config();  // system clock
    GPIO_Init(); // GPIO for buttons
    TIM2_Init(); // Timer 2 for game updates
    TIM3_Init(); // Timer 3 for sound (PWM)

    init_notes(); // Initialize game notes

    return 0;
}
