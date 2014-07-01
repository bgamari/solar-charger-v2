#pragma once

#include <stdint.h>

void adc_take_sample(unsigned int length, uint8_t *sequence, uint16_t *buf);
