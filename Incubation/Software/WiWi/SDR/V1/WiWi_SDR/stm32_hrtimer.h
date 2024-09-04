#ifndef STM32_HRTIMER_H
#define STM32_HRTIMER_H

#include "WWVB_Arduino.h"

/*****************
Some design notes:
1. Using channel A as ART
    It captures PPS input and captures on PPS output from channel C
    Channel A Capture 1 is External PPS Input, Capture 2 is PPS Output from other channel

***********/


void debug_print_hrtimer_common_registers();
void debug_print_hrtimer_registers(int TimerIdx);

void stm32_hrtimer_init();


#endif