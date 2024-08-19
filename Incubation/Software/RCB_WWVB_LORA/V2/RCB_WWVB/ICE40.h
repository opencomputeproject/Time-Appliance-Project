

#ifndef ICE40_H
#define ICE40_H


#include "WWVB_Arduino.h"
#include <Arduino.h>

#include "fpgaFirmware.h"
#include "SX1257.h"



void hold_ice40_reset();
void release_ice40_reset();

void ice40_stop_stream();
void ice40_start_stream();
void ice40_clear_stream_gate(); // may or may not be needed in fpga


bool prog_bitstream_start();
bool prog_bitstream_finish();
bool prog_bitstream(bool reset_only); // not used
bool prog_bitstream_send(unsigned char * buf, long size);


void ice40_test();



#endif