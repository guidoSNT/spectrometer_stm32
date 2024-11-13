#include "encoder_hal.h"

/* Variables to use in the functions */
static encoder_state_t encoder_state = idle;
static encoder_t encoder =
  { UINT16_MAX / 2 };

/*
 *@brief: gets the current state of the encoder
 *@return: current state of encoder
 */
encoder_t
EN_get_value ()
{
  encoder_event_t input = EN_get_inputs ();
  EN_fsm (&encoder_state, input);
  return encoder;
}

/*
 * @brief: returns the state of a button
 * @inputs:
 *   @pin_state: current state of the pin to be read
 *   @button: pointer to a button struct definition
 */
void
EN_button (switch_t pin_state, button_t *button)
{
  if (pin_state == on)
    (*button).button_event = set;
  else
    (*button).button_event = reset;
  if (((*button).tick_counter >= (*button).max))
    {
      (*button).button_event = timeout_1;
    }
  else if ((*button).tick_counter < 0)
    (*button).button_event = timeout_2;

  ;
  EN_fsm_button (button);
}
/*
 * @brief: gives the state of a button
 * @inputs:
 *   @button: this is the descriptor for the button
 */
void
EN_fsm_button (button_t *button)
{
  button_t temp = *button;
  switch (temp.button_state)
    {
    case b_pre_off:
      switch (temp.button_event)
	{
	case timeout_1:
	  temp.button_state = b_off;
	  temp.value = off;
	  break;
	case timeout_2:
	  temp.button_state = b_on;
	  break;
	  break;
	case set:
	  temp.tick_counter--;
	  break;
	case reset:
	  temp.tick_counter++;
	  break;
	}
      break;
    case b_off:
      temp.tick_counter = 0;
      switch (temp.button_event)
	{
	case set:
	  temp.button_state = b_pre_on;

	  break;
	default:
	}
      break;
    case b_pre_on:
      switch (temp.button_event)
	{
	case timeout_1:
	  temp.button_state = b_on;
	  temp.value = on;
	  break;
	case timeout_2:
	  temp.button_state = b_off;
	  break;
	case set:
	  temp.tick_counter++;
	  break;
	case reset:
	  temp.tick_counter--;
	  break;
	}
      break;
    case b_on:
      temp.tick_counter = 0;
      switch (temp.button_event)
	{
	case reset:
	  temp.button_state = b_pre_off;
	  break;
	default:
	}
      break;

    }
  *button = temp;
}

/*
 *@brief: gives the next state for the encoder based on the event
 *@inputs:
 *  @encoder_state: pointer to the current state of the encoder
 *  @encoder_event: last event for the encoder inputs
 */
void
EN_fsm (encoder_state_t *encoder_state, encoder_event_t encoder_event)
{
  switch (*encoder_state)
    {
    case idle:
      switch (encoder_event)
	{
	case A:
	  *encoder_state = A1;
	  break;
	case B:
	  *encoder_state = B1;
	  break;
	default:
	}
      break;
    case A1:
      switch (encoder_event)
	{
	case B:
	  *encoder_state = idle;
	  break;
	case AB:
	  *encoder_state = A2;
	  break;
	case none:
	  *encoder_state = idle;
	  break;
	default:
	}
      break;
    case A2:
      switch (encoder_event)
	{
	case A:
	  *encoder_state = A1;
	  break;
	case B:
	  *encoder_state = A3;
	  break;
	case none:
	  *encoder_state = idle;
	  break;
	default:
	}
      break;
    case A3:
      switch (encoder_event)
	{
	case A:
	  *encoder_state = A1;
	  break;
	case AB:
	  *encoder_state = A2;
	  break;
	case none:
	  *encoder_state = idle;
	  encoder.position++;
	  break;
	default:
	}
      break;
    case B1:
      switch (encoder_event)
	{
	case A:
	  *encoder_state = idle;
	  break;
	case AB:
	  *encoder_state = B2;
	  break;
	case none:
	  *encoder_state = idle;
	  break;
	default:
	}
      break;
    case B2:
      switch (encoder_event)
	{
	case A:
	  *encoder_state = B3;
	  break;
	case B:
	  *encoder_state = B1;
	  break;
	case none:
	  *encoder_state = idle;
	  break;
	default:
	}
      break;
    case B3:
      switch (encoder_event)
	{
	case B:
	  *encoder_state = B1;
	  break;
	case AB:
	  *encoder_state = B2;
	  break;
	case none:
	  *encoder_state = idle;
	  encoder.position--;
	  break;
	default:
	}
    }
}
