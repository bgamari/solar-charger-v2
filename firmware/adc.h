#pragma once

#include <stdint.h>

void adc_take_samples(unsigned int samples,
                      unsigned int length, uint8_t *sequence, uint16_t *buf);
