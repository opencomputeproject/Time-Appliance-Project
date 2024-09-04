#ifndef WIWI_CONTROL_H
#define WIWI_CONTROL_H

#include "WWVB_Arduino.h"
#include "WiWi_Data.h"
#include "SiTime.h"
#include "WWVB_Arduino.h"

// Define proportional gain
extern float KP;
extern float KI;
extern float KD;
extern float ALPHA;

// Define clock characteristics
#define MIN_RESOLUTION (7800.0 / 33554431.0) 
// 200 ppm = 7800 Hertz at 39MHz, 2^25 - 1 = 33554431
// so 7800 / (2^25 - 1) = value of a single bit in Hertz ~= 0.2mHz = 0.0002 Hz


#define MAX_PULL_RANGE 7800
//#define MAX_INTEGRAL (0.2)
#define MAX_INTEGRAL 7800

// Define rate limits
extern float MAX_RATE_OF_CHANGE;

// Structure to hold the control state
typedef struct {
    float accumulatedPhaseError;
    float lastError;
    float filteredError;
    float currentFrequency;
    float integral;
} ControlState;


// top level call for control loop, pass it twophi_C
void wiwi_oscillator_feedback(float twophi_C);


#endif