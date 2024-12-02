
#ifndef CLOCKMATRIX_H
#define CLOCKMATRIX_H

#include "SoftWire_stm.h"
#include "menu_cli.h"




extern SoftWire cm_i2c;

bool dpll_read_reg(uint16_t baseaddr, uint16_t offset, uint8_t * val);
bool dpll_write_reg(uint16_t baseaddr, uint16_t offset, uint8_t val);
bool dpll_write_eeprom(uint32_t addr, uint8_t * data, int count);
void init_clockmatrix();

#endif
