#pragma once

enum charge_rate {
  TRICKLE, // trickle charge
  CHARGE,  // maximum power-point tracking
};

// Cached sensor values (read-only)
extern uint32_t bat_i; // battery charge current (uA)
extern uint32_t bat_v; // battery voltage (mV)
extern uint32_t bat_temp; // battery temperature (centi-Kelvin)
extern uint32_t input_v; // charger input voltage (mV)

void charge_init(void);

void charge_start(enum charge_rate rate);
void charge_stop(void);

