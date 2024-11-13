#include "display_analizador.h"

/* adc values saved to display on screen */
static complex_t fft_screen_points[FFT_MAX / 2];
static char buffer_fft_data[11 * 128];
static UART_HandleTypeDef *uart_sender;

/* variables for the main display state machine */
//static display_state_t display_st = fft_pre_off;
/* variable that sets the diferent inputs for the system */
static inputs_t inputs =
  {
    { 0 },
    { off, 0, 5, b_off, reset },
    { off, 0, 5, b_off, reset } };

static inputs_t inputs_comp =
  {
    { 0 },
    { off, 0, 5, b_off, reset },
    { off, 0, 5, b_off, reset } };

/* state machine vars for the config display */
static selected_state_t selected_state = not_off;
static config_disp_t current_values =
  {
    { 52, 23 }, UINT16_MAX / 2, 1, 1, 0, 0 };
static config_disp_t old_values;
static config_disp_t pwm_set;
static char strings[4][13] =
  { "Gen: A", "Gen: f", "Imprimir FFT", "Cursor" };
static char cursor_selec[13] = "PWM: f";
static char cursor_selec_amp[13] = "PWM: A";

/* values to display the triangle */
static triangle_data_t triangle_config =
  { 4, 6, 10, 14, 12, 10, 5, 102 };

void
AN_init (UART_HandleTypeDef *uart_fm)
{
  for (int i = 0; i < FFT_MAX / 2; i++)
    {
      buffer_fft_data[11 * i + 3] = ',';
      buffer_fft_data[11 * i + 5] = '.';
      buffer_fft_data[11 * i + 8] = ',';
      buffer_fft_data[11 * i + 9] = '\r';
      buffer_fft_data[11 * i + 10] = '\n';
    }
  uart_sender = uart_fm;
}

void
AN_send_uart ()
{
  for (uint16_t i = 0; i < FFT_MAX / 2; i++)
    {
      buffer_fft_data[11 * i] = '0' + (uint16_t) (i * 3.9025) / 100;
      buffer_fft_data[11 * i + 1] = '0' + ((uint16_t) (i * 3.9025) / 10) % 10;
      buffer_fft_data[11 * i + 2] = '0' + (uint16_t) (i * 3.9025) % 10;
      buffer_fft_data[11 * i + 3] = ',';
      buffer_fft_data[11 * i + 4] = '0'
	  + (uint16_t) (fft_screen_points[i].real);
      buffer_fft_data[11 * i + 5] = '.';
      buffer_fft_data[11 * i + 6] = '0'
	  + ((uint16_t) (fft_screen_points[i].real * 10)) % 10;
      buffer_fft_data[11 * i + 7] = '0'
	  + (((uint16_t) (fft_screen_points[i].real * 100)) % 100) % 10;
      buffer_fft_data[11 * i + 8] = ',';
      buffer_fft_data[11 * i + 9] = '\r';
      buffer_fft_data[11 * i + 10] = '\n';
    }
  HAL_UART_Transmit (uart_sender, buffer_fft_data, 11 * 128, HAL_MAX_DELAY);
}

/* @brief: sets the value of the encoder */
void
AN_encoder_setter ()
{
  inputs.encoder = EN_get_value ();
}

/* @brief: sets the value of the buttons */
uint8_t
AN_button_setter ()
{
  EN_button ((switch_t) !HAL_GPIO_ReadPin (GPIOB, GPIO_PIN_12),
	     &(inputs.button_encoder));
  EN_button ((switch_t) !HAL_GPIO_ReadPin (GPIOB, GPIO_PIN_1),
	     &(inputs.button_sec));
  if ((inputs.button_sec.value != inputs_comp.button_sec.value)
      || (inputs.button_encoder.value != inputs_comp.button_encoder.value))
    {
      inputs_comp = inputs;
      return 1;
    }
  inputs_comp = inputs;
  return 0;
}

void
AN_fft_encoder_fun_setter ()
{
  current_values.fft_encoder_mode = !current_values.fft_encoder_mode;
}

void
AN_fft_update_setter ()
{
  current_values.fft_pass = !current_values.fft_pass;
}
/* @brief: returns the current value of the inputs */
inputs_t
AN_inputs_reader ()
{
  return inputs;
}

config_disp_t
AN_config_reader ()
{
  return current_values;
}
/*
 * @brief: Sets the buffer to display an fft read
 * @inputs:
 *   @vector: Pointer to the vector of y-axis points
 *   @fft_len: length of the vector
 *   @cursor: position of cursor
 */
void
AN_graph_fft (uint8_t cursor)
{
  if (cursor < 0 || cursor > FFT_MAX / 2)
    return;
  char buffer[22];
  SSD1306_Fill (SSD1306_COLOR_BLACK);
  for (uint16_t pixelx = 0; pixelx < FFT_MAX / 2; pixelx++)
    {
      SSD1306_DrawLine (pixelx, SSD1306_HEIGHT, pixelx,
      SSD1306_HEIGHT - 2 * (uint16_t) fft_screen_points[pixelx].imag,
			SSD1306_COLOR_WHITE);
    }
  SSD1306_GotoXY (REC_POS_X + 1, CURSOR_HEIGHT + 1);
  switch (current_values.fft_encoder_mode)
    {
    case 0:
      sprintf (
	  buffer,
	  "%d.%.1d;%d|%d.%.1d;%d",
	  (uint16_t) (fft_screen_points[cursor].imag * MAX_V
	      / (float) (SSD1306_HEIGHT - CURSOR_HEIGHT)),
	  (uint16_t) (fft_screen_points[cursor].imag * MAX_V * 10
	      / (float) (SSD1306_HEIGHT - CURSOR_HEIGHT)) % 10,
	  (uint16_t) rintf (cursor * (float) FS / 256),
	  current_values.pwm_state.amp / 10, current_values.pwm_state.amp % 10,
	  (int) current_values.pwm_state.freq);
      AN_set_cursor (cursor, SSD1306_COLOR_WHITE);
      break;
    case 1:
      uint16_t freq_temp = WRAP_FREQ(
	  AN_wrap_value (pwm_set.option,
			 inputs.encoder.position + pwm_set.pwm_state.freq,
			 501));
      current_values.update_pwm = 0;
      if (current_values.pwm_state.freq != freq_temp)
	{
	  current_values.pwm_state.freq = freq_temp;
	  current_values.update_pwm = 1;
	}
      sprintf (buffer, "PWM: %d.%dV %dHz", current_values.pwm_state.amp / 10,
	       current_values.pwm_state.amp % 10,
	       (int) current_values.pwm_state.freq);
      break;
    case 2:
      uint16_t amp_temp = AN_wrap_value (
	  pwm_set.option, inputs.encoder.position + pwm_set.pwm_state.amp, 34);
      current_values.update_pwm = 0;
      if (current_values.pwm_state.amp != amp_temp)
	{
	  current_values.pwm_state.amp = amp_temp;
	  current_values.update_pwm = 1;
	}
      sprintf (buffer, "PWM: %d.%dV %dHz", current_values.pwm_state.amp / 10,
	       current_values.pwm_state.amp % 10,
	       (int) current_values.pwm_state.freq);
      break;
    }
  SSD1306_Puts (buffer, &Font_7x10, SSD1306_COLOR_WHITE);
}

/*
 * @brief: draws the menu for the config display
 * @inputs:
 *  @input: struct with the values of the current inputs
 */
void
AN_graph_control (switch_values_t switches, uint8_t regen)
{
  selected_event_t selected_event = not_pressed;
  char values_str[2][10] =
    { "--", "--" };
  regen = regen % 2;
  inputs_t input_cp = inputs;
  input_cp.button_encoder.value = switches.encoder;
  input_cp.button_sec.value = switches.second;

  /* Checking for selection */
  if (old_values.option != input_cp.encoder.position)
    selected_event = not_change;
  if (input_cp.button_encoder.value == on)
    {
      selected_event = pressed;
      if (old_values.option != input_cp.encoder.position)
	selected_event = pressed_change;
    }

  AN_fsm_config (&selected_state, selected_event, input_cp);

  /*Draws the boxes and part of the text */
  sprintf (values_str[0], "| %d.%dV", current_values.pwm_state.amp / 10,
	   current_values.pwm_state.amp % 10);
  sprintf (values_str[1], "| %3dHz", (int) current_values.pwm_state.freq);

  AN_draw_menu (current_values.option, current_values.fill, regen,
		values_str[0], values_str[1]);
}

/*
 * @brief: fsm for the config display an its different configurations
 * @inputs:
 *   @selected_state: pointer to the state of the current state
 *   @selected_event: current event for the display
 */
void
AN_fsm_config (selected_state_t *selected_state,
	       selected_event_t selected_event, inputs_t inputs_cp)
{

  switch (*selected_state)
    {
    case not_off:
      current_values.option = inputs_cp.encoder.position;
      current_values.update_pwm = 0;
      current_values.uart_send = 0;
      switch (selected_event)
	{
	case pressed_change:
	case pressed:
	  *selected_state = not_on;
	  current_values.fill = 1;
	  break;
	}
      break;
    case not_on:
      current_values.option = inputs_cp.encoder.position;
      switch (selected_event)
	{
	case not_change:
	case not_pressed:
	  current_values.fill = 0;
	  *selected_state = sel_off;
	  old_values.option = inputs_cp.encoder.position;
	  old_values.pwm_state = current_values.pwm_state;
	  break;
	}
      break;
    case sel_off:
      current_values.option = old_values.option;
      switch (selected_event)
	{
	case pressed_change:
	case pressed:
	  *selected_state = sel_on;
	  break;
	case not_change:
	case not_pressed:
	  switch (current_values.option % 4)
	    {
	    case 0:
	      current_values.pwm_state.amp = AN_wrap_value (
		  old_values.option,
		  inputs_cp.encoder.position + old_values.pwm_state.amp, 34);
	      break;
	    case 1:
	      current_values.pwm_state.freq = WRAP_FREQ(
		  AN_wrap_value (
		      old_values.option,
		      inputs_cp.encoder.position + old_values.pwm_state.freq,
		      501));
	      break;
	    case 3:
	      current_values.fft_encoder_mode++;
	      current_values.fft_encoder_mode = current_values.fft_encoder_mode
		  % 3;
	      //current_values.fft_encoder_mode =
	      // !current_values.fft_encoder_mode;
	      break;
	    default:
	    }
	default:
	}
      break;
    case sel_on:
      current_values.fill = 0;
      current_values.option = old_values.option;
      switch (selected_event)
	{
	case not_change:
	case not_pressed:
	  *selected_state = not_off;
	  current_values.update_pwm = 1;
	  current_values.fill = 1;
	  if (current_values.option % 4 == 2)
	    AN_send_uart ();
	  if (current_values.fft_encoder_mode != 0)
	    pwm_set = current_values;
	  break;
	default:
	}
    default:
      break;
    }
}

/*
 * @brief: Draws arrow to point in the menu
 * @inputs:
 *   @pos: position of the arrow (0-3)
 *   @filled: determines if the arrow will be filled or not (0-1)
 */
void
AN_draw_menu (uint8_t pos, uint8_t filled, uint8_t regen, char *str_element_0,
	      char *str_element_1)
{
  pos = pos % 4;
  filled = filled % 2;
  SSD1306_DrawFilledRectangle (
      0, 0, triangle_config.padding_x + triangle_config.offset_x - 1,
      SSD1306_HEIGHT,
      SSD1306_COLOR_BLACK);
  if (filled == 1)
    {
      SSD1306_DrawFilledTriangle (
	  triangle_config.offset_x,
	  triangle_config.padding_y + 1 + pos * triangle_config.size_rec,
	  triangle_config.offset_x,
	  triangle_config.padding_y + 1 + triangle_config.width_arrow
	      + pos * triangle_config.size_rec,
	  triangle_config.padding_x + triangle_config.offset_x - 1,
	  triangle_config.padding_y + 1 + triangle_config.heigth_arrow
	      + pos * triangle_config.size_rec,
	  SSD1306_COLOR_WHITE);
    }
  else
    {
      SSD1306_DrawTriangle (
	  triangle_config.offset_x,
	  triangle_config.padding_y + 1 + pos * triangle_config.size_rec,
	  triangle_config.offset_x,
	  triangle_config.padding_y + 1 + triangle_config.width_arrow
	      + pos * triangle_config.size_rec,
	  triangle_config.padding_x + triangle_config.offset_x - 1,
	  triangle_config.padding_y + 1 + triangle_config.heigth_arrow
	      + pos * triangle_config.size_rec,
	  SSD1306_COLOR_WHITE);
    }

  if (regen == 1)
    {
      SSD1306_Fill (SSD1306_COLOR_BLACK);
      for (uint8_t i = 0; i < 4; i++)
	{
	  SSD1306_DrawRectangle (
	      triangle_config.padding_x + triangle_config.offset_x,
	      triangle_config.padding_y + i * triangle_config.size_rec,
	      triangle_config.length_rec, triangle_config.heigth_rec,
	      SSD1306_COLOR_WHITE);
	  SSD1306_GotoXY (
	      triangle_config.padding_x + triangle_config.offset_x + 2,
	      triangle_config.padding_y + i * triangle_config.size_rec + 2);
	  SSD1306_Puts (strings[i], &Font_7x10, SSD1306_COLOR_WHITE);
	  switch (i)
	    {
	    case 0:
	      SSD1306_DrawRectangle (
		  triangle_config.padding_x + triangle_config.offset_x,
		  triangle_config.padding_y + i * triangle_config.size_rec,
		  triangle_config.length_rec, triangle_config.heigth_rec,
		  SSD1306_COLOR_WHITE);
	      SSD1306_GotoXY (
		  triangle_config.padding_x + triangle_config.offset_x + 2,
		  triangle_config.padding_y + i * triangle_config.size_rec + 2);
	      SSD1306_Puts (strings[i], &Font_7x10, SSD1306_COLOR_WHITE);
	      SSD1306_GotoXY (
		  triangle_config.padding_x + triangle_config.offset_x + 2
		      + 6 * 7 + 3,
		  triangle_config.padding_y + i * triangle_config.size_rec + 2);
	      SSD1306_Puts (str_element_0, &Font_7x10, SSD1306_COLOR_WHITE);
	      break;
	    case 1:
	      SSD1306_DrawRectangle (
		  triangle_config.padding_x + triangle_config.offset_x,
		  triangle_config.padding_y + i * triangle_config.size_rec,
		  triangle_config.length_rec, triangle_config.heigth_rec,
		  SSD1306_COLOR_WHITE);
	      SSD1306_GotoXY (
		  triangle_config.padding_x + triangle_config.offset_x + 2,
		  triangle_config.padding_y + i * triangle_config.size_rec + 2);
	      SSD1306_Puts (strings[i], &Font_7x10, SSD1306_COLOR_WHITE);
	      SSD1306_GotoXY (
		  triangle_config.padding_x + triangle_config.offset_x + 2
		      + 6 * 7 + 3,
		  triangle_config.padding_y + i * triangle_config.size_rec + 2);
	      SSD1306_Puts (str_element_1, &Font_7x10, SSD1306_COLOR_WHITE);
	      break;
	    case 3:
	      SSD1306_DrawRectangle (
		  triangle_config.padding_x + triangle_config.offset_x,
		  triangle_config.padding_y + i * triangle_config.size_rec,
		  triangle_config.length_rec, triangle_config.heigth_rec,
		  SSD1306_COLOR_WHITE);
	      SSD1306_GotoXY (
		  triangle_config.padding_x + triangle_config.offset_x + 2,
		  triangle_config.padding_y + i * triangle_config.size_rec + 2);
	      switch (current_values.fft_encoder_mode)
		{
		case 0:
		  SSD1306_Puts (strings[3], &Font_7x10, SSD1306_COLOR_WHITE);
		  break;
		case 1:
		  SSD1306_Puts (cursor_selec, &Font_7x10, SSD1306_COLOR_WHITE);
		  break;
		case 2:
		  SSD1306_Puts (cursor_selec_amp, &Font_7x10,
				SSD1306_COLOR_WHITE);
		  break;
		}
	      break;
	    default:
	    }
	}
      return;
    }
  SSD1306_GotoXY (
      triangle_config.padding_x + triangle_config.offset_x + 2 + 6 * 7 + 3,
      triangle_config.padding_y + 2);
  SSD1306_Puts (str_element_0, &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY (
      triangle_config.padding_x + triangle_config.offset_x + 2 + 6 * 7 + 3,
      triangle_config.padding_y + triangle_config.size_rec + 2);
  SSD1306_Puts (str_element_1, &Font_7x10, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY (triangle_config.padding_x + triangle_config.offset_x + 2,
		  triangle_config.padding_y + 3 * triangle_config.size_rec + 2);
 switch(current_values.fft_encoder_mode)
  {
    case 0:
    SSD1306_Puts (strings[3], &Font_7x10, SSD1306_COLOR_WHITE);
    break;
    case 1:
    SSD1306_Puts (cursor_selec, &Font_7x10, SSD1306_COLOR_WHITE);
    break;
    case 2:
    SSD1306_Puts (cursor_selec_amp, &Font_7x10, SSD1306_COLOR_WHITE);
    break;
  }
}

/*
 * @brief: Draws an arrow to indicate cursor position
 * @inputs:
 *   @cursor: Position to display arrow in x axis
 *   @color: Color to display the arrow
 */
void
AN_set_cursor (uint16_t cursor, SSD1306_COLOR_t color)
{
uint16_t posx;
if (cursor >= SSD1306_WIDTH)
posx = SSD1306_WIDTH - 2;
else if (cursor == 0)
posx = 1;
else
posx = cursor;

SSD1306_DrawLine (posx, 0, posx, CURSOR_HEIGHT - 1, SSD1306_COLOR_WHITE);
SSD1306_DrawPixel (posx - 1, CURSOR_HEIGHT - 2, SSD1306_COLOR_WHITE);
SSD1306_DrawPixel (posx + 1, CURSOR_HEIGHT - 2, SSD1306_COLOR_WHITE);

for (uint8_t posy = CURSOR_HEIGHT; posy < SSD1306_HEIGHT; posy = posy + 2)
{
  SSD1306_DrawPixel (posx, posy, SSD1306_COLOR_WHITE);
}
}

/*
 * @brief: gives the value of the pixel to be displayed based on the fft value
 * @inputs:
 *   @value: fft value to be displayed on screen
 *   @fft_len: points of the fft used
 */
uint8_t
AN_pixel (complex_t value, uint16_t fft_len)
{
uint8_t pixely = (uint8_t) ((FFT_mod (value, fft_len) * DIS_HEIGHT) / MAX_V);
if (pixely > DIS_HEIGHT)
pixely = DIS_HEIGHT;
else if (pixely < 0)
pixely = 0;
return pixely;
}

/*
 * @brief: saves the screen points to be displayed on screen for the fft
 * @inputs:
 *   @adc: array with the readings of the adc
 *   @adc_len: length of the adc array
 * @return: saves the values in the adc array
 */
void
AN_fft_fast (uint16_t *adc, uint16_t adc_len)
{
float fft_temp;
if (adc_len > FFT_MAX)
{
  return;
}
for (uint16_t i = 0; i < adc_len; i++)
{
  fft_screen_points[i].real = (float) adc[i] * 3.3 / ADC_SCALE;
  fft_screen_points[i].imag = 0;
}
FFT (fft_screen_points, adc_len);
fft_temp = FFT_mod (fft_screen_points[0], adc_len) / 2;
fft_screen_points[0].imag = (float) AN_pixel (fft_screen_points[0], adc_len)
  / 2;
fft_screen_points[0].real = fft_temp;
for (uint16_t i = 1; i < adc_len / 2; i++)
{
  fft_temp = FFT_mod (fft_screen_points[i], adc_len);
  fft_screen_points[i].imag = (float) AN_pixel (fft_screen_points[i], adc_len);
  fft_screen_points[i].real = fft_temp;
}
}

/*
 * @brief: makes a senoidal signal as an adc for testing purposes
 * @inputs:
 *   @freq: frequency of the signal
 *   @sample_fs: sampling frequency
 *   @max: max value that the adc will return
 *   @adc_arr: array that will hold the points
 *   @adc_len: length of the vector
 */
void
AN_sim_adc (float freq, float sample_fs, uint16_t max, uint16_t *adc_arr,
	    uint16_t adc_len)
{
if (adc_len <= 0 || max == 0 || freq <= 0)
return;

for (uint16_t i = 0; i < adc_len; i++)
{
  adc_arr[i] = (uint16_t) (max / 2
      * (sinf (2 * M_PI * freq * i / sample_fs) + 1));
}
}

uint16_t
AN_wrap_value (int32_t option, int32_t position, int32_t points)
{
return (uint16_t) ((position - option) % points + points) % points;
}
