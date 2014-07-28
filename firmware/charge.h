#pragma once

enum charge_rate {
  TRICKLE, // trickle charge
  CHARGE,  // maximum power-point tracking
};

void charge_init(void);

void charge_start(enum charge_rate rate);
void charge_stop(void);

