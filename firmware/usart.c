#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include "usart.h"

on_line_recv_cb on_line_recv;

char rx_buf[255];
unsigned int rx_head = 0;

char tx_buf[255];
unsigned int tx_head = 0; // write in position
unsigned int tx_tail = 0; // read out position

static unsigned int waiting_tx_bytes(void)
{
  if (tx_head < tx_tail)
    return tx_head + sizeof(tx_buf) - tx_tail;
  else
    return tx_head - tx_tail;
}

int usart_putc(char c)
{
  unsigned int next = (tx_head + 1) % sizeof(tx_buf);
  if (waiting_tx_bytes() == sizeof(tx_buf))
    return 0;

  tx_buf[tx_head] = c;
  tx_head = next;
  usart_enable_tx_interrupt(USART1);
  return 1;
}

void usart_write(const char* c, unsigned int length)
{
  for (unsigned int i=0; i<length; i++)
    usart_putc(c[i]);
}

void usart_print(const char* c)
{
  for (const char* i = c; *i != 0; i++)
    usart_putc(*i);
}

unsigned int usart_readline(char* buffer, unsigned int length)
{
  unsigned int i;
  for (i=0; i < length; i++) {
    usart_wait_recv_ready(USART1);
    buffer[i] = usart_recv(USART1);
    if (buffer[i] == '\n')
      break;
  }
  buffer[i] = 0;
  return i;
}

void usart_init(void)
{
  gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_USART1EN);

  usart_enable(USART1);
  usart_set_databits(USART1, 8);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_mode(USART1, USART_MODE_TX_RX);
  usart_set_baudrate(USART1, 9600);
  usart_enable_rx_interrupt(USART1);
  nvic_enable_irq(NVIC_USART1_IRQ);
}

void usart1_isr(void)
{
  if (usart_get_flag(USART1, USART_SR_RXNE)) {
    char c = usart_recv(USART1);
    if (c == '\n') {
      rx_buf[rx_head] = 0;
      on_line_recv(rx_buf, rx_head);
      rx_head = 0;
    } else {
      rx_buf[rx_head] = c;
      rx_head++;
    }
  }

  if (usart_get_flag(USART1, USART_SR_TXE)) {
    if (waiting_tx_bytes() > 0) {
      usart_send(USART1, tx_buf[tx_tail]);
      tx_tail = (tx_tail + 1) % sizeof(tx_buf);
    } else {
      usart_disable_tx_interrupt(USART1);
    }
  }
}
