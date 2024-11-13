#include "main.h"


void TIM2_Init(void) {
    /* Enable TIM2 clock */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* Configure prescaler and auto-reload for 100ms interrupts */
    TIM2->PSC = 47999;       // Divide clock by 48,000
    TIM2->ARR = 99;          // Generate 100ms interval
    TIM2->CNT = 0;           // Reset counter
    TIM2->DIER |= TIM_DIER_UIE; // Enable update interrupt
    TIM2->CR1 |= TIM_CR1_CEN;   // Start the timer

    /* Enable TIM2 interrupt in NVIC */
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_SetPriority(TIM2_IRQn, 2);
}

void TIM3_Init(void) {
    /* Enable TIM3 clock */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* Configure prescaler and auto-reload for ~1 kHz PWM */
    TIM3->PSC = 47;          // Divide clock by 48
    TIM3->ARR = 999;         // Generate 1 kHz frequency
    TIM3->CCR1 = 500;        // Set 50% duty cycle
    TIM3->CCER |= TIM_CCER_CC1E; // Enable output on channel 1
    TIM3->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos); // PWM mode 1
    TIM3->CR1 |= TIM_CR1_CEN; // Start the timer
}

void SystemClock_Config(void) {
    RCC->CR |= RCC_CR_HSION;                // Enable HSI
    while (!(RCC->CR & RCC_CR_HSIRDY));     // Wait until HSI is ready

    RCC->CFGR |= RCC_CFGR_SW_HSI;           // Select HSI as system clock source
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI); // Wait for switch

    SystemCoreClockUpdate();                // Update SystemCoreClock variable
}

void GPIO_Init(void) {
    /* Enable GPIOA clock */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* Configure PA0–PA4 as input with pull-up resistors */
    GPIOA->MODER &= ~0x3FF;  // Clear MODER bits for PA0–PA4 (input mode)
    GPIOA->PUPDR |= 0x155;   // Enable pull-up resistors for PA0–PA4
}
