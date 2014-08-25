#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>

#include "clock.h"
#include "usart.h"
#include "charge.h"
#include "leds.h"
#include "timeout.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/timer.h>

#define DEBUG

int buttons_init(void)
{
  gpio_mode_setup(GPIOH, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO0 | GPIO1);
  exti_select_source(EXTI1, GPIOH); // Button 1
  exti_select_source(EXTI0, GPIOH); // Button 2

  exti_set_trigger(EXTI0 | EXTI1, EXTI_TRIGGER_FALLING);
  exti_enable_request(EXTI0 | EXTI1);
  nvic_enable_irq(NVIC_EXTI0_IRQ);
  nvic_enable_irq(NVIC_EXTI1_IRQ);
  return 0;
}

static void on_line_recv(const char* c, unsigned int length)
{
  if (c[0] == 'm') {
    charge_start(MANUAL);
    usart_print("manual\n");
  } else if (c[0] == 't') {
    charge_start(TRICKLE);
    usart_print("trickle\n");
  } else if (c[0] == 'c') {
    charge_start(CHARGE);
    usart_print("charge\n");
  } else if (c[0] == '=') {
    const char* end = c+length+1;
    long offset = strtol(&c[1], (char** restrict) &end, 10);
    set_charge_offset(offset);
    usart_printf("set offset %d\n", offset);
  } else if (c[0] == 'v') {
    const char* end = c+length+1;
    long target = strtol(&c[1], (char** restrict) &end, 10);
    target_pv_v = target;
    usart_printf("target pv %d\n", target);
  } else if (c[0] == 'l') {
    logging_enabled = c[1] == '1';
  }
}

int main(void)
{
#ifndef DEBUG
  const clock_scale_t* clk = &clock_config[CLOCK_VRANGE1_MSI_RAW_2MHZ];
  rcc_clock_setup_msi(clk);
#endif

  //PWR_CR = (PWR_CR & ~(0x7 << 5)) | (0x6 << 5) | PWR_CR_PVDE; // PVD = 3.1V
  //exti_enable_request(EXTI16); // PVD interrupt
  init_systick();
  timeout_init();

  rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_GPIOAEN);
  rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_GPIOBEN);
  rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_GPIOCEN);
  rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_GPIOHEN);

  // LEDs
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13 | GPIO14 | GPIO15);
  leds_init();
  leds_set(1);
  usart_on_line_recv = on_line_recv;
  usart_init();
  usart_print("hello world\n");

  charge_init();
  leds_set(2);

  for (int i=2; i < 8; i++) {
    leds_set(i);
    delay_ms(40);
  }

  buttons_init();
  leds_set(0);
  debug = gpio_get(GPIOH, GPIO0);

  charge_start(CHARGE);

  while (1) {
    if (debug)
      delay_ms(40);
    else
      __asm__("wfi");
    timeout_poll();
  }
}

static void clear_leds(void *unused) {
  NOT_USED(unused);
  leds_set(0);
}

void button1_pressed(void)
{
  static struct timeout_ctx timeout;
  if (bat_i < 20)
    leds_set(0);
  else if (bat_i < 40)
    leds_set(1);
  else if (bat_i < 60)
    leds_set(3);
  else
    leds_set(7);
  if (!timeout_is_scheduled(&timeout))
    timeout_add(&timeout, 2000, clear_leds, NULL);
}

void button2_pressed(void)
{
  charge_start(CHARGE);
}

void pvd_isr(void)
{
  EXTI_PR |= EXTI16;
}

void exti0_isr(void)
{
  if (exti_get_flag_status(EXTI0))
    button2_pressed();
  EXTI_PR |= EXTI0;
}

void exti1_isr(void)
{
  if (exti_get_flag_status(EXTI1))
    button1_pressed();
  EXTI_PR |= EXTI1;
}
