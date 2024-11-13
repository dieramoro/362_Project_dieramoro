#ifndef __MAIN_H
#define __MAIN_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f0xx.h"

/* Function Prototypes */
void SystemClock_Config(void);
void GPIO_Init(void);
void TIM2_Init(void);
void TIM3_Init(void);

#endif /* __MAIN_H */
