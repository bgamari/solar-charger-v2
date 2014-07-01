#include "leds.h"
#include <libopencm3/stm32/gpio.h>

void leds_init(void)
{
  gpio_mode_setup(GPIOH, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO0 | GPIO1);
}
  
void leds_set(uint8_t value)
{
  gpio_port_write(GPIOC, (value & 0x7) << 13);
}
