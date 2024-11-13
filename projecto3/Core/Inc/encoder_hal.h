#ifndef INC_ENCODER_HAL_H_
#define INC_ENCODER_HAL_H_

#include "stm32f1xx_hal.h"
#include "encoder.h"

/* Struct to describe the pins used */
typedef struct
{
  GPIO_TypeDef *per;
  uint16_t pin;
} pin_t;

encoder_event_t
EN_get_inputs (void);

#endif
