#include "stm32f0xx.h"
#include <stdint.h>
#include <math.h> 
#include <stdio.h>
#include "lcd.h"
#include "track.h"

#define MIDDLE_POS 120
#define LEFT_POS 50
#define RIGHT_POS 190

#define THRESHOLD 20
#define Y_CENTER 287

int score = 1;
int mult = 0;
int mult_count = 0;

uint16_t msg[8] = { 0x0076,0x0100,0x0200,0x0300,0x0400,0x0500,0x0600,0x0700};

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
extern const Picture up_note;

uint32_t vol = 0;

const char font[] = {
    // digits
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67
};
 
void increase_score() {
    mult_count++;

    if (mult_count >= 20)
        mult = 3;
    else if (mult_count >= 10)
        mult = 2;
    else if (mult_count >= 5)
        mult = 1;
    else
        mult = 0;

    score = score + (1 << mult);
 }

 void decrase_score() {
    mult_count = 0;
    mult = 0;
    score--;
    if (score < 0)
        score = 0;
 }

void display_score() {
    int temp_score = score;

    msg[1] &= ~font[8];
    msg[1] |= font[1 << mult];

    for(int i = 7; i >= 4; i--) {
        msg[i] &= ~font[8];
        if (temp_score != 0) {
            msg[i] |= font[temp_score % 10];
            temp_score /= 10;
        }
    }
}

void pic_subset(Picture *dst, const Picture *src, int sx, int sy)
{
    const int dw = dst->width;
    const int dh = dst->height;
    const int sw = src->width;
    const int sh = src->height;

    // Calculate clipped copy bounds
    int start_y = (sy < 0) ? -sy : 0;
    int end_y   = (sy + dh > sh) ? (sh - sy) : dh;

    int start_x = (sx < 0) ? -sx : 0;
    int effective_width = (sx + dw > sw) ? (sw - sx) : dw; 
    int copy_w = effective_width - start_x;

    if (copy_w <= 0 || end_y <= start_y) {
        return; // Nothing to copy after clipping
    }

    uint16_t *dst_line = dst->pix2 + start_x;
    const uint16_t *src_line = src->pix2 + (sy + start_y) * sw + (sx + start_x);

    for (int y = start_y; y < end_y; y++) {
        memcpy(dst_line + y * dw, src_line + y * sw, copy_w * sizeof(uint16_t));
    }
}

// Overlay a picture onto a destination picture.
// xoffset,yoffset are the offset into the destination picture that the
// source picture is placed.
// Any pixel in the source that is the 'transparent' color will not be
// copied.  This defines a color in the source that can be used as a
// transparent color.
// Optional: If you have a precomputed mask for the source image where bit=1 means opaque.
// This is just a conceptual snippet. The mask would be computed beforehand.
// uint8_t *src_mask; // For each pixel, a bit indicating if opaque or not.

void pic_overlay(Picture *dst, int xoffset, int yoffset, const Picture *src, int transparent)
{
    const int sw = src->width;
    const int sh = src->height;
    const int dw = dst->width;
    const int dh = dst->height;

    // Only allow padding (negative offset) for top edge
    int start_y = (yoffset < 0) ? -yoffset : 0;
    // For bottom edge, strictly clip to destination bounds
    int end_y = (yoffset + sh > dh) ? (dh - yoffset) : sh;

    // Strict clipping for left and right edges
    int start_x = (xoffset < 0) ? -xoffset : 0;
    int end_x = (xoffset + sw > dw) ? (dw - xoffset) : sw;

    if (end_y <= start_y || end_x <= start_x || yoffset >= dh) {
        return; // Nothing to draw
    }

    // Rest of the function remains the same...
    uint16_t *dst_ptr = dst->pix2 + (yoffset + start_y) * dw + (xoffset + start_x);
    const uint16_t *src_ptr = src->pix2 + start_y * sw + start_x;

    int copy_height = end_y - start_y;
    int copy_width = end_x - start_x;

    for (int row = 0; row < copy_height; row++) {
        const uint16_t *src_row = src_ptr + row * sw;
        uint16_t *dst_row = dst_ptr + row * dw;

        for (int col = 0; col < copy_width; col++) {
            uint16_t p = src_row[col];
            if (p != transparent) {
                dst_row[col] = p;
            }
        }
    }
}


void internal_clock();

// Uncomment only one of the following to test each step
#define SHELL

#ifdef SHELL
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
    TIM2->ARR = 800 - 1;

    // Enable the UIE bit in the DIER to enable the UIE flag
    // This will enable an update interrupt to occur each time the free-running counter of the timer reaches the ARR value and starts back at zero.
    TIM2->DIER |= TIM_DIER_UIE;

    // Enable the interrupt for Timer 2 in the NVIC ISER.
    NVIC->ISER[0] |= 1 << TIM2_IRQn;

    // Enable Timer 2
    TIM2->CR1 |= TIM_CR1_CEN;
}

// Create a canvas to composite Pictures
TempPicturePtr(canvas, 29, 35);

#define x_offset 14 // canvas width/2
#define y_offset 18 // canvas height/2

volatile int display_update_needed = 0;
volatile int notes_to_update[100];  // Track which notes need updating
volatile int notes_update_count = 0;

void TIM2_IRQHandler() {
    // Acknowledge the interrupt
    TIM2->SR &= ~TIM_SR_UIF;
    
    notes_update_count = 0;  // Reset count for this frame

    for (int i = 0; i < 100; i++) {
        note * current_note = &Track[i];
        
        // Move the note down the screen
        current_note->position += 1;

        if (current_note->position + y_offset >= 0 && current_note->position - y_offset <= 320) {
            // Mark this note for update
            notes_to_update[notes_update_count++] = i;
        }

        if (current_note->played == 0 && current_note->position > (Y_CENTER + THRESHOLD)) {
            current_note->played = 1;
            decrase_score();
            display_update_needed = 1;
        }
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

void init_spi2(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    GPIOB->MODER &= ~(GPIO_MODER_MODER15| GPIO_MODER_MODER13| GPIO_MODER_MODER12);
    GPIOB->MODER |= GPIO_MODER_MODER15_1| GPIO_MODER_MODER13_1| GPIO_MODER_MODER12_1;
    GPIOB->AFR[1] &= 0x0f00ffff;
    SPI2->CR1 &= ~(SPI_CR1_SPE);
    SPI2->CR1 |= SPI_CR1_BR_0| SPI_CR1_BR_1 |SPI_CR1_BR_2;
    SPI2->CR2 |= SPI_CR2_DS_0|SPI_CR2_DS_1|SPI_CR2_DS_2|SPI_CR2_DS_3;
    SPI2->CR1 |= SPI_CR1_MSTR;
    SPI2->CR2 |= SPI_CR2_NSSP | SPI_CR2_SSOE | SPI_CR2_TXDMAEN;
    SPI2->CR1 |= SPI_CR1_SPE;

}

void spi2_setup_dma(void) {
    SPI2->CR2 |= SPI_CR2_TXDMAEN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
    DMA1_Channel5->CMAR = (uint32_t) &msg;
    DMA1_Channel5->CPAR = (uint32_t) &SPI2->DR;
    DMA1_Channel5->CNDTR = 8;
    DMA1_Channel5->CCR |= DMA_CCR_DIR;
    DMA1_Channel5->CCR |= DMA_CCR_MINC;

    DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0;
    DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0;
    
    DMA1_Channel5->CCR |= DMA_CCR_CIRC;
    
}

void init_tim15(void) {
    RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
    TIM15->PSC = 4800-1;
    TIM15->ARR = 100000-1;
    TIM15->DIER |= TIM_DIER_UDE;
    TIM15->CR1 |= TIM_CR1_CEN;

}

void spi2_enable_dma(void) {
    DMA1_Channel5->CCR |= DMA_CCR_EN;
}

void enable_ports(){

    RCC->AHBENR |= (RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOCEN);
    GPIOA->MODER |= (GPIO_MODER_MODER1);

    // outputs PC 0,2
    GPIOC->MODER &= ~(0x33);
    GPIOC->MODER |= 0x11;

    // input PA8,9,10

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

#define BCSIZE 6
int bcsum = 0;
int boxcar[BCSIZE];
int bcn = 0;
int score_reset = 0;

note * note_pointer = &Track[0];

int32_t readpin(int32_t pin_num) {
    int32_t num = 0;
    num = GPIOA->IDR;
    num &= (1 << pin_num);

    return (num);
}

int32_t readbuttons(){
    int pin_value = 0;

    if (readpin(8)) pin_value += LEFT_POS;
    if (readpin(9)) pin_value += MIDDLE_POS;
    if (readpin(10)) pin_value += RIGHT_POS;

    return pin_value;
}

void TIM3_IRQHandler(void) {

    TIM3->SR &= ~TIM_SR_UIF;

    while (note_pointer->position > (Y_CENTER + THRESHOLD)) {
        //note_pointer->played = 1;
        note_pointer++;
    }
    
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
    //if((score_count == 10) || (score_count < 0)){
    //    score_count = 0;
    //}
    if ((vol > 3500) && (score_reset == 0)){
        // GPIOC->BSRR = GPIO_BSRR_BS_0;
        score_reset = 1;
        // score_count++;
        // msg[0] = font[score_count];

        if (note_pointer->position > (Y_CENTER - THRESHOLD) && !note_pointer->played) {
            // CHECK BUTTONS
            if ((note_pointer->dir == UP_NOTE) && (note_pointer->string == readbuttons())) {
                note_pointer->played = 1;
                increase_score();
            }
        } else {
            decrase_score();
        }

    } 
    else if(vol < 1000 && (score_reset == 0)){
        // GPIOC->BSRR = GPIO_BSRR_BS_0;
        score_reset = 1;
        // score_count--;
        // msg[0] = font[score_count];

        if (note_pointer->position > (Y_CENTER - THRESHOLD) && !note_pointer->played) {
            // CHECK BUTTONS
            if ((note_pointer->dir == DOwN_NOTE) && (note_pointer->string == readbuttons())) {
                note_pointer->played = 1;
                increase_score();
            }
        } else {
            decrase_score();
        }

    } 
    else if (vol < 3500 && vol > 1000) {
    //     GPIOC->BSRR = GPIO_BSRR_BR_0;
    //     GPIOC->BSRR = GPIO_BSRR_BR_2;
        score_reset = 0;
        // msg[0] = font[score_count];

    }
    display_score();
}

void init_tim3(void) {
    
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // ADC Sample Rate
    TIM3->PSC = 480-1; // Trigger at 100 Hz
    TIM3->ARR = 1000-1;

    TIM3->DIER |= TIM_DIER_UIE;

    NVIC_EnableIRQ(TIM3_IRQn);
    //NVIC_SetPriority(TIM3_IRQn, 0);
    
    TIM3->CR1 |= TIM_CR1_CEN;
    NVIC_SetPriority(TIM2_IRQn, 3);
}


int main() {

    internal_clock();

    LCD_Setup();
    init_spi2();
    spi2_setup_dma();
    spi2_enable_dma();
    init_tim15();

    LCD_DMA_Init();

    // Draw the background
    LCD_DrawPicture(0, 0, &background);
    enable_ports();
    setup_adc();

    while (readbuttons() != LEFT_POS + MIDDLE_POS + RIGHT_POS);

    init_tim3();
    init_tim2();

    while(1) {
        // Handle display updates outside the interrupt
        if (notes_update_count > 0) {
            for (int i = 0; i < notes_update_count; i++) {
                note * current_note = &Track[notes_to_update[i]];
                
                int x_draw_pos = current_note->string - x_offset;
                int y_draw_pos = current_note->position - y_offset;

                pic_subset(canvas, &background, x_draw_pos, y_draw_pos);
                
                if (current_note->dir == 0)
                    pic_overlay(canvas, 0, 6, &red_note, 0xffff);
                else
                    pic_overlay(canvas, 0, 6, &up_note, 0xffff);

                LCD_DrawPictureDMA(x_draw_pos, y_draw_pos, canvas);
            }
            notes_update_count = 0;  // Reset after handling
        }

        if (display_update_needed) {
            display_score();
            display_update_needed = 0;
        }
    }
}
#endif
