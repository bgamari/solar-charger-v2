#pragma once

typedef void (*timeout_cb)(void *cbdata);

struct timeout_time {
  union {
    uint32_t raw;
    struct {
      uint16_t count;
      uint16_t epoch;
    };
  };
};

struct timeout_ctx {
  struct timeout_ctx *next;
  struct timeout_time time;
  timeout_cb cb;
  void *cbdata;
};

void timeout_init(void);

void timeout_add(struct timeout_ctx *ctx, unsigned int millis,
                 timeout_cb cb, void *cbdata);

// Is the given timeout currently scheduled?
bool timeout_is_scheduled(struct timeout_ctx *ctx);

void timeout_poll(void);
