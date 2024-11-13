#include "encoder.h"
#include "encoder_hal.h"

/* Pin definitions for the encoder */
static pin_t dt_pin =
  { GPIOB, GPIO_PIN_13 };
static pin_t clk_pin =
  { GPIOB, GPIO_PIN_14 };

/*
 * @brief: reads the value of pins and selects event and switch based on it
 * @return: value of current event and state of the switch
 */
encoder_event_t
EN_get_inputs ()
{
  encoder_event_t event = none;
  uint8_t encoder_read = HAL_GPIO_ReadPin (dt_pin.per, dt_pin.pin)
      + 2 * HAL_GPIO_ReadPin (clk_pin.per, clk_pin.pin);
  switch (encoder_read)
    {
    case 1:
      event = B;
      break;
    case 2:
      event = A;
      break;
    case 0:
      event = AB;
      break;
    default:
    }
  return event;
}
