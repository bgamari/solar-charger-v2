#include <stddef.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/cortex.h>
#include "timeout.h"

static struct timeout_time cur_time = {.raw = 0}; // lazily updated, in milliseconds
static uint32_t prescaler;

struct timeout_ctx *head;

void timeout_init(void)
{
  timer_reset(TIM5);
  nvic_enable_irq(NVIC_TIM5_IRQ);

  // timer tick is 1 millisecond
  prescaler = rcc_ppre1_frequency / 1000;
  timer_set_prescaler(TIM5, prescaler);
  timer_set_period(TIM5, 0xffff);
  timer_enable_irq(TIM5, TIM_DIER_UIE);
  timer_enable_counter(TIM5);
}

static void timeout_update_time(void)
{
  uint32_t count = timer_get_counter(TIM5);
  cur_time.count = count;
}

static void timeout_reschedule(void)
{
  if (head == NULL) {
    timer_disable_irq(TIM5, TIM_DIER_CC1IE);
    timer_disable_counter(TIM5);
  } else {
    timer_set_oc_value(TIM5, TIM_OC1, head->time.count);
    timer_enable_irq(TIM5, TIM_DIER_CC1IE);
  }
}

void timeout_add(struct timeout_ctx *ctx, unsigned int millis,
                 timeout_cb cb, void *cbdata)
{
  timeout_update_time();
  ctx->time.raw = cur_time.raw + millis;
  ctx->cb = cb;
  ctx->cbdata = cbdata;

  cm_disable_interrupts();
  struct timeout_ctx **last = &head;
  while (true) {
    if (*last == NULL)
      break;
    if ((*last)->time.raw > ctx->time.raw)
      break;
    last = &(*last)->next;
  }
  ctx->next = *last;
  *last = ctx;
  cm_enable_interrupts();

  if (last == &head)
    timeout_reschedule();
}

void tim5_irq(void)
{
  timeout_update_time();
  if (timer_get_flag(TIM5, TIM_SR_UIF)) {
    timer_clear_flag(TIM5, TIM_SR_UIF);
    cur_time.epoch++;
  }

  if (timer_get_flag(TIM5, TIM_SR_CC1IF)) {
    timer_clear_flag(TIM5, TIM_SR_CC1IF);
    if (!head) {
      timer_disable_irq(TIM5, TIM_DIER_CC1IE);
    } else {
      struct timeout_ctx *p = head;
      while (p != NULL) {
        if (p->time.raw <= cur_time.raw) {
          head = p->next;
          p->cb(p->cbdata);
          timeout_update_time();
        } else {
          break;
        }
        p = p->next;
      }
      timeout_reschedule();
    }
  }
}
