#pragma once

enum charge_rate {
  TRICKLE,     // trickle charge
  CHARGE,      // charge with maximum power-point traccking
  MANUAL,      // manual charge point control
};

enum mppt_method {
  MPPT_PO,   // perturb-and-observe maximum power-point tracking (MPPT)
  MPPT_FOCV, // fractional open-circuit voltage (FOCV) MPPT
};

// Cached sensor values (read-only)
extern uint32_t bat_i; // battery charge current (uA)
extern uint32_t bat_v; // battery voltage (mV)
extern uint32_t bat_temp; // battery temperature (centi-Kelvin)
extern uint32_t input_v; // charger input voltage (mV)

void charge_init(void);

void charge_start(enum charge_rate rate);
void charge_stop(void);

void set_charge_offset(uint16_t o);
