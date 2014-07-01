#include <stdint.h>

extern volatile uint32_t msTicks;      /* counts 1ms timeTicks */

void init_systick(void);
void delay_ms(unsigned int ms);

// Using 16MHz HSI clock
#define CLOCKRATE 16000000


