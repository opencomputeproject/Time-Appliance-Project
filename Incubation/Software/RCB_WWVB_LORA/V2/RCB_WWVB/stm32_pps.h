#ifndef STM32_PPS_H
#define STM32_PPS_H


#include "WWVB_Arduino.h"


/**************************
Theory of operation:

1. HRTIM is running at 480MHz , period of ~2.08nanoseconds, provides fine time control
  HRTIM is the "front-line" time, drives the PPS output / captures PPS input
2. TIM1 can run at 240MHz, but use it as coarse time control
  TIM1 output can trigger HRTIM
3. Prescale TIM1 to 100KHz, period 10uS, prescalar value = 2400


PPS output
  1. TIM1 runs continously
  2. When TIM1 rolls over, triggers HRTIM in single shot retriggerable mode
  3. Software adjusts TIM1 rollover for coarse time, HRTIM rollover for fine time

PPS input
  1. HRTIM and TIM1 run constantly
  2. On HRTIM_EEV trigger input, capture HRTIM value
  3. On HRTIM capture event, trigger DMA to trigger TIM1 capture (DMA abuse but should work) 

************************/

void init_stm_pps();


void loop_stm_pps(); // run periodically

/******* PPS Input functions ******/

/******* PPS output functions ******/

#endif