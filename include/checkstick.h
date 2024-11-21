#include "stm32f0xx.h"
#include <stdint.h>

extern volatile int score;

int read_buttons(void);
void togglexn(GPIO_TypeDef *port, int n);
void init_exti(void);
void EXTI0_1_IRQHandler(void);     // Interrupt handler for upstrum (PB0)
void EXTI2_3_IRQHandler(void);   // Interrupt handler for downstrum (PB2)

