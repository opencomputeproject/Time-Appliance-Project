

#ifndef ICE40_H
#define ICE40_H


#include "WWVB_Arduino.h"
#include <Arduino.h>

#include "fpgaFirmware.h"



void hold_ice40_reset();
void release_ice40_reset();

bool prog_bitstream_start();
bool prog_bitstream_finish();
bool prog_bitstream(bool reset_only); // not used
bool prog_bitstream_send(unsigned char * buf, long size);



#endif