

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

class PIController:
    def __init__(self, kp, ki):
        self.kp = kp
        self.ki = ki
        self.integral = 0

    def update(self, error):
        self.integral += error
        return self.kp * error + self.ki * self.integral




class Single_MiniPTM:
    def __init__(self, board_num, devinfo, adap_num):
        self.board_num = board_num
        self.devinfo = devinfo
        self.bar = devinfo[1]
        self.bar_size = devinfo[2]
        self.adap_num = adap_num

        self.PCIe = MiniPTM_PCIe(self.bar, self.bar_size)
        self.i2c = miniptm_i2c(adap_num)
        self.best_clock_quality_seen = 255 - board_num # hacky
        #print(f"Register MiniPTM device {devinfo[0]} I2C bus {adap_num}")

        self.dpll = DPLL(self.i2c, self.i2c.read_dpll_reg_direct,
                         self.i2c.read_dpll_reg_multiple_direct,
                         self.i2c.write_dpll_reg_direct,
                         self.i2c.write_dpll_multiple)

        self.dpof = DPOF_Top(self)

        self.tod_pi = []
        for i in range(4):
            self.tod_pi.append( PIController(0.6, 0.2) )

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
        if (led_num < 0 or led_num > 2):
            print(f"Invalid LED number {led_num}")
            return
        if ( led_num == 0 ):
            #GPIO13
            self.dpll.gpio.configure_pin(13, gpiomode.OUTPUT,val)
        if ( led_num == 1 ):
            #GPIO5
            self.dpll.gpio.configure_pin(5, gpiomode.OUTPUT,val)
        if ( led_num == 2 ):
            #GPIO6
            self.dpll.gpio.configure_pin(6, gpiomode.OUTPUT,val)



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
        #print(f"Read pcie clk phase board {self.board_num} val = {average}")
        return average



    # loopbw_units same as DPL_CTRL_0.DPLL_BW definitions
    def set_dpll_loop_params(self, dpll_num=0,
            loopbw=1000, loopbw_units = 1,
            decimator_bw_mult=4, psl=0):

        self.dpll.modules["DPLL_Ctrl"].write_reg(dpll_num,
                "DPLL_PSL_7_0", psl & 0xff)
        self.dpll.modules["DPLL_Ctrl"].write_reg(dpll_num,
                "DPLL_PSL_15_8", (psl >> 8) & 0xff)
        self.dpll.modules["DPLL_Ctrl"].write_reg(dpll_num,
                "DPLL_DECIMATOR_BW_MULT", decimator_bw_mult & 0xff)
        self.dpll.modules["DPLL_Ctrl"].write_reg(dpll_num,
                "DPLL_BW_0", loopbw & 0xff)
        self.dpll.modules["DPLL_Ctrl"].write_reg(dpll_num,
                "DPLL_BW_1", ((loopbw>>8) & 0x3f) + (loopbw_units << 6) )

        



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
        #self.i2c.read_sfp_module(1)
        #return
        for i in range(1, 5):
            self.i2c.read_sfp_module(i)



    def setup_phase_measurement(self, channel, ref_clk, meas_clk):
        self.dpll.modules["DPLL_Config"].write_field(channel,
                "DPLL_PHASE_MEASUREMENT_CFG", "PFD_FB_CLK_SEL", ref_clk)

        self.dpll.modules["DPLL_Config"].write_field(channel,
                "DPLL_PHASE_MEASUREMENT_CFG", "PFD_REF_CLK_SEL", meas_clk)

        # set it to phase measurement mode
        self.dpll.modules["DPLL_Config"].write_reg(channel,
                "DPLL_MODE", (0x5 << 3) )


    # need to do this if phase is rolling over
    def restart_phase_measurement(self, channel):
        ref_clk = self.dpll.modules["DPLL_Config"].read_field(channel,
                "DPLL_PHASE_MEASUREMENT_CFG", "PFD_FB_CLK_SEL")

        meas_clk = self.dpll.modules["DPLL_Config"].read_field(channel,
                "DPLL_PHASE_MEASUREMENT_CFG", "PFD_REF_CLK_SEL")

        # method 1: Swap these two
        self.dpll.modules["DPLL_Config"].write_field(channel,
                "DPLL_PHASE_MEASUREMENT_CFG", "PFD_FB_CLK_SEL", meas_clk)

        self.dpll.modules["DPLL_Config"].write_field(channel,
                "DPLL_PHASE_MEASUREMENT_CFG", "PFD_REF_CLK_SEL", ref_clk)

        # set it to phase measurement mode
        self.dpll.modules["DPLL_Config"].write_reg(channel,
                "DPLL_MODE", (0x5 << 3) )


    def read_phase_measurement_mode(self, channel, high_precision=True):
        if ( high_precision ):
            reg_name = f"DPLL{channel}_FILTER_STATUS_7_0"
            phase_reg = self.dpll.modules["Status"].read_reg_mul(0, reg_name, 6)
            phase_val = 0
            for i, byte in enumerate(phase_reg):
                phase_val += byte << (8*i)
            phase_val = int_to_signed_nbit(phase_val, 48)
            return phase_val
        else:
            reg_name = f"DPLL{channel}_PHASE_STATUS_7_0"
            phase_reg = self.dpll.modules["Status"].read_reg_mul(0, reg_name, 5)
            phase_val = 0
            for i, byte in enumerate(phase_reg):
                phase_val += byte << (8*i)
            phase_val = int_to_signed_nbit(phase_val, 36)
            return phase_val

    def init_pwm_dplloverfiber(self):
        # disable all decoders
        for i in range(len(self.dpll.modules["PWMDecoder"].BASE_ADDRESSES)):
            #print(f"Debug PWM Decoder {i}")
            self.dpll.modules["PWMDecoder"].write_field(
                i, "PWM_DECODER_CMD", "ENABLE", 0)

        # initialize all TODs 
        for i in range(len(self.dpll.modules["TODWrite"].BASE_ADDRESSES)):
            self.dpof.write_tod_absolute(i, 0, 0, 0)

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


    # call this periodically , non blocking "super-loop" function
    def dpll_over_fiber_loop(self):
        #print(f"Board {self.board_num} dpll_over_fiber_loop")
        #self.dpof.run_loop()
        self.dpof.tick()
        #print(f"Board {self.board_num} done dpll_over_fiber_loop")



    

    def setup_dpll_track_and_priority_list(self, dpll_num, input_priorities=[]):
        if ( len(input_priorities) == 0 ):
            return

        print(f"Board {self.board_num} setting input priorities and dpll{dpll_num} mode {input_priorities}")
        for index, chan in enumerate(input_priorities):
            reg_val = (chan << 1) + 1
            print(f"Setting DPLL{dpll_num} Priority {index} = {reg_val}")
            self.dpll.modules["DPLL_Config"].write_reg(dpll_num, 
                    f"DPLL_REF_PRIORITY_{index}", reg_val)

        # keep primary combo slave for local TCXO, but disable secondary source
        #self.dpll.modules["DPLL_Config"].write_field(2,
        #        "DPLL_COMBO_SLAVE_CFG_0", "PRI_COMBO_SRC_EN", 0)
        self.dpll.modules["DPLL_Config"].write_field(dpll_num,
                "DPLL_COMBO_SLAVE_CFG_1", "SEC_COMBO_SRC_EN", 0)

        # reference inputs are setup, now switch to DPLL mode, just 0
        self.dpll.modules["DPLL_Config"].write_reg(dpll_num,
                    "DPLL_MODE", 0)



    # usually want to do relative adjustments, handle that here
    def add_output_phase_offset(self, output_num, offset_nanoseconds):
        print(f"Add output phase offset {output_num} by {offset_nanoseconds}")
        cur_adjust = self.dpll.modules["Output"].read_reg_mul(output_num,
                "OUT_PHASE_ADJ_7_0", 4)
        cur_adjust_val = 0
        for index,val in enumerate(cur_adjust):
            cur_adjust_val += val << (8*index)
        cur_adjust_val = int_to_signed_nbit(cur_adjust_val, 32)

        # assume 500MHz FOD, units on this register are 2ns units
        cur_adjust_val_ns = cur_adjust_val * 2
        print(f"Current output phase adjust {cur_adjust_val}, {cur_adjust_val_ns} ns")

        cur_adjust_val_ns += offset_nanoseconds

        cur_adjust_val_ns = cur_adjust_val_ns // 2 # divide by 2 again to get into units

        new_adjust_bytes = to_twos_complement_bytes( int(cur_adjust_val_ns), 32)
        print(f"New output phase adjust {new_adjust_bytes}, {cur_adjust_val_ns*2} ns")

        self.dpll.modules["Output"].write_reg_mul(output_num,
                "OUT_PHASE_ADJ_7_0", new_adjust_bytes)

        
    def get_tod_trigger_from_pps(self, tod_num = 0, use_sec=True, is_pwm_decoder=False, input_num=0, timeout=3):
        if use_sec:
            module = self.dpll.modules["TODReadSecondary"]
            counter = "TOD_READ_SECONDARY_COUNTER"
            cfg_name = "TOD_READ_SECONDARY_SEL_CFG_0"
            cmd = "TOD_READ_SECONDARY_CMD"
            tod_start = "TOD_READ_SECONDARY_SUBNS"
        else:
            module = self.dpll.modules["TODReadPrimary"]
            counter = "TOD_READ_PRMIARY_COUNTER"
            cfg_name = "TOD_READ_PRIMARY_SEL_CFG_0"
            cmd = "TOD_READ_PRIMARY_CMD"
            tod_start = "TOD_READ_PRIMARY_SUBNS"

        module.write_reg(tod_num, cmd, 0x0) # disable any trigger
        start_count = module.read_reg(tod_num, counter)

        if ( is_pwm_decoder ):
            module.write_reg(tod_num, cfg_name, (input_num & 0xf) << 4) # PWM decoder
            module.write_reg(tod_num, cmd, 0x4)
        else:
            module.write_reg(tod_num, cfg_name, input_num & 0xf) # input reference
            module.write_reg(tod_num, cmd, 0x3)
        start_time = time.time()

        while ( time.time() < ( start_time + timeout ) ):
            cur_count = module.read_reg(tod_num, counter)
            if ( cur_count != start_count ): # got trigger
                tod_val = module.read_reg_mul(tod_num, tod_start, 11)
                return tod_val
            time.sleep(0.1) # don't spam i2c

        # only hit here if timed out
        print(f"Get tod trigger from PPS timed out!")
        return []


    # tod_num is which TOD to use to check frame sync mode with
    # dpll_frame_sync is list of dplls that are frame syncing at the same time 
    def wait_for_frame_sync_loopback_stable(self, tod_num=0, dpll_frame_sync=[0], clkin=13, timeout=30, good_count_threshold=10):
        start_time = time.time()
        tod_vals = []
        count = 0
        good_count = 0

        # make sure frame sync mode is enabled and trigger it

        for dpll_num in dpll_frame_sync:
            self.dpll.modules["DPLL_Config"].write_field(dpll_num,
                    "DPLL_CTRL_2", "FRAME_SYNC_MODE", 1)
            self.dpll.modules["DPLL_Ctrl"].write_reg(dpll_num, 
                    "DPLL_FRAME_PULSE_SYNC", 0x1)

        while time.time() - start_time <= timeout:
            # trigger TOD with frame sync loopback clkin
            tod_val = self.get_tod_trigger_from_pps(tod_num, True, False, clkin)
            if not len(tod_val):
                continue # didnt get a valid reading, go again

            # got a valid reading
            tod_ns = time_to_nanoseconds(tod_val)
            print(f"Count {count}, Board {self.board_num}, wait for frame sync, TOD{tod_num} = {tod_ns}")
            count += 1
            tod_vals.append(tod_ns)
            if len(tod_vals) >= 2:
                diff = tod_vals[-1] - tod_vals[-2]
                print(f"Difference: {diff}")
                if ( diff == 1e9 ):
                    # difference is exactly 1e9 nanoseconds, as it should be
                    good_count += 1
                tod_vals = tod_vals[-2:]

            if ( good_count >= good_count_threshold ):
                # seen at least some number of exact 1e9 nanosecond tod values
                # disable frame sync mode on both channels
                print(f"Saw {good_count} exact 1e9 frame sync pulses, trying disable frame sync")
                for dpll_num in dpll_frame_sync:
                    self.dpll.modules["DPLL_Config"].write_field(dpll_num,
                            "DPLL_CTRL_2", "FRAME_SYNC_MODE", 0)
                 
                # check tod again
                tod_val = self.get_tod_trigger_from_pps(tod_num, True, False, clkin)
                tod_ns = time_to_nanoseconds(tod_val)
                tod_diff = tod_ns - tod_vals[-2]
                if not len(tod_val) or tod_diff != 1e9:
                    print(f"After frame sync disable, didn't get 1e9 timestamp, restart, {tod_ns} , {tod_vals[-2]}")
                    for dpll_num in dpll_frame_sync:
                        self.dpll.modules["DPLL_Config"].write_field(dpll_num,
                                "DPLL_CTRL_2", "FRAME_SYNC_MODE", 1)
                        self.dpll.modules["DPLL_Ctrl"].write_reg(dpll_num, 
                                "DPLL_FRAME_PULSE_SYNC", 0x1)
                    good_count = 0
                    tod_vals = []
                    count = 0
                else:
                    print(f"After frame sync disable, got 1e9 timestamp, slave frame sync good!")
                    return True
        return False # timeout expired


