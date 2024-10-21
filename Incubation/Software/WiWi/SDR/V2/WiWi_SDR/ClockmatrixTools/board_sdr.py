
from i2c_clockmatrix import i2c_cm
from renesas_cm_configfiles import *

from renesas_cm_registers import *
# one class object for ALL MiniPTM boards installed in the system
import time
from renesas_cm_gpio import gpiomode

from enum import Enum



class Single_SDR:
    def __init__(self, board_num, devinfo, com_num):
        self.board_num = board_num
        self.i2c = i2c_cm(com_num)
        self.best_clock_quality_seen = 255 - board_num # hacky
        #print(f"Register SDR device serial bus {com_num}")

        self.dpll = DPLL(self.i2c, self.i2c.read_dpll_reg_direct,
                         self.i2c.read_dpll_reg_multiple_direct,
                         self.i2c.write_dpll_reg_direct,
                         self.i2c.write_dpll_multiple)



    def init_eeprom_addr(self, block=0):
        addr = 0x54
        if (block):
            addr = 0x55
        # check if cached address matches what I want to set
        if (hasattr(self, "eeprom_addr")):
            if self.eeprom_addr != addr:
                self.dpll.modules["EEPROM"].write_reg(0, "EEPROM_I2C_ADDR",
                    addr)
                self.eeprom_addr = addr
        else:
            self.dpll.modules["EEPROM"].write_reg(0, "EEPROM_I2C_ADDR",
                addr)
            self.eeprom_addr = addr

    def write_to_eeprom(self, offset, data):
    
        hex_values = [f"{value:02x}" for value in data]
        print(
            f"Write Board {self.board_num} EEPROM offset 0x{offset:x} data {hex_values}")

        self.i2c.dpll_write_eeprom(offset, data)
        return
    
        if (offset > 0xffff):
            self.init_eeprom_addr(1)
        else:
            self.init_eeprom_addr(0)

        hex_values = [f"{value:02x}" for value in data]
        print(
            f"Write Board {self.board_num} EEPROM offset 0x{offset:x} data {hex_values}")

        # write offset
        self.dpll.modules["EEPROM"].write_reg(0, "EEPROM_OFFSET_LOW", offset & 0xff)
        self.dpll.modules["EEPROM"].write_reg(0, "EEPROM_OFFSET_HIGH", (offset >> 8) & 0xff)
        


        # write byte count
        data_len = len(data)
        if (data_len > 128):
            data_len = 128
            data = data[:128]


        self.dpll.modules["EEPROM"].write_reg(0, "EEPROM_SIZE", data_len)


        # write the data to the data buffer
        for i in range(data_len):
            reg = f"BYTE_OTP_EEPROM_PWM_BUFF_{i}"
            self.dpll.modules["EEPROM_DATA"].write_reg(0, reg, data[i])

        # write the command
        self.dpll.modules["EEPROM"].write_reg(
            0, "EEPROM_CMD_LOW",  0x2)  # Write to EEPROM
        self.dpll.modules["EEPROM"].write_reg(
            0, "EEPROM_CMD_HIGH", 0xEE)  # Write to EEPROM

        # give it some time
        #time.sleep(0.0005 * data_len)

    def write_eeprom_file(self, eeprom_file="8A34002_MiniPTMV3_12-29-2023_Julian_AllPhaseMeas_EEPROM.hex"):
        hex_file_data, non_data_records_debug = parse_intel_hex_file(
            eeprom_file)
        self.eeprom_addr = 0  # make sure this gets reset
        for addr in hex_file_data.keys():
            # print(f"Write addr {addr:02x} = {hex_file_data[addr]}")
            self.write_to_eeprom(addr, hex_file_data[addr])

    def is_configured(self) -> bool:
        # check GPIO config registers for GPIOs used by DPLL for LEDs on MiniPTM
        # by default GPIOs are input, so if they're output assume config has been written previously

        # GPIO2 and GPIO3 are LEDs on MiniPTM board, programming guide doesnt match GUI, use GUI here
        gpio2 = self.i2c.read_dpll_reg(0xc8e6, 0x10)

        gpio3 = self.i2c.read_dpll_reg(0xc900, 0x10)

        # print(f"Check Miniptm adap {self.adap_num} is configured, 0x{gpio2:x} 0x{gpio3:x}")

        gpio2 &= 0x5
        gpio3 &= 0x5

        if (gpio2 == 0x4 and gpio3 == 0x4):
            return True
        return False


    def clear_all_dpll_sticky_status(self):
        # clear all stickies
        self.i2c.write_dpll_reg(0xc164, 0x5, 0x0)
        self.i2c.write_dpll_reg(0xc164, 0x5, 0x1)
