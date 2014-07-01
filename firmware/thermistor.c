#include "thermistor.h"

struct lut_entry {
  uint16_t codepoint, temp;
};

static struct lut_entry lut[] = {
  // generated by Thermistor.hs
#include "thermistor-lut.c"
};

const unsigned int lut_size = sizeof(lut) / sizeof(struct lut_entry);

// map ADC codepoint to temperature in hundredths of a degree Kelvin
unsigned int lookup_temperature(uint16_t codepoint)
{
  unsigned int i = 0;
  for (; i < lut_size; i++) {
    if (lut[i].codepoint > codepoint)
      break;
  }
  
  if (i == 0)
    return 0;
  else
    return lut[i-1].temp + (codepoint - lut[i-1].codepoint) * (lut[i].temp - lut[i-1].temp) / (lut[i].codepoint - lut[i-1].codepoint);
}