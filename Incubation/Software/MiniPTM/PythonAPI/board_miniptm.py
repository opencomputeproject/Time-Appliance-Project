

from pcie_miniptm import MiniPTM_PCIe, get_miniptm_devices
from i2c_miniptm import find_i2c_buses, miniptm_i2c
from renesas_cm_configfiles import *

from renesas_cm_registers import *
# one class object for ALL MiniPTM boards installed in the system
import time
from renesas_cm_gpio import gpiomode

from enum import Enum
# a single MiniPTM board is characterized by its PCIe info and i2c adapter

from dpll_over_fiber_miniptm import DPOF_Top


class Single_MiniPTM:
    def __init__(self, board_num, devinfo, adap_num):
        self.board_num = board_num
        self.devinfo = devinfo
        self.bar = devinfo[1]
        self.bar_size = devinfo[2]
        self.adap_num = adap_num

        self.PCIe = MiniPTM_PCIe(self.bar, self.bar_size)
        self.i2c = miniptm_i2c(adap_num)
        print(f"Register MiniPTM device {devinfo[0]} I2C bus {adap_num}")

        self.dpll = DPLL(self.i2c, self.i2c.read_dpll_reg_direct,
                         self.i2c.read_dpll_reg_multiple_direct,
                         self.i2c.write_dpll_reg_direct,
                         self.i2c.write_dpll_multiple)

        self.dpof = DPOF_Top(self)

    def led_visual_test(self):
        for i in range(4):
            # do a simple blink a couple times
            print(f"*********LEDS OFF**********")
            for led in range(4):
                self.set_board_led(led, 0)

            time.sleep(0.25)

            print(f"*********LEDS ON***********")
            for led in range(4):
                self.set_board_led(led, 1)
            time.sleep(0.25)

    def set_led_id_code(self):
        # set LEDs based on board ID
        self.set_board_led(0, self.board_num & 0x1)
        self.set_board_led(1, (self.board_num >> 1) & 0x1)
        self.set_board_led(2, (self.board_num >> 2) & 0x1)
        self.set_board_led(3, (self.board_num >> 3) & 0x1)

    def set_board_led(self, led_num, val):
        if (led_num < 0 or led_num > 3):
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
            if (val):
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
            if (val):
                led_mode = 1

            if (led_num == 1):  # LED2
                val = self.PCIe.read32(0xe00)
                val = val & ~(1 << 23)  # turn off blink
                val = val & ~(0xf << 16)
                val = val | (led_mode << 16)
                self.PCIe.write32(0xe00, val)
            else:
                val = self.PCIe.read32(0xe00)
                val = val & ~(1 << 15)  # turn off blink
                val = val & ~(0xf << 8)
                val = val | (led_mode << 8)
                self.PCIe.write32(0xe00, val)

    def init_eeprom_addr(self, block=0):
        addr = 0x54
        if (block):
            addr = 0x55
        # check if cached address matches what I want to set
        if (hasattr(self, "eeprom_addr")):
            if self.eeprom_addr != addr:
                self.dpll.modules["EEPROM"].write_field(
                    0, "EEPROM_I2C_ADDR", "I2C_ADDR", addr)
                self.eeprom_addr = addr
        else:
            self.dpll.modules["EEPROM"].write_field(
                0, "EEPROM_I2C_ADDR", "I2C_ADDR", addr)
            self.eeprom_addr = addr

    def write_to_eeprom(self, offset, data):
        if (offset > 0xffff):
            self.init_eeprom_addr(1)
        else:
            self.init_eeprom_addr(0)

        hex_values = [f"{value:02x}" for value in data]
        print(
            f"Write Board {self.board_num} EEPROM offset 0x{offset:x} data {hex_values}")

        # write offset
        self.dpll.modules["EEPROM"].write_field(0, "EEPROM_OFFSET_LOW", "EEPROM_OFFSET",
                                                offset & 0xff)
        self.dpll.modules["EEPROM"].write_field(0, "EEPROM_OFFSET_HIGH", "EEPROM_OFFSET",
                                                (offset >> 8) & 0xff)

        # write byte count
        data_len = len(data)
        if (data_len > 128):
            data_len = 128
            data = data[:128]

        self.dpll.modules["EEPROM"].write_field(
            0, "EEPROM_SIZE", "BYTES", data_len)

        # write the data to the data buffer
        for i in range(data_len):
            reg = f"BYTE_OTP_EEPROM_PWM_BUFF_{i}"
            self.dpll.modules["EEPROM_DATA"].write_reg(0, reg, data[i])

        # write the command
        self.dpll.modules["EEPROM"].write_field(
            0, "EEPROM_CMD_LOW", "EEPROM_CMD", 0x2)  # Write to EEPROM
        self.dpll.modules["EEPROM"].write_field(
            0, "EEPROM_CMD_HIGH", "EEPROM_CMD", 0xEE)  # Write to EEPROM

        # give it some time
        time.sleep(0.0005 * data_len)

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

    # returns the decimal value in picoseconds.
    # Positive means feedback clock (CLK8 = 100MHz PCIe) leads reference clock (CLK5 = Local 10MHz)
    # Negative means feedback clock lags reference clock
    def read_pcie_clk_phase_measurement(self, restart_phase=True, avg_count=5):
        # want to read DPLL_PHASE_STATUS for signed 36-bit phase offset in units of 50 picoseconds
        # it's a continous measurement, with register refreshing every 100us (irrelevant for i2c)
        # but it will rollover potentially , especially with large ppm
        # solution is to toggle DPLL between Synthesizer and Phase Measurement modes
        if (restart_phase):
            # set to synthesizer mode
            # DPLL1 hard coding for now, didn't include DPLL in map yet
            # 0x37[5:3] = mode, 0x4 for synthesizer, 0x5 for phase measurement
            base_addr = 0xc400
            reg_addr = 0x37

            self.i2c.write_dpll_reg(
                base_addr, reg_addr, 0x20)  # set synthesizer
            # set phase measurement
            self.i2c.write_dpll_reg(base_addr, reg_addr, 0x28)

            # dont remove these debug reads, it makes it work for some reason????
            debug = self.i2c.read_dpll_reg(0xc400, 0x36)
            # print(f"Read pcie clk phase debug val 0xc436 = 0x{debug:x}")
            debug = self.i2c.read_dpll_reg(0xc400, 0x37)
            # print(f"Read pcie clk phase debug val 0xc437 = 0x{debug:x}")

        # time.sleep(0.1)
        # Phase status register is 5 bytes, read all 5
        average = 0

        base_addr = self.dpll.modules["Status"].BASE_ADDRESSES[0]
        reg_addr = self.dpll.modules["Status"].LAYOUT["DPLL1_PHASE_STATUS_7_0"]["offset"]
        reg_addr = reg_addr + base_addr
        for count in range(avg_count):
            for i in range(5):
                data = self.i2c.read_dpll_reg_multiple(reg_addr, 0x0, 5)
                # print(
                #    f"Read PCIe clock phase measurement start 0x{reg_addr:02x} data={data}")
                if (sum(data) != 0):
                    break

            phase_val = 0
            for i, byte in enumerate(data):
                phase_val += byte << (8*i)
            # print(f"Raw phase value: {phase_val}")
            phase_val = int_to_signed_nbit(phase_val, 36)
            # print(f"Signed phase value: {phase_val}")
            average += phase_val

        average = average / avg_count
        return average

    def clear_all_dpll_sticky_status(self):
        # clear all stickies
        self.i2c.write_dpll_reg(0xc164, 0x5, 0x0)
        self.i2c.write_dpll_reg(0xc164, 0x5, 0x1)

    def print_pwm_channel_status(self):
        for i in range(0, 1, 2):
            # get the input monitor status
            self.dpll.modules["Status"].print_register(
                0, f"IN{i}_MON_STATUS", True)

            # get the input monitor ffo values
            self.dpll.modules["Status"].print_register(
                0, f"IN{i}_MON_FREQ_STATUS_0", True)
            self.dpll.modules["Status"].print_register(
                0, f"IN{i}_MON_FREQ_STATUS_1", True)

        self.clear_all_dpll_sticky_status()

    def print_sfps_info(self):
        for i in range(1, 5):
            self.i2c.read_sfp_module(i)

    def write_tod_absolute(self, tod_num=0, tod_subns=0, tod_ns=0, tod_sec=0):
        data = []
        data += [tod_subns & 0xff]
        data += [byte for byte in tod_ns.to_bytes(4, byteorder='little')]
        data += [byte for byte in tod_sec.to_bytes(6, byteorder='little')]
        base_addr = self.dpll.modules["TODWrite"].BASE_ADDRESSES[tod_num]
        reg_addr = self.dpll.modules["TODWrite"].LAYOUT["TOD_WRITE_SUBNS"]["offset"]
        addr = base_addr + reg_addr
        # print(f"Write TOD Absolute addr 0x{addr:x} -> {data}")
        self.i2c.write_dpll_multiple(addr, data)
        # write the trigger for immediate absolute
        self.dpll.modules["TODWrite"].write_reg(tod_num, "TOD_WRITE_CMD", 0x1)

    def init_pwm_dplloverfiber(self):
        # disable all decoders
        for i in range(len(self.dpll.modules["PWMDecoder"].BASE_ADDRESSES)):
            print(f"Debug PWM Decoder {i}")
            self.dpll.modules["PWMDecoder"].write_field(
                i, "PWM_DECODER_CMD", "ENABLE", 0)

        # initialize all TODs to PWM negotiation flag mode
        for i in range(len(self.dpll.modules["TODWrite"].BASE_ADDRESSES)):
            # using highest part of TOD seconds as PWM flag
            # highest
            self.write_tod_absolute(i, 0, 0, 0x80 << (8*5))

        # enable all encoders to transmit
        for i in range(len(self.dpll.modules["PWMEncoder"].BASE_ADDRESSES)):
            self.dpll.modules["PWMEncoder"].write_field(
                i, "PWM_ENCODER_CMD", "TOD_AUTO_UPDATE", 1)
            self.dpll.modules["PWMEncoder"].write_field(
                i, "PWM_ENCODER_CMD", "TOD_TX", 1)
            self.dpll.modules["PWMEncoder"].write_field(
                i, "PWM_ENCODER_CMD", "ENABLE", 1)

    def pwm_switch_listen_channel(self, decoder_num=0):
        # disable all decoders
        for i in range(len(self.dpll.modules["PWMDecoder"].BASE_ADDRESSES)):
            self.dpll.modules["PWMDecoder"].write_field(
                i, "PWM_DECODER_CMD", "ENABLE", 0)

        # enable the decoder I want
        if (decoder_num >= 0 or decoder_num <= 15):  # valid decoder
            self.dpll.modules["PWMDecoder"].write_field(
                decoder_num, "PWM_DECODER_CMD", "ENABLE", 1)

    # experimental function -> THIS WORKS, global RX buffer

    def read_pwm_tod_incoming(self):
        # read global register set 0xce80, 11 bytes
        data = self.i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)
        print(f"Debug pwm incoming, data={data}")

    # This works as per app note and to properly align TOD you need this procedure
    # However, for DPLL over fiber, using TOD as data channel
    # this is not necessary

    def read_pwm_tod_incoming_consumes_tod_as_rx(self):
        # following APN ToD over PWM

        # 3a. read TOD read primary counter, just use all TOD0 for now
        count = self.dpll.modules["TODReadPrimary"].read_field(
            0, "TOD_READ_PRIMARY_COUNTER", "READ_COUNTER")
        print(f"Debug pwm incoming, TODReadPrimary count = {count}")

        # 3b. clear any previous trigger
        self.dpll.modules["TODReadPrimary"].write_field(
            0, "TOD_READ_PRIMARY_CMD", "TOD_READ_TRIGGER", 0x0)

        # 3c. Configure PWM Decoder for trigger, HARD CODE FOR INPUT 0
        self.dpll.modules["TODReadPrimary"].write_field(
            0, "TOD_READ_PRIMARY_SEL_CFG_0", "PWM_DECODER_INDEX", 0)

        # 3c-i . Configure ToD read trigger mode to single shot. Configure trigger on PWM_read_primary
        #   PWM internal clock by setting selected PWM decoder's 1PPS output
        self.dpll.modules["TODReadPrimary"].write_reg(
            0, "TOD_READ_PRIMARY_CMD", 0x4)

        # 3c-ii . Poll Read counter until it increments for ~3 seconds
        for i in range(3*5):
            new_count = self.dpll.modules["TODReadPrimary"].read_field(
                0, "TOD_READ_PRIMARY_COUNTER", "READ_COUNTER")
            print(
                f"Debug pwm incoming, TODReadPrimary count = {count}, new_count = {new_count}")
            if (new_count != count):
                print(f"Got incoming ToD!")
                # 3c-iii . Read and store value of TOD_READ_PRIMARY
                base_addr = self.dpll.modules["TODReadPrimary"].BASE_ADDRESSES[0]
                reg_offset = self.dpll.modules["TODReadPrimary"].layout["TOD_READ_PRIMARY_SUBNS"]["offset"]
                new_tod = self.i2c.read_dpll_reg_multiple(
                    base_addr, reg_offset, 11)

                # THIS DOES WORK!!!!!!!!
                print(f"New TOD from read primary: {new_tod}")

                # read global register set 0xce80, 11 bytes -> THIS DOES NOT WORK WITHOUT FIRMWARE
                data = self.i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)
                print(f"Debug pwm incoming, data={data}")
                break
            time.sleep(0.25)

    # call this periodically , non blocking "super-loop" function
    def dpll_over_fiber_loop(self):
        print(f"Board {self.board_num} dpll_over_fiber_loop")
        self.dpof.run_loop()
        #self.dpof.tick()
        print(f"Board {self.board_num} done dpll_over_fiber_loop")
