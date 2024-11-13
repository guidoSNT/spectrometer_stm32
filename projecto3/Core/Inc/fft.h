#ifndef INC_FFT_H_
#define INC_FFT_H_

#include "stdint.h"
#include "math.h"

/* Data struct to allocate complex numbers */
typedef struct
{
  float real;
  float imag;
} complex_t;

void
FFT (complex_t *Y, uint16_t N);
float
FFT_mod (complex_t value, uint16_t N);

#endif
