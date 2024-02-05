#ifndef WWVB_RF_H
#define WWVB_RF_H

#include "WWVB_Arduino.h"


void init_wwvb_adc();

// either 1 for direct or 0 for amplifier path
void set_wwvb_antenna(bool direct);

#endif
