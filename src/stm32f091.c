#include "game.h"
#include "game_display.h"
#include "score_display.h"
#include "main.h"

void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) { // Check update interrupt flag
        TIM2->SR &= ~TIM_SR_UIF; // Clear interrupt

        update_notes(); // Move notes
        validate_input(); // Validate button input
        refresh_game_display();  // Refresh game
        refresh_score_display(); // Refresh score
    }
}
