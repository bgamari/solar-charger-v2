#include "adc.h"
#include <stddef.h>
#include <libopencm3/stm32/adc.h>

static uint16_t *buffer = NULL;
static unsigned int remaining = 0;

void adc_take_sample(unsigned int length, uint8_t *sequence, uint16_t *buf)
{
  buffer = buf;
  remaining = 0;
  adc_set_regular_sequence(ADC1, length, sequence);
  adc_start_conversion_regular(ADC1);
  while (remaining > 0)
    __asm__("wfi");
}

void adc1_isr(void)
{
  *buffer = adc_read_regular(ADC1);
  buffer++;
  remaining--;
}
