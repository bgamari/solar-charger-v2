#pragma once

#define DEBUG

#ifdef DEBUG
#define WAIT_FOR(cond) \
  while (!(cond));
#else
#define WAIT_FOR(cond) \
  while (!(cond)) __asm__("wfi");
#endif

#define NOT_USED(x) ( (void)(x) )
