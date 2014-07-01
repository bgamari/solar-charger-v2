#include <stddef.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include "clock.h"
#include "charge.h"
#include "adc.h"
#include "timeout.h"
#include "thermistor.h"

const uint32_t cell_v = 1400; // charged cell voltage in millivolts
const uint32_t n_cells = 6;
const uint32_t current_sense_r = 1. /  0.1; // current sense resistor in Siemens
const uint32_t current_sense_gain = 200;
const uint32_t trickle_current = 10; // milliamps
const uint32_t charge_retry_time = 60 * 60 * 6; // how often to try full charge rate in seconds
const uint32_t iteration_time = 1; // update charge feedback in seconds

enum charge_rate {
  TRICKLE, // trickle charge
  CHARGE,  // maximum power-point tracking
};

static bool charging = false;
static enum charge_rate rate = CHARGE;
static uint16_t charge_offset;

static struct timeout_ctx retry_timeout, iteration_timeout;

static void set_charge_en(void) { gpio_set(GPIOB, GPIO15); }
static void clear_charge_en(void) { gpio_clear(GPIOB, GPIO15); }

void charge_init(void)
{
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO15); // CHARGE_EN
  clear_charge_en();

  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0);  // BAT_I
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);  // BAT_V
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2);  // VBAT_EN
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4);  // CHARGE_OFFSET
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO5);  // IN_SENSE
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);  // BAT_TEMP_EN
  gpio_mode_setup(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO2);  // BAT_TEMP

  rcc_periph_clock_enable(RCC_TIM5);
  charge_stop();
}

// returns temperature in centi-Kelvin
static unsigned int sample_battery_temp(void)
{
  uint16_t sample;
  uint8_t sequence[1] = {9};
  gpio_clear(GPIOB, GPIO1);
  adc_take_sample(1, sequence, &sample);
  gpio_set(GPIOB, GPIO1);
  return lookup_temperature(sample);
}

// returns whether to terminate the charging process
static bool charge_update(void)
{
  uint8_t sequence[2] = {0, 1};
  uint16_t sample[2];
  adc_take_sample(2, sequence, sample);
  uint32_t bat_i = 3300 * sample[0] / current_sense_gain / current_sense_r; // in milliamps
  uint32_t bat_v = 3300 * sample[1]; // in millivolts

  // check battery voltage termination condition
  if (bat_v > cell_v * n_cells)
    return true;

  uint32_t bat_temp = sample_battery_temp();
  if (bat_temp < 372000 - 20000)
    return true; // sanity check
  if (bat_temp > 372000 + 60000)
    return true;

  if (rate == CHARGE) {
    uint32_t power = bat_i * bat_v / 1000; // in milliwatts
    // TODO: MPPT
  } else if (rate == TRICKLE) {
    if (bat_i > trickle_current)
      charge_offset++;
    else
      charge_offset--;
  }
  return false;
}

static void retry_charge(void *unused)
{
  rate = CHARGE;
}

static void charge_iteration(void *unused)
{
  if (charge_update()) {
    rate = TRICKLE;
    timeout_add(&retry_timeout, charge_retry_time * 1000, retry_charge, NULL);
  } else {
    timeout_add(&iteration_timeout, iteration_time, charge_iteration, NULL);
  }
}

void charge_start(void)
{
  if (charging)
    return;
  charging = true;
  dac_enable(CHANNEL_1);
  adc_power_on(ADC1);
  delay_ms(1); // wait for DAC and ADC to come up
  charge_offset = (1 << 12) - 1;
  dac_load_data_buffer_single(CHANNEL_1, RIGHT12, charge_offset);
  adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_16CYC);
  adc_disable_scan_mode(ADC1);
  set_charge_en();
  timeout_add(&iteration_timeout, iteration_time, charge_iteration, NULL);
}

void charge_stop(void)
{
  if (!charging)
    return;
  charging = false;
  timer_disable_counter(TIM5);
  clear_charge_en();
  delay_ms(1); // let things stabilize
  dac_disable(CHANNEL_1);
  adc_off(ADC1);
}
