#include "adc.h"
#include "util.h"
#include <stddef.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/cm3/nvic.h>

static uint16_t *buffer = NULL;
static unsigned int remaining = 0;
static uint8_t *seq;

void adc_take_sample(unsigned int length, uint8_t *sequence, uint16_t *buf)
{
  buffer = buf;
  remaining = length;
  seq = sequence+1;
  adc_set_regular_sequence(ADC1, 1, sequence);
  adc_disable_scan_mode(ADC1);
  adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_48CYC);
  adc_set_single_conversion_mode(ADC1);
  ADC_CR2(ADC1) |= ADC_CR2_EOCS;
  adc_enable_eoc_interrupt(ADC1);
  nvic_enable_irq(NVIC_ADC1_IRQ);

  // turn on ADC
  while (ADC_SR(ADC1) & ADC_SR_ADONS);
  adc_power_on(ADC1);

  // start conversion
  while ((ADC_SR(ADC1) & ADC_SR_ADONS) == 0
         || (ADC_SR(ADC1) & ADC_SR_RCNR) != 0);
  adc_start_conversion_regular(ADC1);

  WAIT_FOR(remaining == 0);

  // turn off ADC
  nvic_disable_irq(NVIC_ADC1_IRQ);
  while (!(ADC_SR(ADC1) & ADC_SR_ADONS));
  adc_off(ADC1);
}

void adc1_isr(void)
{
  // also clears IRQ flag
  *buffer = adc_read_regular(ADC1);
  buffer++;
  remaining--;
  if (remaining > 0) {
    adc_set_regular_sequence(ADC1, 1, seq);
    seq++;
    adc_start_conversion_regular(ADC1);
  }
}
