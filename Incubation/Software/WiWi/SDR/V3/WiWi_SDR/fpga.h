#ifndef FPGA_H
#define FPGA_H

#include "menu_cli.h"
#include <Arduino.h>
#include "WWVB_Arduino.h"
#include "my_qspi.h"
#include "SoftWire_stm.h"
#include "my_qspi.h"


/**** Register access *******/
bool fpga_i2c_read(uint8_t address, uint8_t * val);
bool fpga_i2c_write(uint8_t address, uint8_t val);

/**** Radio control *****/
void enable_subg_shift();
void disable_subg_shift();
void enable_wifi_shift();
void disable_wifi_shift();


/********* CLI boiler plate ********/
void init_fpga_cli();

#endif