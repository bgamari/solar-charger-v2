#pragma once

void charge_init(void);

void charge_start(void);
void charge_stop(void);

enum charge_rate {
  TRICKLE, // trickle charge
  CHARGE,  // maximum power-point tracking
};

void charge_set_rate(enum charge_rate new_rate);
