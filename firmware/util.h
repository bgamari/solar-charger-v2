#pragma once

#include <stdbool.h>

extern bool debug;

#define WAIT_FOR(cond) \
  if (debug) {         \
    while (!(cond));   \
  } else {             \
    while (!(cond)) __asm__("wfi"); \
  }

#define NOT_USED(x) ( (void)(x) )
