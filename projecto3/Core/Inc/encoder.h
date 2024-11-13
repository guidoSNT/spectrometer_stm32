#ifndef INC_ENCODER_H_
#define INC_ENCODER_H_

#include "stdint.h"

/* Encoder switch state */
typedef enum
{
  off, on
} switch_t;

/* Values for the encoder current position and the state of the switch */
typedef struct
{
  uint16_t position;
} encoder_t;

/* Enums with the states and events of the encoder */
typedef enum
{
  A1, A2, A3, B1, B2, B3, idle
} encoder_state_t;

typedef enum
{
  A, B, AB, none
} encoder_event_t;

/* Enums with the states and events for the button */
typedef enum
{
  b_pre_on, b_on, b_pre_off, b_off
} button_state_t;
typedef enum
{
  timeout_1, timeout_2, set, reset
} button_event_t;

/*typedef for the buttons */
typedef struct
{
  switch_t value;
  int16_t tick_counter;
  uint16_t max;
  button_event_t button_event;
  button_state_t button_state;
} button_t;

encoder_t
EN_get_value ();

void
EN_fsm (encoder_state_t *encoder_state, encoder_event_t encoder_event);

void
EN_button (switch_t pin_state, button_t *button);

void
EN_fsm_button (button_t *button);
#endif
