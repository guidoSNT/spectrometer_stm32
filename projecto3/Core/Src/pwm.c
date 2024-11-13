#include "pwm.h"

static uint16_t wave[BASE_FREQ / MIN_FREQ_HIGH] =
  { 0 };
static TIM_HandleTypeDef *timer;
static uint32_t channel;

/*
 * @brief: Saves the timer to be used and the channel of the pwm
 * @inputs:
 *   @htim: Handler of the timer to be used
 *   @Channel: This is the channel used for the pwm
 *   @tim: Timer reference used for bit operation (TIM1,TIM2,etc)
 */
void
PWM_init (TIM_HandleTypeDef *htim, uint32_t Channel)
{
  timer = htim;
  channel = Channel;
}
/*
 * @brief: Changes the frequency of the sinusoidal wav e
 * @freq: This is the value of the frequency in Hz
 * @amplitude: Value of the amplitude from 0 to 1
 */
void
PWM_change_freq_dma (uint32_t freq, float amp)
{
  if (freq <= MAX_FREQ && freq >= MIN_FREQ_LOW && amp <= 1)
    {
      if (freq < MIN_FREQ_HIGH)
	{
	  freq *= 10;
	  amp *= 10;
	  __HAL_TIM_SET_AUTORELOAD(timer, COUNTER_MAX*10);
	}
      else
	{
	  __HAL_TIM_SET_AUTORELOAD(timer, COUNTER_MAX);
	}
      uint16_t amplitude = amp * COUNTER_MAX;
      HAL_TIM_PWM_Stop_DMA (timer, channel);
      uint16_t wave_size = BASE_FREQ / freq;
      for (uint16_t step = 0; step < wave_size; step++)
	{
	  wave[step] = (uint16_t) ((float) amplitude / 2.0
	      * (sinf (2.0 * M_PI * step / (float) wave_size) + 1.0));
	}
      HAL_TIM_PWM_Start_DMA (timer, channel, (uint32_t*) wave, wave_size);
      ;
    }
}
