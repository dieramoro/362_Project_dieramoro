/**
  ******************************************************************************
  * @file    main.c
  * @author  Weili An, Niraj Menon
  * @date    Feb 7, 2024
  * @brief   ECE 362 Lab 7 student template
  ******************************************************************************
*/

/*******************************************************************************/

// Fill out your username!  Even though we're not using an autotest, 
// it should be a habit to fill out your username in this field now.
const char* username = "silva48";

/*******************************************************************************/ 

#include "stm32f0xx.h"
#include <stdint.h>
#include "fifo.h"
#include "tty.h"
#include <math.h> 
#include <stdio.h>
#include "lcd.h"
extern const Picture background;
#define FIFOSIZE 16
char serfifo[FIFOSIZE];
int seroffset = 0;


void internal_clock();
////////////DAC START//////////////////////
void setup_dac(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER &= 0xfffffcff;
    GPIOA->MODER |= 0x00000300;
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR &= ~0x38; 
    DAC->CR |= DAC_CR_TEN1;
    DAC->CR |= DAC_CR_EN1;

}

//============================================================================
// Timer 6 ISR
//============================================================================
// Write the Timer 6 ISR here.  Be sure to give it the right name.
void TIM6_DAC_IRQHandler(){
  TIM6->SR &= ~TIM_SR_UIF;
  //int samp = wavetable[offset0>>16] + wavetable[offset1>>16];
  //samp *= volume;
  //samp = samp >> 17; 
  //samp += 2048;
  //DAC->DHR12R1 = samp;
  
}

//============================================================================
// init_tim6()
//============================================================================
void init_tim6(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    //TIM6->PSC = (480000/RATE)-1;
    TIM6->ARR = 100-1;
    TIM6->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] |= 1 << TIM6_IRQn;
    TIM6->CR1 |= TIM_CR1_CEN;
    TIM6->CR2 |= 0x20;

}

////////////DAC END/////////////////////////

// Uncomment only one of the following to test each step
#define SHELL

void init_usart5() {
    // TODO
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    RCC->AHBENR |= RCC_AHBENR_GPIODEN;
    GPIOC->MODER &= ~(GPIO_MODER_MODER12);
    GPIOC->MODER |=  GPIO_MODER_MODER12_1;
    GPIOC->AFR[1] &= 0xfff0ffff;
    GPIOC->AFR[1] |= 0x00020000;
    GPIOD->MODER &= ~(GPIO_MODER_MODER2);
    GPIOD->MODER |=  GPIO_MODER_MODER2_1;
    GPIOD->AFR[0] &= 0xfffff0ff;
    GPIOD->AFR[0] |= 0x00000200;
    RCC->APB1ENR |= 0x00100000;
    USART5->CR1  &= ~(USART_CR1_UE | USART_CR1_M1| USART_CR1_M0| USART_CR1_OVER8| USART_CR1_PCE);
    USART5->CR2 &= ~(USART_CR2_STOP);
    USART5->BRR = 0x1a1;
    USART5->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    while(!((USART5->ISR & USART_ISR_TEACK) && (USART5->ISR & USART_ISR_REACK))){

    }   
}


void enable_tty_interrupt(void) {
    // TODO
    USART5->CR1 |= USART_CR1_RXNEIE;
    NVIC->ISER[0] |= 1<< 29;
    USART5->CR3 |= USART_CR3_DMAR;
    RCC->AHBENR |= RCC_AHBENR_DMA2EN;
    DMA2->CSELR |= DMA2_CSELR_CH2_USART5_RX;
    DMA2_Channel2->CCR &= ~DMA_CCR_EN;

    DMA2_Channel2->CMAR = (uint32_t) serfifo;
    DMA2_Channel2->CPAR = (uint32_t) &USART5->RDR;
    DMA2_Channel2->CNDTR = FIFOSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_DIR;
    DMA2_Channel2->CCR |= DMA_CCR_MINC;
    
    DMA2_Channel2->CCR &= ~DMA_CCR_MSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_PSIZE;
    DMA2_Channel2->CCR |= DMA_CCR_CIRC;
    DMA2_Channel2->CCR |= DMA_CCR_PL;
    DMA2_Channel2->CCR |= DMA_CCR_EN;
}

// Works like line_buffer_getchar(), but does not check or clear ORE nor wait on new characters in USART
char interrupt_getchar() {

    while(fifo_newline(&input_fifo) == 0) {
        asm volatile ("wfi");
    }
    // Return a character from the line buffer.
    char ch = fifo_remove(&input_fifo);
    return ch;
    // TODO
}
int __io_getchar(void) {
    // TODO Use interrupt_getchar() instead of line_buffer_getchar()
    int out;
    //out = line_buffer_getchar();
    out = interrupt_getchar();
    return out;
}
int __io_putchar(int c) {
    int temp = '\r';
    if(c == '\n'){

        while(!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = temp;
    }
    while(!(USART5->ISR & USART_ISR_TXE));
    USART5->TDR = c;
    return c;

} 


void USART3_8_IRQHandler(void) {
    while(DMA2_Channel2->CNDTR != sizeof serfifo - seroffset) {
        if (!fifo_full(&input_fifo))
            insert_echo_char(serfifo[seroffset]);
        seroffset = (seroffset + 1) % sizeof serfifo;
    }
}

#ifdef SHELL
#include "commands.h"
#include <stdio.h>
void init_spi1_slow(){
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~(GPIO_MODER_MODER3 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5);
    GPIOB->MODER |=  (GPIO_MODER_MODER3_1 | GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1);
    GPIOB->AFR[0] &=  ~(GPIO_AFRL_AFRL3 | GPIO_AFRL_AFRL5| GPIO_AFRL_AFRL4);
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    SPI1->CR1 &= ~(SPI_CR1_SPE);
    SPI1->CR1 |= 0x0038;
    SPI1->CR1 |= (SPI_CR1_MSTR);
    SPI1->CR2 |= (SPI_CR2_DS_0|SPI_CR2_DS_1|SPI_CR2_DS_2|SPI_CR2_DS_3);
    SPI1->CR2 &= ~(SPI_CR2_DS_3);  
    SPI1->CR1 |= (SPI_CR1_SSM | SPI_CR1_SSI);
    SPI1->CR2 |= SPI_CR2_FRXTH;
    SPI1->CR1 |= SPI_CR1_SPE;

}


void enable_sdcard(){
    GPIOB->BSRR &= ~(GPIO_BSRR_BS_2);
    GPIOB->BSRR |= (GPIO_BSRR_BR_2);
}

void disable_sdcard(){
    GPIOB->BSRR |= (GPIO_BSRR_BS_2);
    GPIOB->BSRR &= ~(GPIO_BSRR_BR_2);
}

void init_sdcard_io(){
    init_spi1_slow();
    GPIOB->MODER &= ~(GPIO_MODER_MODER2);
    GPIOB->MODER |= (GPIO_MODER_MODER2_0);
    disable_sdcard();
}


void sdcard_io_high_speed(){
    SPI1->CR1 &= ~(SPI_CR1_SPE);
    SPI1->CR1 &= ~(0x0038);
    SPI1->CR1 |= 0x0008;
    SPI1->CR1 |= SPI_CR1_SPE;

}

void init_lcd_spi(){
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER11 | GPIO_MODER_MODER14);
    GPIOB->MODER |=  (GPIO_MODER_MODER8_0 | GPIO_MODER_MODER11_0 | GPIO_MODER_MODER14_0);
    init_spi1_slow();
    sdcard_io_high_speed();
}




int main() {
    internal_clock();
    init_usart5();
    enable_tty_interrupt();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);
    lcd_init();
    LCD_DrawPicture(0, 0, &background);
    //drawexamp();
    command_shell();
    init_spi1_slow();
    enable_sdcard();
    //disable_sdcard();
    init_sdcard_io();
    sdcard_io_high_speed();
}
#endif
// TODO Remember to look up for the proper name of the ISR function
