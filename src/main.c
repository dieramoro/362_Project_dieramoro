#include "main.h"
#include "game.h"
#include "display.h"
#include "sound.h"

int main(void) {
    HAL_Init();             // Initialize HAL
    SystemClock_Config();   // Configure system clock
    MX_GPIO_Init();         // Initialize GPIO
    MX_TIM2_Init();         // Initialize Timer 2
    MX_I2C1_Init();         // Initialize I2C for display

    init_notes();           // Initialize game notes

    HAL_TIM_Base_Start_IT(&htim2); // Start timer with interrupts

    while (1) {
        bool button_states[4] = {
            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0),
            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1),
            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2),
            HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3)
        };
        bool strum_button_pressed = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);

        validate_input(button_states, strum_button_pressed);
        HAL_Delay(10);      // Small delay for debouncing
    }

    display_game_over(score); // End the game
    return 0;
}
