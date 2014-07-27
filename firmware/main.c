#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>

#include "clock.h"
#include "usart.h"
#include "charge.h"
#include "leds.h"
#include "timeout.h"

#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/timer.h>

int buttons_init(void)
{
  exti_select_source(EXTI1, GPIOH); // Button 1
  exti_select_source(EXTI0, GPIOH); // Button 2

  exti_enable_request(EXTI0 | EXTI1);
  exti_set_trigger(EXTI0 | EXTI1, EXTI_TRIGGER_FALLING);
  nvic_enable_irq(NVIC_EXTI0_IRQ);
  nvic_enable_irq(NVIC_EXTI1_IRQ);
  return 0;
}

int main(void)
{
  const clock_scale_t* clk = &clock_config[CLOCK_VRANGE1_MSI_RAW_2MHZ];
  rcc_clock_setup_msi(clk);

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
  for (int i=0; i <= 8; i++) {
    leds_set(i);
    delay_ms(100);
  }

  buttons_init();
  charge_init();

  while (1) {
    __asm__("wfi");
  }
}

void button1_pressed(void) { }
void button2_pressed(void) { }

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
