#include "stm32f0xx.h"
#include "lcd.h"

void setup_dma5(void) {
    // Enables the RCC clock to the DMA controller
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    // Turn off the enable bit for the channel first, like with every other peripheral.
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;

    // Set CMAR later
    // DMA1_Channel3->CMAR

    // Set CPAR to the destination address (SPI Data register)
    DMA1_Channel3->CPAR = (uint32_t) (&(SPI1->DR)); // maybe change to u_int32_t

    // Set CNDTR
    // DMA1_Channel3->CNDTR = 8;

    // Set the DIRection for copying from-memory-to-peripheral.
    DMA1_Channel3->CCR |= DMA_CCR_DIR;


    DMA1_Channel3->CCR |= DMA_CCR_MINC;

    // Set the Memory datum SIZE to 16-bit.
    DMA1_Channel3->CCR &= ~DMA_CCR_MSIZE;
    DMA1_Channel3->CCR |= DMA_CCR_MSIZE_0;

    // Set the Peripheral datum SIZE to 16-bit.
    DMA1_Channel3->CCR &= ~DMA_CCR_PSIZE;
    DMA1_Channel3->CCR |= DMA_CCR_PSIZE_0;

    // Set the channel for CIRCular operation.
    // DMA1_Channel3->CCR |= DMA_CCR_CIRC;

    DMA1->CSELR &= ~DMA_CSELR_C3S;
    DMA1->CSELR |= (1 << 8);
}

// New function to start DMA transfer
void start_dma_transfer(const void* data, uint16_t count) {
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1_Channel3->CMAR = (uint32_t)data;
    DMA1_Channel3->CNDTR = count;
    DMA1_Channel3->CCR |= DMA_CCR_EN;
}