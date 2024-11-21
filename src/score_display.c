#include "stm32f0xx.h"
#include <stdint.h>
#include "score_display.h"
#include "checkstick.h"

// Global Variables
static uint16_t display_buffer[8] = {0}; // Buffer for the 7-segment display.


//===========================================================================
// Initialize GPIO and peripherals for the display.
//===========================================================================
void init_score_display(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN; // Enable GPIOB clock.

    // Set PB12, PB13, and PB15 to general purpose output mode (01).
    GPIOB->MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER15);
    GPIOB->MODER |= 0x45000000; // Configure as outputs.

    // Set initial states.
    GPIOB->BSRR = GPIO_BSRR_BS_12; // Set PB12 high (CS).
    GPIOB->BSRR = GPIO_BSRR_BR_13; // Set PB13 low (SCK).
    GPIOB->BSRR = GPIO_BSRR_BS_15; // Set PB15 high (SDI).
}

//===========================================================================
// Continuously update the display with the current score.
//===========================================================================
void drive_score_display(void)
{
    while (1)
    {
        update_display_buffer(); // Update display buffer with the current score.

        for (int d = 0; d < 8; d++)
        {
            bb_write_halfword(display_buffer[d]); // Write each digit to the display.
            small_delay();
        }
    }
}

//===========================================================================
// Update the display buffer to match the current score.
//===========================================================================
void update_display_buffer(void)
{
    int temp_score = score; // Copy the current score.
    for (int i = 7; i >= 0; i--)
    {
        if (temp_score > 0)
        {
            display_buffer[i] = digit_to_segment(temp_score % 10); // Convert digit to segment encoding.
            temp_score /= 10;
        }
        else
        {
            display_buffer[i] = 0; // Blank out unused digits.
        }
    }
}

//===========================================================================
// Map a digit (0-9) to its corresponding 7-segment encoding.
//===========================================================================
uint16_t digit_to_segment(int digit)
{
    static const uint16_t segment_map[10] = {
        0b00111111, // 0
        0b00000110, // 1
        0b01011011, // 2
        0b01001111, // 3
        0b01100110, // 4
        0b01101101, // 5
        0b01111101, // 6
        0b00000111, // 7
        0b01111111, // 8
        0b01101111  // 9
    };
    return segment_map[digit];
}

//===========================================================================
// Write a 16-bit halfword to the display using bit-banging.
//===========================================================================
void bb_write_halfword(int halfword)
{
    GPIOB->BSRR = GPIO_BSRR_BR_12; // Set PB12 low (CS).

    for (int i = 15; i >= 0; i--)
    {
        bb_write_bit((halfword >> i) & 0x1); // Write each bit.
    }

    GPIOB->BSRR = GPIO_BSRR_BS_12; // Set PB12 high (CS).
}

//===========================================================================
// Write a single bit to the display using bit-banging.
//===========================================================================
void bb_write_bit(int val)
{
    if (val)
    {
        GPIOB->BSRR = GPIO_BSRR_BS_15; // Set SDI high.
    }
    else
    {
        GPIOB->BSRR = GPIO_BSRR_BR_15; // Set SDI low.
    }

    small_delay();
    GPIOB->BSRR = GPIO_BSRR_BS_13; // Set SCK high.
    small_delay();
    GPIOB->BSRR = GPIO_BSRR_BR_13; // Set SCK low.
}

//===========================================================================
// A small delay for timing purposes.
//===========================================================================
void small_delay(void)
{
    nano_wait(50000);
}
