#ifndef INC_PWM_C_
#define INC_PWM_C_

#include "stm32f1xx_hal.h"
#include "math.h"
#include "stdint.h"

/*
 ----------------------------------------------------------------
 This variables have to be modified depending on the timer config

 The MIN_FREQ can't be to low because it makes the vector of
 points to big, for MIN_FREQ = 50 and CPU_CLOCK_MHZ = 72 it's 500
 data values.
 */
#define COUNTER_MAX 2880
#define CPU_CLOCK_MHZ 72
#define MAX_FREQ 500
#define MIN_FREQ_HIGH 50
#define MIN_FREQ_LOW 4
#define BASE_FREQ ((CPU_CLOCK_MHZ*1000000)/COUNTER_MAX)
/* ---------------------------------------------------------------- */

void
PWM_init (TIM_HandleTypeDef *htim, uint32_t Channel);

void
PWM_change_freq_dma (uint32_t freq, float amp);

#endif
