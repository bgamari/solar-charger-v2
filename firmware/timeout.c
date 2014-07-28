#include <stddef.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/cortex.h>
#include "timeout.h"

static struct timeout_time cur_time = {.raw = 0}; // lazily updated, in milliseconds
static uint32_t prescaler;

// Waiting for their time to be executed
struct timeout_ctx *scheduled = NULL;
// Waiting to execute
struct timeout_ctx *pending = NULL;

void timeout_init(void)
{
  rcc_periph_clock_enable(RCC_TIM4);
  timer_reset(TIM4);
  nvic_enable_irq(NVIC_TIM4_IRQ);

  // timer tick is 1 millisecond
  prescaler = rcc_ppre1_frequency / 1000;
  timer_set_prescaler(TIM4, prescaler);
  timer_set_period(TIM4, 0xffff);
  timer_enable_irq(TIM4, TIM_DIER_UIE);
  timer_enable_counter(TIM4);
}

static void timeout_update_time(void)
{
  uint32_t count = timer_get_counter(TIM4);
  cur_time.count = count;
}

static void timeout_reschedule(void)
{
  if (scheduled == NULL) {
    timer_disable_irq(TIM4, TIM_DIER_CC1IE);
    timer_disable_counter(TIM4);
  } else {
    timer_set_oc_value(TIM4, TIM_OC1, scheduled->time.count);
    timer_enable_counter(TIM4);
    timer_enable_irq(TIM4, TIM_DIER_CC1IE);
  }
}

void timeout_add(struct timeout_ctx *ctx, unsigned int millis,
                 timeout_cb cb, void *cbdata)
{
  timeout_update_time();
  // make sure it's not already scheduled
  if (timeout_is_scheduled(ctx))
    while(1);
  ctx->time.raw = cur_time.raw + millis;
  ctx->cb = cb;
  ctx->cbdata = cbdata;

  // Add timeout to scheduled list
  cm_disable_interrupts();
  struct timeout_ctx **last = &scheduled;
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

  // If we added to the head of the list we need to reschedule
  if (last == &scheduled)
    timeout_reschedule();
}

void tim4_isr(void)
{
  struct timeout_ctx **pending_tail = NULL;
  timeout_update_time();
  if (timer_get_flag(TIM4, TIM_SR_UIF)) {
    timer_clear_flag(TIM4, TIM_SR_UIF);
    cur_time.epoch++;
  }

  if (timer_get_flag(TIM4, TIM_SR_CC1IF)) {
    timer_clear_flag(TIM4, TIM_SR_CC1IF);
    if (!scheduled) {
      timer_disable_irq(TIM4, TIM_DIER_CC1IE);
    } else {
      struct timeout_ctx *p = scheduled;
      while (p != NULL) {
        if (p->time.raw <= cur_time.raw) {
          scheduled = p->next;

          // find the tail of the pending list if we haven't yet done
          // so
          if (pending_tail == NULL) {
            pending_tail = &pending;
            while (true) {
              if (*pending_tail == NULL)
                break;
              pending_tail = &(*pending_tail)->next;
            }
          }

          // add to end of pending list
          p->next = NULL;
          pending = p;
          pending_tail = &pending;
        } else {
          break;
        }
        p = p->next;
      }
      timeout_update_time();
      timeout_reschedule();
    }
  }
}

// This is where the timeout callbacks are called, out of IRQ context
void timeout_poll()
{
  while (pending != NULL) {
    cm_disable_interrupts();
    struct timeout_ctx *p = pending;
    timeout_cb cb = p->cb;
    pending = p->next;
    cm_enable_interrupts();

    p->cb = NULL; // so we know whether it's scheduled
    p->next = NULL;
    cb(p->cbdata);
  }
}

bool timeout_is_scheduled(struct timeout_ctx *ctx)
{
  return ctx->cb != NULL;
}
