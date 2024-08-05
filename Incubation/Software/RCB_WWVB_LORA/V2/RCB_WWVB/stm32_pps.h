#ifndef STM32_PPS_H
#define STM32_PPS_H


#include "WWVB_Arduino.h"
#include "stm32_hrtimer.h"


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



// Virtual clock structure
typedef struct {
    uint64_t seconds; // Full seconds of the virtual clock
    double nanoseconds; // Nanoseconds part of the virtual clock
    double frequency_offset; // in ppm
    double phase_offset_ns; // in nanoseconds
} VirtualClock;

// PI Controller structure
typedef struct {
    double kp; // Proportional gain
    double ki; // Integral gain
    double integral; // Integral term
} PIController;

// Disciplining state structure
typedef struct {
    int phase_adjustments_remaining;
    bool phase_adjustment_stage;
} DiscipliningState;

#define PHASE_ADJUSTMENT_THRESHOLD 5 // Number of initial phase adjustments
#define PPS_HISTORY_COUNT 5

void init_stm_pps();


void loop_stm_pps(); // run periodically

/******* PPS Input functions ******/

bool has_new_pps_input_timestamp();
void get_pps_input_timestamp(uint64_t * sec_ts, uint32_t * ns_ts);

/******* PPS output functions ******/

void pps_freq_adjust(int64_t ns_freq_adj) ;
void pps_step(int64_t step_val_ns);

/******** PPS input discipline PPS output ********/

void enable_pps_in_discipline();
void disable_pps_in_discipline();


#endif