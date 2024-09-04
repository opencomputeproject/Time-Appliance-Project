#ifndef SI5341B_H
#define SI5341B_H

#include "Si5341-RevD-WIWISDRV1-Registers.h"
#include <stdint.h>
#include "SoftWire_stm.h"


/*
#define SI5341_RST PJ_3
#define SI5341_INTR PJ_4
#define SI5341_FINC PI_4
#define SI5341_FDEC PI_5
#define SI5341_LOLB PI_6
*/


//#define SI5341_I2C_ADDR 0x77 // 7 bit address

#define PAGE_REG 0x1

int si5341b_write_reg(uint8_t page, uint8_t reg, uint8_t val);
int si5341b_read_reg(uint8_t page, uint8_t reg, uint8_t * val);


bool init_si5341b();

#endif