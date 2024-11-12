#include "sound.h"
#include "tim.h"

void play_success_sound() {
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_Delay(100);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

void play_fail_sound() {
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_Delay(100);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
}
