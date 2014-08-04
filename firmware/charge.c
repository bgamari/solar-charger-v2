#include <stddef.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>

#include "clock.h"
#include "usart.h"
#include "charge.h"
#include "adc.h"
#include "timeout.h"
#include "thermistor.h"

// disable battery temperature logic
#define DISABLE_BATTERY_TEMP

// enable debug output to serial console
#define DEBUG

#ifdef DEBUG
#define LOG(fmt, ...) usart_printf(fmt, __VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

#define NOT_USED(x) ( (void)(x) )

const uint32_t cell_v = 1400; // charged cell voltage in millivolts
const uint32_t n_cells = 6;
const uint32_t n_samples =  10; // number of ADC samples to average over
const uint32_t current_sense_r = 25; // current sense resistor in Siemens
const uint32_t current_sense_gain = 200; // gain of current sense amplifier
const uint32_t voltage_sense_gain = 1000 * (68+22) / 22; // millivolts / volt
const uint32_t trickle_current = 10; // milliamps
//const uint32_t charge_retry_time = 60 * 60 * 6; // how often to try full charge rate in seconds
const uint32_t charge_retry_time = 3; // how often to try full charge rate in seconds
const uint32_t iteration_time = 500; // update charge feedback in milliseconds
const uint32_t power_thresh = 50; // low charge power threshold (mW)

static bool charging = false;
static enum charge_rate rate = CHARGE;
static int charge_offset = 0;
static uint32_t last_power = 0; // for MPPT
static uint32_t perturbation = 10; // codepoints

static struct timeout_ctx retry_timeout, iteration_timeout;

static void set_charge_en(void) { gpio_set(GPIOB, GPIO15); }
static void clear_charge_en(void) { gpio_clear(GPIOB, GPIO15); }

static void update_charge_offset(void)
{
  if (charge_offset > 0x0fff)
    charge_offset = 0x0fff;
  else if (charge_offset < 0)
    charge_offset = 0;
  dac_load_data_buffer_single(charge_offset, RIGHT12, CHANNEL_1);
  LOG("o=%d\n", charge_offset);
}

void charge_init(void)
{
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO15); // CHARGE_EN
  clear_charge_en();

  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0);  // BAT_I
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);  // BAT_V
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4);  // CHARGE_OFFSET
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO5);  // IN_SENSE
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);  // BAT_TEMP_EN
  gpio_mode_setup(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO2);  // BAT_TEMP

  charge_stop();
}

static void set_battery_temp_enabled(bool enable)
{
  if (enable)
    gpio_clear(GPIOB, GPIO1);
  else
    gpio_set(GPIOB, GPIO1);
}

// returns whether to terminate the charging process
static bool charge_update(void)
{
  uint8_t sequence[3] = {0, 1, 9};
  uint16_t sample[3];
  set_battery_temp_enabled(true);
  adc_take_samples(n_samples, 3, sequence, sample);
  set_battery_temp_enabled(false);

  // battery charge current in microamps
  uint32_t bat_i = 3300 * sample[0] / 0x0fff * 1000 * current_sense_r / current_sense_gain;
  LOG("pv_i=%d mA\n", bat_i / 1000);

  // battery voltage in millivolts
  uint32_t bat_v = 3300 * sample[1] / 0x0fff * voltage_sense_gain / 1000;
  LOG("bat_v=%d mV\n", bat_v);

  // check battery voltage termination condition
  if (bat_v > cell_v * n_cells)
    return true;

#ifndef DISABLE_BATTERY_TEMP
  // temperature in centi-Kelvin
  unsigned int bat_temp = lookup_temperature(sample[2]);
  if (bat_temp < 372000 - 20000)
    return true; // sanity check
  if (bat_temp > 372000 + 60000)
    return true;
#endif

#if 1
  uint32_t power = bat_i * bat_v / 1000 / 1000; // in milliwatts
  LOG("power=%d mW\n", power);
  if (rate == CHARGE && power < power_thresh) {
    // The output voltage is too low so we aren't charging
    charge_offset -= perturbation;
  } else if (rate == CHARGE) {
    // Perturb and observe maximum power-point tracking
    LOG("mode=charge %d\n", perturbation);
    if (power < last_power)
      perturbation *= -1;
    charge_offset += perturbation;
    last_power = power;

  } else if (rate == TRICKLE) {
    LOG("mode=trickle setpoint=%d\n", trickle_current);
    // Track trickle current
    if (bat_i > trickle_current)
      charge_offset++;
    else
      charge_offset--;
  }
#else
  charge_offset = (charge_offset + 100) % 0x0fff;
#endif
  update_charge_offset();
  return false;
}

static void retry_charge(void *unused)
{
  NOT_USED(unused);
  charge_start(CHARGE);
}

static void charge_iteration(void *unused)
{
  NOT_USED(unused);
  usart_print("iterate\n");
  if (charge_update()) {
    charge_start(TRICKLE);
    if (!timeout_is_scheduled(&retry_timeout))
      timeout_add(&retry_timeout, charge_retry_time * 1000, retry_charge, NULL);
  }
  timeout_add(&iteration_timeout, iteration_time, charge_iteration, NULL);
}

// Start charging at the given rate
void charge_start(enum charge_rate new_rate)
{
  if (rate != new_rate || !charging) {
    last_power = 0;
    rate = new_rate;
  }

  if (!charging) {
    charging = true;
    usart_print("charge start\n");

    rcc_periph_clock_enable(RCC_DAC);
    rcc_osc_on(HSI);
    rcc_wait_for_osc_ready(HSI);
    rcc_periph_clock_enable(RCC_ADC1);

    dac_enable(CHANNEL_1);
    delay_ms(1); // wait for DAC and ADC to come up
    charge_offset = 1000; // this seems to be a reasonable initial value
    update_charge_offset();
    set_charge_en();
    timeout_add(&iteration_timeout, iteration_time, charge_iteration, NULL);
  }
}

void charge_stop(void)
{
  if (!charging)
    return;
  usart_print("charge stop\n");
  charging = false;
  clear_charge_en();
  delay_ms(1); // let things stabilize
  dac_disable(CHANNEL_1);
  rcc_periph_clock_disable(RCC_DAC);
  rcc_periph_clock_disable(RCC_ADC1);
}
