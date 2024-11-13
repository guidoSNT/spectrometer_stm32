#ifndef INC_DISPLAY_ANALIZADOR_H_
#define INC_DISPLAY_ANALIZADOR_H_

#include "stdio.h"
#include "fft.h"
#include "ssd1306.h"
#include "fonts.h"
#include "encoder.h"
#include "stm32f1xx_hal.h"

#define MAX_V 3.3
#define ADC_SCALE 4096
#define FS 1000
#define FFT_MAX 256
#define CURSOR_HEIGHT 5
#define DIS_HEIGHT (SSD1306_HEIGHT - CURSOR_HEIGHT)
#define REC_WIDTH 116
#define REC_HEIGTH 12
#define REC_POS_X (SSD1306_WIDTH - REC_WIDTH - 1)
#define REC_POS_Y (CURSOR_HEIGHT + REC_HEIGTH)
#define WRAP_VALUE(option,position,points) (((position-option)%points+points)%points)
#define WRAP_FREQ(freq) (((freq - 4) % 497 + 497) % 497 + 4)

/* typedef for the inputs of the project */
typedef struct
{
  encoder_t encoder;
  button_t button_encoder;
  button_t button_sec;
} inputs_t;

typedef struct
{
  uint8_t padding_y;
  uint8_t padding_x;
  uint8_t offset_x;
  uint8_t size_rec;
  uint8_t heigth_rec;
  uint8_t width_arrow;
  uint8_t heigth_arrow;
  uint8_t length_rec;
} triangle_data_t;

typedef struct
{
  uint32_t freq;
  uint8_t amp;
} pwm_t;

typedef struct
{
  pwm_t pwm_state;
  uint16_t option;
  uint8_t fill;
  uint8_t update_pwm;
  uint8_t fft_pass;
  uint8_t fft_encoder_mode;
  uint8_t uart_send;
} config_disp_t;

typedef struct
{
  switch_t encoder;
  switch_t second;
} switch_values_t;

/* State machine for the config menu */
typedef enum
{
  sel_on, sel_off, not_off, not_on
} selected_state_t;
typedef enum
{
  pressed, not_pressed, pressed_change, not_change
} selected_event_t;

/* state machine for the main displays */
typedef enum
{
  off_update, off_stay, on_update, on_stay
} display_event_t;
typedef enum
{
  fft, config, config_pre_off, fft_pre_off
} display_state_t;

void
AN_init (UART_HandleTypeDef *uart_fm);

void
AN_send_uart ();

/* functions of the main displays machine state */
void
AN_displays_fsm (display_state_t *display_state, display_event_t display_ev,
		 switch_values_t switches);

/* functions related to displaying the fft */
void
AN_graph_fft (uint8_t cursor);

void
AN_set_cursor (uint16_t cursor, SSD1306_COLOR_t color);

uint8_t
AN_pixel (complex_t value, uint16_t fft_len);

void
AN_fft_fast (uint16_t *adc, uint16_t adc_len);

void
AN_sim_adc (float freq, float sample_fs, uint16_t max, uint16_t *adc_arr,
	    uint16_t adc_len);

/* functions related to reading the inputs */
void
AN_fft_update_setter ();

uint8_t
AN_botton_setter ();

void
AN_encoder_setter ();

inputs_t
AN_inputs_reader ();

config_disp_t
AN_config_reader ();

/* functions related to showing different screens */
void
AN_graph_control (switch_values_t switches, uint8_t regen);

void
AN_draw_arrow (uint8_t pos, uint8_t filled);

void
AN_draw_menu (uint8_t pos, uint8_t filled, uint8_t regen, char *str_element_0,
	      char *str_element_1);
void
AN_fsm_config (selected_state_t *selected_state,
	       selected_event_t selected_event, inputs_t inputs_cp);

uint16_t
AN_wrap_value (int32_t option, int32_t position, int32_t points);
#endif
