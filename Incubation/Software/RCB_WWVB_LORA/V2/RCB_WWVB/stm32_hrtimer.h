#ifndef STM32_HRTIMER_H
#define STM32_HRTIMER_H

#include "WWVB_Arduino.h"


#define DMA_PPS_COUNT 7000

// Define the hardware timer frequency
#define HW_TIMER_FREQUENCY 480000000.0

extern uint64_t art_val_ts;
extern uint16_t capture_ts;
extern uint16_t dma_remaining_ts;
extern uint16_t typical_count_ts;
extern uint64_t millis_ts;
extern bool valid_ts;

typedef struct art_timer_val {
  uint64_t art_val_ts;
  uint16_t capture_ts;
  uint16_t dma_remaining_ts;
  uint16_t typical_count_ts;
  uint64_t millis_ts;
  bool valid_ts;
} art_timer_val;

extern art_timer_val last_pps_out_time;
extern art_timer_val last_pps_in_time;

/*****************
Some design notes:
1. Using channel A as ART
    It captures PPS input and captures on PPS output from channel C
    Channel A Capture 1 is External PPS Input, Capture 2 is PPS Output from other channel

***********/


void debug_print_hrtimer_common_registers();
void debug_print_hrtimer_registers(int TimerIdx);

void compute_timing_adjust_ns(double adjustment_ns, uint16_t *X, uint16_t *Y);
void compute_level_ticks(uint32_t art_ticks, uint16_t * dma_count, uint16_t * fine_count);

void set_next_level_duration(uint64_t art_ticks, bool high_level);

void get_current_art_time(uint64_t * art_ticks);
void get_art_time_change(uint64_t * new_art_ticks);

void get_art_timer_val_absolute(uint64_t * art_ticks, art_timer_val * val);

uint64_t get_last_rising_edge_timestamp();

bool get_out_level();
bool has_ppsout_timestamp();
bool has_ppsin_timestamp();
void clear_has_ppsout_timestamp();
void clear_has_ppsin_timestamp();

uint16_t get_art_dma_remaining();
uint64_t get_art_dma_complete_count();

void stm32_hrtimer_init();


#endif