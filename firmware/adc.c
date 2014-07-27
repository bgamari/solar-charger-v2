#include "adc.h"
#include "util.h"
#include <stddef.h>
#include <libopencm3/stm32/adc.h>

static uint16_t *buffer = NULL;
static unsigned int remaining = 0;

void adc_take_sample(unsigned int length, uint8_t *sequence, uint16_t *buf)
{
  buffer = buf;
  remaining = 0;
  adc_set_regular_sequence(ADC1, length, sequence);

  // turn on ADC
  while (ADC_SR(ADC1) & ADC_SR_ADONS);
  adc_power_on(ADC1);

  // start conversion
  while ((ADC_SR(ADC1) & ADC_SR_ADONS) == 0
         || (ADC_SR(ADC1) & ADC_SR_RCNR) != 0);
  adc_start_conversion_regular(ADC1);

  WAIT_FOR(remaining == 0);

  // turn off ADC
  while (!(ADC_SR(ADC1) & ADC_SR_ADONS));
  adc_off(ADC1);
}

void adc1_isr(void)
{
  *buffer = adc_read_regular(ADC1);
  buffer++;
  remaining--;
}
