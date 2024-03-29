
from i2c_miniptm import miniptm_i2c


from enum import Enum
#########
# General architecture of a module
# 1. it writes config directly to chipset

class gpiomode(Enum):
    INPUT = 0
    OUTPUT = 1
    FUNCTION = 2
    
    
class cm_gpios:
    def __init__(self, i2c_dev):
        self.i2c_dev = i2c_dev
        self.base_addrs = [0xc8c2, 0xc8d4, 0xc8e6, 0xc900, 0xc912,
                0xc924, 0xc936, 0xc948, 0xc95a, 0xc980, 0xc992, 
                0xc9a4, 0xc9b6, 0xc9c8, 0xc9da, 0xca00]
        self.valid_num = [i for i in range(0,16)]

    # mode can be lots of values in theory
    # coding for 1 = output, 0 = input
    def configure_pin(self, pin_num: int, mode: int, value: bool):
        if pin_num not in self.valid_num:
            return
        #print(f"Configure DPLL GPIO{pin_num} mode {mode} value {value}")
        # write gpio enable function and cmos 
        if ( mode == gpiomode.INPUT ):
            # 0x10 in programming guide v4.9, 0x11 in 5.3 , assume 4.9
            self.i2c_dev.write_dpll_reg(self.base_addrs[pin_num], 0x10, 0x0) # trigger register
        elif ( mode == gpiomode.OUTPUT ):            
            # 0x2 in 5.3, 0x0 in v4.9, assume 4.9
            if ( pin_num >= 8 ):
                read_val = self.i2c_dev.read_dpll_reg(0xc161, 0x0)
                if ( value ):                
                    read_val |= (1 << pin_num-8)
                else:
                    read_val &= ~(1 << pin_num-8)
                self.i2c_dev.write_dpll_reg(0xc161, 0x0, read_val)
                self.i2c_dev.write_dpll_reg(0xc161, 0x0, self.i2c_dev.read_dpll_reg(0xc161, 0x0) ) #trigger register
                self.i2c_dev.write_dpll_reg(self.base_addrs[pin_num], 0x10, 0x4) # GPIO trigger register and set to output
            else:
                read_val = self.i2c_dev.read_dpll_reg(0xc160, 0x0)
                if ( value ):                
                    read_val |= (1 << pin_num)
                else:
                    read_val &= ~(1 << pin_num)
                self.i2c_dev.write_dpll_reg(0xc160, 0x0, read_val)
                self.i2c_dev.write_dpll_reg(0xc160, 0x1, self.i2c_dev.read_dpll_reg(0xc160, 0x1) ) #trigger register
                self.i2c_dev.write_dpll_reg(self.base_addrs[pin_num], 0x10, 0x4) # GPIO trigger register and set to output

    # returns [mode, value]
    def read_pin_mode(self, pin_num: int) -> [int, int]:
        if pin_num not in self.valid_num:
            return 
        mode = self.i2c_dev.read_dpll_reg(self.base_addrs[pin_num], 0x10)
        if ( mode & 0x4 ) and ~( mode & 0x1 ):
            mode = gpiomode.OUTPUT
        elif ~(mode & 0x4) and ~(mode & 0x1):
            mode = gpiomode.INPUT
        elif (mode & 0x1):
            mode = gpiomode.FUNCTION
            
        val = self.i2c_dev.read_dpll_reg(0xc03c, 0x8a) # GPIO level 
        val = (val >> pin_num) & 0x1
        return [ mode, val ] 
        
    def print_status(self, pin_num: int):
        if pin_num not in self.valid_num:
            return
        [mode,val] = self.read_pin_mode(pin_num)
        mode_str = ""
        if ( mode == gpiomode.INPUT ):
            mode_str = "Input"
        elif ( mode == gpiomode.OUTPUT):
            mode_str = "Output"
        elif ( mode == gpiomode.FUNCTION):
            mode_str = "Function"
        else:
            mode_str = "UNKNOWN"
        print(f"GPIO{pin_num} mode={mode_str} value={val}")
        
    def __str__(self):
        # read back all the configs
        for i in self.valid_num:
            self.print_status(i)
            
        
        
 
