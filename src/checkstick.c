#include "stm32f0xx.h"
#include <stdint.h>

int read_rows()
{
    return (~GPIOC->IDR) & 0x8; // reads our 3 input buttons
    // right now have it as PC0,1,2
}

void togglexn(GPIO_TypeDef *port, int n) {

  // if (n < 0 || n > 15) {
  //       return;
  // } //handle if not 0-15 for GPIO

    // Toggle the pin using XOR operation
    port->ODR ^= (1 << n);
}

void init_exti(){
      // Enable SYSCFG clock in RCC APB2ENR register
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  // Select PORT B for EXTI 0,1 -> Configure 0x001
  // 32 bit register so 000_000_000_000
  SYSCFG->EXTICR[0] = 0x0101; //sets to 000_001_00_001
  
  // EXTI_RTSR so that interrupt gen. on rising edge of pins
  EXTI->RTSR |= (EXTI_RTSR_TR0 | EXTI_RTSR_TR2);

  // Unmask interrupt for each pin in EXTI_IMR register
  EXTI->IMR |= (EXTI_IMR_IM0 | EXTI_IMR_IM2);

  // Enable the three interrupts for EXTI pins 0-1, 2-3, 4-15
  // NVIC_EnableIRQ(EXTI0_1_IRQn);

  NVIC->ISER[0] |= (1 << EXTI0_1_IRQn); // pos 5
  NVIC->ISER[0] |= (1 << EXTI2_3_IRQn); // pos 7
  
}

// occurs at PB0 rising edge
void EXTI0_1_IRQHandler(){

  EXTI->PR = EXTI_PR_PR0; // clear interrupt flag 
  // logic for strum up
}

void EXTI2_3_IRQHandler(){

  EXTI->PR = EXTI_PR_PR2; // clear interrupt flag 
  // logic for strum down
}
