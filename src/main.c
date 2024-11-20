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
#include "dma.c"
#define FIFOSIZE 16
//#define MAX_Y_coordinate 2000
char serfifo[FIFOSIZE];
int seroffset = 0;

// This C macro will create an array of Picture elements.
// Really, you'll just use it as a pointer to a single Picture
// element with an internal pix2[] array large enough to hold
// an image of the specified size.
// BE CAREFUL HOW LARGE OF A PICTURE YOU TRY TO CREATE:
// A 100x100 picture uses 20000 bytes.  You have 32768 bytes of SRAM.
//
// Diego: updated to use 1 byte per pixel 
// IMAGES STILL NEED TO BE UPDATED
// Can these be created dynamically for multiple notes?
#define TempPicturePtr(name,width,height) Picture name[(width)*(height)/6+2] = { {width,height,1} }

// Create a note (37x21) plus 1 pixel of padding on all sides
TempPicturePtr(object, 39, 23);

// These can possibly be compressed further using 8 bit color and RLE compression
extern const Picture background;
extern const Picture red_note; // NEEDS TO BE TESTED

// Copy a subset of a large source picture into a smaller destination
// sx,sy are the offset into the source picture.
void pic_subset(Picture *dst, const Picture *src, int sx, int sy)
{
    int dw = dst->width;
    int dh = dst->height;
    for(int y=0; y<dh; y++) {
        if (y+sy < 0)
            continue;
        if (y+sy >= src->height)
            break;
        for(int x=0; x<dw; x++) {
            if (x+sx < 0)
                continue;
            if (x+sx >= src->width)
                break;
            dst->pix2[dw * y + x] = src->pix2[src->width * (y+sy) + x + sx];
        }
    }
}

// Overlay a picture onto a destination picture.
// xoffset,yoffset are the offset into the destination picture that the
// source picture is placed.
// Any pixel in the source that is the 'transparent' color will not be
// copied.  This defines a color in the source that can be used as a
// transparent color.
void pic_overlay(Picture *dst, int xoffset, int yoffset, const Picture *src, int transparent)
{
    for(int y=0; y<src->height; y++) {
        int dy = y+yoffset;
        if (dy < 0)
            continue;
        if (dy >= dst->height)
            break;
        for(int x=0; x<src->width; x++) {
            int dx = x+xoffset;
            if (dx < 0)
                continue;
            if (dx >= dst->width)
                break;
            unsigned short int p = src->pix2[y*src->width + x];
            if (p != transparent)
                dst->pix2[dy*dst->width + dx] = p;
        }
    }
}


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

void display_note(const Picture *pic, uint16_t x, uint16_t y){
    for(int i = y; i <= lcddev.height; i= i+ 1){
        LCD_DrawPicture(x, i, pic);
        for(int y = 0; y<=1000000; y = y+1);
        LCD_DrawPicture(0, 0, &background);

    }
}

#ifdef SHELL
#include "commands.h"
void init_spi1_slow(){
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~(GPIO_MODER_MODER3 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5);
    GPIOB->MODER |=  (GPIO_MODER_MODER3_1 | GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1);
    GPIOB->AFR[0] &=  ~(GPIO_AFRL_AFRL3 | GPIO_AFRL_AFRL5| GPIO_AFRL_AFRL4);

    // Enable SPI1 and DMA1 clocks
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    SPI1->CR1 &= ~(SPI_CR1_SPE);
    SPI1->CR1 |= 0x0038;
    SPI1->CR1 |= (SPI_CR1_MSTR);
    SPI1->CR2 |= (SPI_CR2_DS_0|SPI_CR2_DS_1|SPI_CR2_DS_2|SPI_CR2_DS_3);
    SPI1->CR2 &= ~(SPI_CR2_DS_3);  
    SPI1->CR1 |= (SPI_CR1_SSM | SPI_CR1_SSI);

    // Enable DMA requests for SPI1
    SPI1->CR2 |= SPI_CR2_TXDMAEN;

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

    LCD_Setup();
    LCD_DrawPicture(0, 0, &background);
    while(1){
        display_note(&red_note, lcddev.width/2, lcddev.height/2);   
    }

    // Diego:
    // Process for drawing a picture will be to call pic_subset to sample background
    // then pic_overlay to overlay the note object on top of it
    // then LCD_DrawPicture to push it to the display
    //int x = 100; 
    //int y = 100;
    //TempPicturePtr(tmp, 39, 23);

    //for (int i = 0; ic29*29; i++)
    //    object->pix2[i] = 0xffff;

    // Center the 19x19 ball into center of the 29x29 object.
    // Now, the 19x19 ball has 5-pixel white borders in all directions.
    //pic_overlay(object,5,5,&ball,0xffff);



    #ifdef NOTE_TEST
    pic_subset(tmp, &background, x-tmp->width/2, y-tmp->height/2); // Copy the background
    pic_overlay(tmp, 0,0, object, 0xffff); // Overlay the object
    LCD_DrawPicture(x - tmp->width/2, y - tmp->height/2, tmp); // Draw the picture
    #endif


    //command_shell();
    //init_spi1_slow();
    //enable_sdcard();
    //disable_sdcard();
    //init_sdcard_io();
    //sdcard_io_high_speed();
}
#endif

// TODO Remember to look up for the proper name of the ISR function
