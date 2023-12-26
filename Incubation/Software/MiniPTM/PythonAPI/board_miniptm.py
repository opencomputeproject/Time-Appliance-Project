

from pcie_miniptm import MiniPTM_PCIe, get_miniptm_devices
from i2c_miniptm import find_i2c_buses, miniptm_i2c
from renesas_cm_configfiles import *

from renesas_cm_registers import * 
# one class object for ALL MiniPTM boards installed in the system
import time
from renesas_cm_gpio import gpiomode

# a single MiniPTM board is characterized by its PCIe info and i2c adapter
class Single_MiniPTM:
    def __init__(self, board_num, devinfo, adap_num):
        self.board_num = board_num
        self.devinfo = devinfo
        self.bar = devinfo[1]
        self.bar_size = devinfo[2]
        self.adap_num = adap_num

        self.PCIe = MiniPTM_PCIe( self.bar, self.bar_size ) 
        self.i2c = miniptm_i2c( adap_num ) 
        print(f"Register MiniPTM device {devinfo[0]} I2C bus {adap_num}")

        self.dpll = DPLL( self.i2c, self.i2c.read_dpll_reg_direct, self.i2c.write_dpll_reg_direct )
     

    def led_visual_test(self):
        for i in range(4):
            # do a simple blink a couple times
            print(f"*********LEDS OFF**********")
            for led in range(4):
                self.set_board_led(led,0)

            time.sleep(0.25)

            print(f"*********LEDS ON***********")
            for led in range(4):
                self.set_board_led(led,1)
            time.sleep(0.25)


    def set_led_id_code(self):
        # set LEDs based on board ID
        self.set_board_led(0, self.board_num & 0x1)
        self.set_board_led(1, (self.board_num >> 1) & 0x1)
        self.set_board_led(2, (self.board_num >> 2) & 0x1)
        self.set_board_led(3, (self.board_num >> 3) & 0x1)

    def set_board_led(self, led_num, val):
        if ( led_num < 0 or led_num > 3 ):
            print(f"Invalid LED number {led_num}")
            return


        # USING BOARD LAYOUT, LEDS FROM BOTTOM TO TOP
        # LED0 = I225 LED_SPEED_2500# , LED1
        # LED1 = I225 LINK_ACT# , LED2
        # LED2 = DPLL GPIO2
        # LED3 = DPLL GPIO3

        if val:
            val = 0
        else:
            val = 1
        if led_num == 2 or led_num == 3:
            if ( val ): 
                self.dpll.gpio.configure_pin(led_num, gpiomode.OUTPUT, 1)
            else:
                self.dpll.gpio.configure_pin(led_num, gpiomode.OUTPUT, 0)
        else:
            # need to do PCIe access to I225
            # LEDx_MODE , 0x0 = LED_ON (always on), 0x1 = LED_OFF (always low)
            # LED1 MODE = 0xE00[11:8]
            # LED1 BLINK = 0xE00[15] , turn this off otherwise it auto blinks when always on
            # LED2 MODE = 0xE00[19:16]
            # LED2 BLINK = 0xE00[23]

            led_mode = 0
            if ( val ):
                led_mode = 1

            if ( led_num == 1 ): #LED2
                val = self.PCIe.read32(0xe00)
                val = val & ~(1 << 23) # turn off blink
                val = val & ~(0xf << 16)
                val = val | (led_mode << 16)
                self.PCIe.write32(0xe00, val)
            else:
                val = self.PCIe.read32(0xe00)
                val = val & ~(1 << 15) #turn off blink
                val = val & ~(0xf << 8)
                val = val | (led_mode << 8)
                self.PCIe.write32(0xe00, val)


    

    def is_configured(self) -> bool:
        # check GPIO config registers for GPIOs used by DPLL for LEDs on MiniPTM
        # by default GPIOs are input, so if they're output assume config has been written previously

        # GPIO2 and GPIO3 are LEDs on MiniPTM board, programming guide doesnt match GUI, use GUI here
        gpio2 = self.i2c.read_dpll_reg(0xc8e6, 0x10)

        gpio3 = self.i2c.read_dpll_reg(0xc900, 0x10)

        #print(f"Check Miniptm adap {self.adap_num} is configured, 0x{gpio2:x} 0x{gpio3:x}")

        gpio2 &= 0x5
        gpio3 &= 0x5

        if ( gpio2 == 0x4 and gpio3 == 0x4 ):
            return True
        return False

