

#ifndef SITIME_H
#define SITIME_H

#include <Arduino.h>
#include "SoftWire_stm.h"


# define RNG_6_25_PPM 0
# define RNG_10_PPM 1
# define RNG_12_5_PPM 2
# define RNG_25_PPM 3
# define RNG_50_PPM 4
# define RNG_80_PPM 5
# define RNG_100_PPM 6
# define RNG_125_PPM 7
# define RNG_150_PPM 8
# define RNG_200_PPM 9
# define RNG_400_PPM 10
# define RNG_600_PPM 11
# define RNG_800_PPM 12
# define RNG_1200_PPM 13
# define RNG_1600_PPM 14
# define RNG_3200_PPM 15


#define DCTCXO_ADDR 0x60 // arduino uses 7bit address 
#define DCTCXO_SCL 35
#define DCTCXO_SDA 21

extern float frequency_offset; // frequency offset accumulator, in hertz

void init_sitime(float center_freq);

void apply_freq_change(float del_freq_hz);

#endif