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
#include "track.h"
#define FIFOSIZE 16
//#define MAX_Y_coordinate 2000
char serfifo[FIFOSIZE];
int seroffset = 0;

#define MIDDLE_POS 101
#define LEFT_POS 32
#define RIGHT_POS 171

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
#define TempPicturePtr(name,width,height) Picture name[(width)*(height)/6+2] = { {width,height,2} }

// These can possibly be compressed further using 8 bit color and RLE compression
extern const Picture background;
extern const Picture red_note;

// Copy a subset of a large source picture into a smaller destination
// sx,sy are the offset into the source picture.
uint32_t vol = 0;
void pic_subset(Picture *dst, const Picture *src, int sx, int sy)
{
    int dw = dst->width;
    int dh = dst->height;
    for(int y=0; y<dh; y++) {
        if (y+sy < 0)
            break; // changed from continue
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

// Uncomment only one of the following to test each step
#define SHELL

#ifdef SHELL
#include "commands.h"
void init_spi1_slow(){
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~(GPIO_MODER_MODER3 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5);
    GPIOB->MODER |=  (GPIO_MODER_MODER3_1 | GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1);
    GPIOB->AFR[0] &=  ~(GPIO_AFRL_AFRL3 | GPIO_AFRL_AFRL5| GPIO_AFRL_AFRL4);

    // Enable SPI1
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

void init_tim2(void) {
    // Enable the RCC clock for TIM2.
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // Set the Prescaler (PSC) and Auto-Reload Register (ARR) to achieve 30 fps
    TIM2->PSC = 480 - 1;  // Prescaler to divide 48 MHz to 1 kHz
    TIM2->ARR = 500 - 1;

    // Enable the UIE bit in the DIER to enable the UIE flag
    // This will enable an update interrupt to occur each time the free-running counter of the timer reaches the ARR value and starts back at zero.
    TIM2->DIER |= TIM_DIER_UIE;

    // Enable the interrupt for Timer 2 in the NVIC ISER.
    NVIC->ISER[0] |= 1 << TIM2_IRQn;

    // Enable Timer 2
    TIM2->CR1 |= TIM_CR1_CEN;
}


void TIM2_IRQHandler() {
    // Acknowledge the interrupt
    TIM2->SR &= ~TIM_SR_UIF;

    // Create a canvas
    TempPicturePtr(canvas, 39, 21);

    for (int i = 0; i < 10; i++) {

        note * current_note = &Track[i];

        // Move the note down the screen
        current_note->position += 1;

        // Copy the background to canvas
        pic_subset(canvas, &background, current_note->string, current_note->position);
        // Overlay the object with black (0x0) set to transparent w/ 1 px padding
        pic_overlay(canvas, 0, 1, &red_note, 0x0);
        // Draw the canvas
        LCD_DrawPictureDMA(current_note->string, current_note->position, canvas);
        
    }

}

void displayStartMessage(u16 x, u16 y, u16 fc, u16 bg, u8 size, u8 mode) {
    const char *message = "Press all three note buttons to start";
    LCD_DrawString(x, y, fc, bg, message, size, mode);
    // while (1) {
    //     // Check the state of buttons (assumes buttons are active low)
    //     if ((GPIOA->IDR & (1 << 0)) != 0 &&  // PA0 pressed
    //         (GPIOA->IDR & (1 << 1)) != 0 &&  // PA1 pressed
    //         (GPIOA->IDR & (1 << 2)) != 0) {  // PA2 pressed
    //         break;  // Exit the loop when all buttons are pressed
    //     }
    // }

}

void enable_ports(){
void enable_ports(){

    RCC->AHBENR |= (RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOCEN);
    GPIOA->MODER |= (GPIO_MODER_MODER1);

    // outputs PC 0,2
    GPIOC->MODER &= ~(0x33);
    GPIOC->MODER |= 0x11;

}

void setup_adc(void) {

    // Enable clock to GPIOA
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    // Configure pin for ADC_IN6: PA6, output voltage
   // GPIOA->MODER |= 0x3000;

    // Enable clock for ADC peripheral
    RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

    // Enable the High-Speed Internal (HSI14) clock, wait until its ready
    RCC->CR2 |= RCC_CR2_HSI14ON;
    while ((RCC->CR2 & RCC_CR2_HSI14RDY) == 0);

    // Enable ADEN bit, wait until ready
    ADC1->CR |= ADC_CR_ADEN;
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);

    ADC1->CHSELR |= ADC_CHSELR_CHSEL1;
    // while ((ADC1->ISR & ADC_ISR_ADRDY) == 0);
    // ADC1->CFGR2 &= ~ADC_CFGR2_CKMODE;
}

#define BCSIZE 32
int bcsum = 0;
int boxcar[BCSIZE];
int bcn = 0;

void TIM3_IRQHandler(void) {

    TIM3->SR &= ~TIM_SR_UIF;  

    ADC1->CR |= ADC_CR_ADSTART;
    // Wait for EOC bit
    while ((ADC1->ISR & ADC_ISR_EOC) == 0);

    bcsum -= boxcar[bcn];
    bcsum += boxcar[bcn] = ADC1->DR;
    bcn +=1;
    if(bcn >= BCSIZE){
        bcn = 0;
    }
    vol = bcsum / BCSIZE;

    if (vol > 2000) GPIOC->BSRR = GPIO_BSRR_BS_0;
    else if(vol < 1000) GPIOC->BSRR = GPIO_BSRR_BS_2;
    else {
        GPIOC->BSRR = GPIO_BSRR_BR_0;
        GPIOC->BSRR = GPIO_BSRR_BR_2;
    }
    


}

void init_tim3(void) {
    
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->PSC = 480-1; // Trigger at 100 Hz
    TIM3->ARR = 1000-1;

    TIM3->DIER |= TIM_DIER_UIE;

    NVIC_EnableIRQ(TIM3_IRQn);
    
    TIM3->CR1 |= TIM_CR1_CEN;
    // NVIC_SetPriority(TIM2_IRQn, 3);
}




int main() {

    internal_clock();

    // Put red_note into note object w/ 1 px of padding
    //pic_overlay(note, 1, 1, &red_note, 0xffff);

    LCD_Setup();
    // displayStartMessage(0,0,0xFFFF, 0x0000, 12, 0);

    LCD_DMA_Init();

    // Draw the background
    LCD_DrawPicture(0, 0, &background);
    enable_ports();
    setup_adc();
    init_tim3();
    // while(1){
    //     display_note(&red_note, lcddev.width/2, lcddev.height/2);   
    // }

    init_tim2();
}
#endif
