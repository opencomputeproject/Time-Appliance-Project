

#include "WiWi_control.h"


float KP = -0.000256163591985f;
//float KI = -0.0000015f;
float KI = -0.000000054657637f;
float KD = 0.000000000981473f;
float MAX_RATE_OF_CHANGE = (MIN_RESOLUTION * 4);
// Define low-pass filter parameters
float ALPHA = 0.3f;
ControlState controlState = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};




// Function to adjust the clock frequency
float adjustClockFrequency(float adjustment) {

    // Ensure the new frequency is within the limits
    if (adjustment >=  MAX_PULL_RANGE ) {
        adjustment = MAX_PULL_RANGE;
    } else if (adjustment <= (-1 * MAX_PULL_RANGE)) {
        adjustment = -1*MAX_PULL_RANGE;
    }

    apply_freq_change(adjustment);

    return adjustment;
}


// Function to apply a first-order low-pass filter
float lowPassFilter(float currentValue, float previousValue, float alpha) {
    return alpha * currentValue + (1.0f - alpha) * previousValue;
}

// Function to update the control state with a new phase error measurement
void updateControlState(ControlState *state, float newPhaseError, float dt) {
  // not included in this release
  
}

// Function to get the current clock frequency
float getCurrentFrequency(const ControlState *state) {
    return state->currentFrequency;
}

unsigned long last_osc_feedback = 0;
void wiwi_oscillator_feedback(float twophi_C) {  
  // Not included in this release

  
}