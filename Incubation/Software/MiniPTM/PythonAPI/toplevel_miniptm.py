
from pcie_miniptm import get_miniptm_devices
from i2c_miniptm import find_i2c_buses
from board_miniptm import Single_MiniPTM
from renesas_cm_configfiles import *
import concurrent.futures
import time
import argparse
from dpll_over_fiber_miniptm import *
from scipy import stats
import sys
import threading
import select
from scipy.signal import medfilt
import numpy as np


def input_with_timeout(timeout):
    # print("Enter something: ", end='', flush=True)
    ready, _, _ = select.select([sys.stdin], [], [], timeout)
    if ready:
        return sys.stdin.readline().rstrip('\n')  # Reads the whole line
    else:
        return None


def input_thread_func(stop_event):
    while not stop_event.is_set():
        result = input_with_timeout(1)  # 1 second timeout
        if result is not None:
            # print(f"Input received: {result}")
            with MiniPTM.user_input_lock:
                MiniPTM.user_input_str.append(result.lower())


class PIController:
    def __init__(self, kp, ki):
        self.kp = kp
        self.ki = ki
        self.integral = 0

    def update(self, error):
        self.integral += error
        return self.kp * error + self.ki * self.integral


class MovingAverageFilter:
    def __init__(self, window_size=5, outlier_percentage=50):
        self.window_size = window_size
        self.outlier_percentage = outlier_percentage / 100.0
        self.rolling_window = []
        self.previous_average = None

    def update(self, new_sample):
        if self.previous_average is not None and self.previous_average != 0:
            percentage_change = abs(new_sample - self.previous_average) / abs(self.previous_average)
            
            if percentage_change > self.outlier_percentage:
                return self.previous_average  # Ignore outlier and return previous average

        self.rolling_window.append(new_sample)
        if len(self.rolling_window) > self.window_size:
            self.rolling_window.pop(0)

        if len(self.rolling_window) > 0:
            self.previous_average = np.mean(self.rolling_window)
        
        return self.previous_average

class MiniPTM:
    user_input_str = []
    user_input_lock = threading.Lock()
    # PFM_KP = 10
    # PFM_KI = 0.2 for input TDC mode
    PFM_KP = 0.7
    PFM_KI = 0.3

    def __init__(self):
        miniptm_devs = get_miniptm_devices()
        print(f"Got {len(miniptm_devs)} mini ptm devices: {miniptm_devs}")

        i2c_busses = find_i2c_buses("MiniPTM I2C Adapter")
        print(f"Matching i2c buses: {i2c_busses}")

        if (len(miniptm_devs) != len(i2c_busses)):
            print("Mismatch between number of pcie devices and i2c busses, end!")
            return

        # assume theyre in order, probably terrible assumption but oh well
        self.boards = []

        for i in range(len(miniptm_devs)):
            self.boards.append(Single_MiniPTM(
                i, miniptm_devs[i], i2c_busses[i]))

    def check_user_input(self):
        # pop the latest

        with MiniPTM.user_input_lock:
            while len(MiniPTM.user_input_str):
                val = MiniPTM.user_input_str.pop(0)
                if (len(val) == 0):
                    continue
                print(f"User entered {val}")
                if val == "quit":
                    quit()
                else:
                    parts = val.split()
                    print(f"User input in parts, {parts}")
                    if (parts[0] == 'set'):
                        if (parts[1] == "kp"):
                            MiniPTM.PFM_KP = float(parts[2])
                        elif (parts[1] == "ki"):
                            MiniPTM.PFM_KI = float(parts[2])

    def set_all_boards_leds_idcode(self):
        # leave LEDs to show what board number it is
        for board in self.boards:
            board.set_led_id_code()

    def program_one_board(self, board, data):
        for [address, value] in data:
            print(
                f"Board {board.adap_num} 0x{address:x}=0x{value:x}")
            board.i2c.write_dpll_reg_direct(address, value)
        return board.adap_num

    def program_all_boards(self, config_file="8A34002_MiniPTMV3_12-24-2023_Julian.tcs", check_first=False):
        # now lets do some initialization if needed. Do a simple check on each board
        # if the GPIOs for LEDs are set for output, then assume the board is configured
        parsed_config_tcs = parse_dpll_tcs_config_file(config_file)
        # print(f"First few tcs lines: {parsed_config_tcs[:10]}")

        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures = []
            for board in self.boards:
                if (check_first):
                    if not board.is_configured():
                        print(
                            f"Board {board.adap_num} not configured, configuring!")
                        futures.append(executor.submit(
                            self.program_one_board, board, parsed_config_tcs))
                        # self.program_one_board(board, parsed_config_tcs)
                    else:
                        print(f"Board {board.adap_num} already configured!")
                else:
                    print(f"Board {board.adap_num} configuring!")
                    futures.append(executor.submit(
                        self.program_one_board, board, parsed_config_tcs))
                    # self.program_one_board(board, parsed_config_tcs)
            print(futures)
            for future in concurrent.futures.as_completed(futures):
                pass

    def flash_eeprom_one_board(self, board, eeprom_file="8A34002_MiniPTMV3_12-29-2023_Julian_AllPhaseMeas_EEPROM.hex"):
        print(f"Start flash board {board.board_num} EEPROM = {eeprom_file}")
        board.write_eeprom_file(eeprom_file)
        print(f"DONE flash board {board.board_num} EEPROM = {eeprom_file}")

    def flash_all_boards_eeprom(self, eeprom_file="8A34002_MiniPTMV3_12-29-2023_Julian_AllPhaseMeas_EEPROM.hex"):
        with concurrent.futures.ThreadPoolExecutor() as executor:
            futures = []
            for board in self.boards:
                futures.append(executor.submit(
                    self.flash_eeprom_one_board, board, eeprom_file))

            for future in concurrent.futures.as_completed(futures):
                pass

    def board_led_blink_test(self):
        # do ID test with GPIOs, toggle DPLL GPIO, then toggle I225 LEDs
        for index, board in enumerate(self.boards):
            print(f"Board {index} LED test")
            board.led_visual_test()

    def print_all_full_dpll_status(self):
        for board in self.boards:
            print(
                f"****************** BOARD {board.board_num} STATUS REGISTERS *************")
            board.dpll.modules["Status"].print_all_registers_all_modules()

    def print_all_pcie_clock_info(self):
        for board in self.boards:
            print(
                f"\n****************** BOARD {board.board_num} PCIE CLOCK REGISTERS ********\n")
            # Input 8 (GPIO0) configuration
            board.dpll.modules["Input"].print_all_registers(8)
            # REFMON 8 Configuration
            board.dpll.modules["REFMON"].print_all_registers(8)
            # STATUS register
            board.dpll.modules["Status"].print_register(
                0, "IN8_MON_STATUS", True)
            board.dpll.modules["Status"].print_register(
                0, "IN8_MON_FREQ_STATUS_0", True)
            board.dpll.modules["Status"].print_register(
                0, "IN8_MON_FREQ_STATUS_1", True)
            board.dpll.modules["Status"].print_register(
                0, "DPLL1_PHASE_STATUS_7_0", True)
            board.dpll.modules["Status"].print_register(
                0, "DPLL1_PHASE_STATUS_15_8", True)
            board.dpll.modules["Status"].print_register(
                0, "DPLL1_PHASE_STATUS_23_16", True)
            board.dpll.modules["Status"].print_register(
                0, "DPLL1_PHASE_STATUS_31_24", True)
            board.dpll.modules["Status"].print_register(
                0, "DPLL1_PHASE_STATUS_35_32", True)

    def do_pfm_use_refmon_ffo_DOES_NOT_WORK(self):
        # use IN8_MON_FREQ_STATUS register to do frequency comparison
        # simple algorithm , compare board 0 FFO value with other boards
        # crap code, do it sequentially, SHOULD BE PARALLEL
        # DOES NOT WORK, FFO units are not user controllable, can't set to ppb vs ppm
        ffo_values = []
        for board in self.boards:
            board.dpll.modules["Status"].print_register(
                0, "IN8_MON_FREQ_STATUS_0", True)
            board.dpll.modules["Status"].print_register(
                0, "IN8_MON_FREQ_STATUS_1", True)

            lower = board.dpll.modules["Status"].read_field(
                0, "IN8_MON_FREQ_STATUS_0", "FFO_7_0")
            upper = board.dpll.modules["Status"].read_field(
                0, "IN8_MON_FREQ_STATUS_1", "FFO_13:8")
            print(
                f"Board {board.board_num} , Read FFO lower = {lower} upper = {upper}")
            # total = (upper << 8) + lower
            # ffo_values.append( hex_to_signed_

    def do_pfm_use_output_tdc(self):
        # Use Output TDC 1 , measure between Input8 (GPIO0) and DPLL3 (Zero delay output)
        # DOES NOT WORK, Can't configure Output TDC to measure GPIO0
        pass

    def do_pfm_output_tdc_one_measurement(self):
        results_first = []
        results_first_int = []
        with concurrent.futures.ThreadPoolExecutor() as executor:
            # Output TDC trigger time should be concurrent, reading it back doesn't matter
            # 0x89 = filter disabled, clear accumulated , single shot, measurement, GO
            futures = [executor.submit(self.boards[j].dpll.modules["OUTPUT_TDC"].write_reg, 2,
                                       "OUTPUT_TDC_CTRL_4", 0x89) for j in range(len(self.boards))]
            results_first = [future.result()
                             for future in futures]  # get them in order

            results_first = []

            for board in self.boards:
                for i in range(100):
                    status = board.dpll.modules["Status"].read_reg(
                        0, "OUTPUT_TDC2_STATUS")
                    print(
                        f"Board {board.board_num} output tdc2 status = 0x{status:02x}")
                    if (not (status & 0x2)):  # not in progress
                        results_first.append(board.dpll.modules["Status"].read_reg_mul(0,
                                                                                       "OUTPUT_TDC2_MEASUREMENT_7_0", 6))
                        break
                    else:
                        time.sleep(0.1)

            results_first_int = []
            for val in results_first:
                phase_val = 0
                for i, byte in enumerate(val):
                    phase_val += byte << (8*i)
                phase_val = int_to_signed_nbit(phase_val, 36)
                # already in picoseconds apparently
                results_first_int.append(phase_val)

            print(
                f" Got first results, {results_first}, int {results_first_int}")
            return results_first_int

    def old_do_pfm_sacrifice_dpll3_use_output_tdc(self):
        # DPLL 0 / DPLL 1 are combo slaves of DPLL 2
        # DPLL 2 is DCO Operation with write frequency mode, combo slave to system DPLL
        # DPLL 3 is sacrificed, tracking 100MHz input divided down to 16KHz
        # Use output TDC 2, source = DPLL2 target = DPLL3 in single shot mode to measure phase
        if (len(self.boards) <= 1):
            print("PFM only works with two+ boards, ending")
            return

        results_board_0 = []
        results_board_1 = []
        cur_ppm_adjust = 0

        slope_differences = []
        for board in self.boards:
            # start of it , make sure frequency adjust word is zero
            board.dpll.modules["DPLL_Freq_Write"].write_reg_mul(2,
                                                                "DPLL_WR_FREQ_7_0", [0, 0, 0, 0, 0, 0])
            # assume output TDC2 is setup from config file
            print(f"\n********* Board {board.board_num} config ************\n")
            board.dpll.modules["OUTPUT_TDC_CFG"].print_all_registers(0)
            board.dpll.modules["OUTPUT_TDC"].print_all_registers(2)
            reg = board.dpll.modules["Status"].read_reg(
                0, "OUTPUT_TDC_CFG_STATUS")
            print(f"OUTPUT_TDC_CFG_STATUS = 0x{reg:02x}")
            reg = board.dpll.modules["Status"].read_reg(
                0, "OUTPUT_TDC2_STATUS")
            print(f"OUTPUT_TDC2_STATUS = 0x{reg:02x}")

        print(f"Start pfm use sacrificial DPLL3 and output tdc")
        for j in range(1000):
            print(f"\nLoop{j}")
            results_first = []
            results_second = []
            results_first = self.do_pfm_output_tdc_one_measurement()
            time.sleep(0.25)  # some time to accumulate phase offset
            results_second = self.do_pfm_output_tdc_one_measurement()

            print(
                f"Board 0 measurements {results_first[0]},{results_second[0]}")
            print(
                f"Board 1 measurements {results_first[1]},{results_second[1]}")
            board0_change = results_second[0] - results_first[0]
            board1_change = results_second[1] - results_first[1]
            change_diff = board0_change - board1_change

            print(
                f"Loop {j} board0_change={board0_change} , board1_change={board1_change}, diff = {change_diff}")
            print(
                f"{j} -> Slope_diff = {change_diff} , Cur_ppm_adjust = 0, FCW change to [0,0,0,0,0,0]")

    # Median filter function

    def moving_average_with_outlier_removal(data, window_size=5, outlier_threshold=1.5):
        q25, q75 = np.percentile(data, [25, 75])
        iqr = q75 - q25
        lower_bound = q25 - (iqr * outlier_threshold)
        upper_bound = q75 + (iqr * outlier_threshold)

        filtered_data = [x for x in data if lower_bound <= x <= upper_bound]
        if not filtered_data:  # If all data are outliers, return an empty array
            return np.array([])

        ma_filtered = np.convolve(filtered_data, np.ones(
            window_size), 'valid') / window_size
        return ma_filtered

    def handle_all_outliers(previous_valid_value):
        return previous_valid_value

    def do_pfm_sacrifice_dpll3(self):
        # Use DPLL3 in DPLL mode locked to 100MHz
        # read back loop filter status values from DPLL3 and compare between boards
        # Feed into DPLL2 as FCW
        if (len(self.boards) <= 1):
            print("PFM only works with two+ boards, ending")
            return

        ffo_diff_history = []
        cur_ppm_adjust = 0

        integral = 0

        for board in self.boards:
            # start of it , make sure frequency adjust word is zero
            board.dpll.modules["DPLL_Freq_Write"].write_reg_mul(2,
                                                                "DPLL_WR_FREQ_7_0", [0, 0, 0, 0, 0, 0])

        time.sleep(2)
        print(f"Start pfm use sacrifical DPLL3")
        time_diff = 0
        last_ffo_write = 0
        alpha = 0.1
        filter0 = MovingAverageFilter(window_size=5, outlier_percentage=50)
        filter1 = MovingAverageFilter(window_size=5, outlier_percentage=50)

        for j in range(10000):
            results_first_int = []
            print(f"\nLoop{j}")

            time_start = time.time()
            with concurrent.futures.ThreadPoolExecutor() as executor:
                results = []
                futures = [executor.submit(self.boards[j].dpll.modules["Status"].read_reg_mul, 0,
                                           "DPLL3_FILTER_STATUS_7_0", 6) for j in range(len(self.boards))]
                results_first = [future.result()
                                 for future in futures]  # get them in order

                print(f" Got first results, {results_first}")
            for val in results_first:
                phase_val = 0
                for i, byte in enumerate(val):
                    phase_val += byte << (8*i)
                phase_val = int_to_signed_nbit(phase_val, 48)
                # 48-bit FFO value in units of 2^-53
                results_first_int.append(phase_val)  # units of 50ps

            print(
                f"Loop {j} board0 = {results_first_int[0]} , board1 = {results_first_int[1]}")

            # need to filter these inputs actually, the noise here causes problems
            filtered_value0 = filter0.update(results_first_int[0])
            filtered_value1 = filter1.update(results_first_int[1])
        
            filtered_value0 = results_first_int[0]
            filtered_value1 = results_first_int[1]

            if (filtered_value0 is None or filtered_value1 is None):
                print(
                    f"Filtered value is none, pass, {filtered_value0} {filtered_value1}")
                continue

            print(f"Filtered values {filtered_value0} , {filtered_value1}")

            ffo_diff = filtered_value1 - filtered_value0
            ffo_ratio = filtered_value1 / filtered_value0
            print(f"FFO difference={ffo_diff}, ratio = {ffo_ratio}")

            # simple low pass filter
            #if (last_ffo_write == 0):
            #    last_ffo_write = ffo_diff
            #ffo_to_write = alpha * ffo_diff + (1 - alpha) * last_ffo_write
            #last_ffo_write = ffo_to_write

            # just use value directly
            ffo_to_write = ffo_diff

            print(f"FFO to write after filter:  {ffo_to_write}")
            # apparently write frequency is already in the same scale, just convert it
            ffo_to_write_bytes = to_twos_complement_bytes(
                int(ffo_to_write), 42)
            hex_val = [hex(val) for val in ffo_to_write_bytes]
            print(f"Write frequency bytes {hex_val}")
            self.boards[1].dpll.modules["DPLL_Freq_Write"].write_reg_mul(2,
                    "DPLL_WR_FREQ_7_0", ffo_to_write_bytes)

            time.sleep(0.1)
            time_diff += time.time() - time_start

    def do_pfm_use_input_tdc(self):
        # Use Input TDC from DPLL1 , measure CLK8 (10MHz from PCIe 100MHz divided down)
        #   versus CLK5 (10MHz loopback from Q7 for zero delay)

        if (len(self.boards) <= 1):
            print("PFM only works with two+ boards, ending")
            return

        results_board_0 = []
        results_board_1 = []
        cur_ppm_adjust = 0

        slope_differences = []
        num_per_slope = 2

        integral = 0

        for board in self.boards:
            # start of it , make sure frequency adjust word is zero
            board.dpll.modules["DPLL_Freq_Write"].write_reg_mul(3,
                                                                "DPLL_WR_FREQ_7_0", [0, 0, 0, 0, 0, 0])

            # HACK FROM RENESAS, DISABLE DECIMATOR OF PFD
            board.i2c.write_dpll_reg_direct(0x8a2d, 0x0)  # channel 0
            board.i2c.write_dpll_reg_direct(0x8b2d, 0x0)  # channel 1
            board.i2c.write_dpll_reg_direct(0x8c2d, 0x0)  # channel 2
            board.i2c.write_dpll_reg_direct(0x8d2d, 0x0)  # channel 3

        print(f"Start pfm use input tdc")
        # reset it
        for board in self.boards:
            board.read_pcie_clk_phase_measurement(True, 1)

        for j in range(200):
            difference_board_0 = []
            difference_board_1 = []
            time_diff = 0
            print(f"\nLoop{j}")
            self.check_user_input()
            with concurrent.futures.ThreadPoolExecutor() as executor:
                results = []
                self.check_user_input()
                futures = [executor.submit(self.boards[j].read_pcie_clk_phase_measurement, False, 1)
                           for j in range(len(self.boards))]
                results_first = [future.result()
                                 for future in futures]  # get them in order
                # print(f" Got first results, {results_first}")
                start_time = time.time()
                time.sleep(2)  # give it time to accumulate phase

                futures = [executor.submit(self.boards[j].read_pcie_clk_phase_measurement, False, 1)
                           for j in range(len(self.boards))]
                results_second = [future.result()
                                  for future in futures]  # get them in order
                # print(f" Got second results, {results_second}")

                end_time = time.time()
                time_diff = end_time - start_time
                self.check_user_input()

                results_board_0.append(
                    results_first[0] * 50e-12)  # in units of 50ps
                results_board_0.append(
                    results_second[0] * 50e-12)  # in units of 50ps

                print(
                    f"Board 0 measurements {results_board_0[-2]},{results_board_0[-1]}")

                results_board_1.append(
                    results_first[1] * 50e-12)  # in units of 50ps
                results_board_1.append(
                    results_second[1] * 50e-12)  # in units of 50ps

                print(
                    f"Board 1 measurements {results_board_1[-2]},{results_board_1[-1]}")

                board0_change = (
                    results_second[0] * 50e-12) - (results_first[0] * 50e-12)
                board1_change = (
                    results_second[1] * 50e-12) - (results_first[1] * 50e-12)

                # if saturates, change is zero, use that as threshold
                if (abs(results_second[0]) > 0.0004094 or abs(results_second[1]) > 0.0004094):
                    for index, board in enumerate(self.boards):
                        print(
                            f"Board {index} trying to reset phase measurement")
                        # flip the clocks being measured
                        cfg = self.boards[index].dpll.modules["DPLL_Config"].read_reg(1,
                                                                                      "DPLL_PHASE_MEASUREMENT_CFG")
                        fb_clk = (cfg >> 4) & 0xf
                        ref_clk = cfg & 0xf

                        new_cfg = (fb_clk) + (ref_clk << 4)
                        self.boards[index].dpll.modules["DPLL_Config"].write_reg(1,
                                                                                 "DPLL_PHASE_MEASUREMENT_CFG", new_cfg)

                        # just write DPLL mode to phase measurement again, trigger register
                        self.boards[index].dpll.modules["DPLL_Config"].write_field(1,
                                                                                   "DPLL_MODE", "PLL_MODE", 0x2)
                        self.boards[index].i2c.write_dpll_reg_direct(
                            0x8a2d, 0x0)  # channel 0
                        self.boards[index].i2c.write_dpll_reg_direct(
                            0x8b2d, 0x0)  # channel 1
                        self.boards[index].i2c.write_dpll_reg_direct(
                            0x8c2d, 0x0)  # channel 2
                        self.boards[index].i2c.write_dpll_reg_direct(
                            0x8d2d, 0x0)  # channel 3

                print(
                    f"Loop {j} board0_change={board0_change} , board1_change={board1_change}, diff={board0_change - board1_change}", flush=True)

            # compute slope
            x = list(range(num_per_slope))
            slope_0, _, _, _, _ = stats.linregress(
                x, results_board_0[-1*num_per_slope:])
            # slopes_board_0.append(slope_0)
            slope_1, _, _, _, _ = stats.linregress(
                x, results_board_1[-1*num_per_slope:])
            # slopes_board_1.append(slope_1)

            slope_difference = slope_0 - slope_1
            slope_differences.append(slope_difference)

            proportional = slope_difference * MiniPTM.PFM_KP
            integral += slope_difference * time_diff * MiniPTM.PFM_KI

            cur_ppm_adjust += proportional + integral

            fcw = calculate_fcw(cur_ppm_adjust)
            fcw = to_twos_complement_bytes(fcw, 42)
            print(
                f"{j} -> Slope_diff = {slope_difference} , Cur_ppm_adjust = {cur_ppm_adjust} , FCW change to {fcw}\n")
            # it's in the right order, just write it
            if (False):
                self.boards[1].dpll.modules["DPLL_Freq_Write"].write_reg_mul(3,
                                                                             "DPLL_WR_FREQ_7_0", fcw)
            # HACK FROM RENESAS, DISABLE DECIMATOR OF PFD
            self.boards[1].i2c.write_dpll_reg_direct(0x8a2d, 0x0)  # channel 0
            self.boards[1].i2c.write_dpll_reg_direct(0x8b2d, 0x0)  # channel 1
            self.boards[1].i2c.write_dpll_reg_direct(0x8c2d, 0x0)  # channel 2
            self.boards[1].i2c.write_dpll_reg_direct(0x8d2d, 0x0)  # channel 3

            self.check_user_input()
            time.sleep(2)
            self.check_user_input()

        print(f"Slope differences: {slope_differences}")
        print(f"Board 0 phase: {results_board_0}")
        print(f"Board 1 phase: {results_board_1}")

        # print(f"Results board 0: {results_board_0}")
        # print(f"Results board 1: {results_board_1}")

        # print(f"Slopes 0: {slopes_board_0}")
        # print(f"Slopes 1: {slopes_board_1}")


######################################################################################################
# DPLL over fiber

    # used as part of dpll over fiber

    def handle_query_response(self, board, query_response):
        print(f"Top board {board.board_num} query response {query_response}")
        decoder_num = query_response[0]
        query_id = query_response[1]
        query_data = query_response[2]
        print(f"Got query data {len(query_data)}")

        status_bytes = query_data[:16]
        dpll_statuses = query_data[16:20]
        input_freq_monitor_info = query_data[20:52]
        name_string = query_data[52:68]
        TOD_delta = query_data[68:79]  # TOD delta seen at far side
        # what remote side saw, 1 for local tod > incoming tod, 0 for local < incoming WRT remote side
        tod_flag = query_data[79]
        clock_quality = query_data[80]

        print(f"Board {board.board_num} handle query response {query_data}")
        # switch to it if not already
        cur_dpllmode = board.dpll.modules["DPLL_Config"].read_reg(3,
                                                                  "DPLL_MODE")
        cur_reference = board.dpll.modules["Status"].read_reg(0,
                                                              "DPLL3_REF_STATUS")

        if (clock_quality < board.best_clock_quality_seen):  # found a better clock
            if (cur_dpllmode != 0 or cur_reference != decoder_num):
                print(
                    f"Board {board.board_num} changing to track input {decoder_num} clock quality {clock_quality}!")
                # 0x11 is write frequency input, keep it in the list, that's the default software control
                board.setup_dpll_track_and_priority_list([decoder_num, 0x11])
                board.best_clock_quality_seen = clock_quality

                # reset this variable upon starting to track a clock
                board.tod_compare_count = 0

        if (clock_quality == board.best_clock_quality_seen):
            if (cur_dpllmode == 0 and cur_reference == decoder_num):
                # this query is for the decoder I'm tracking
                print(f"Board {board.board_num} got query for board tracking")

                if (board.tod_compare_count >= 10):
                    # give it 10 tods to align and do jumps, should be pretty stable after that
                    # compare what the far side sees and estimate round trip

                    print(f"Got 10 TODs, do round trip calculation")
                    half_round_trip = time_divide_by_value(TOD_delta, 2)

                    print(
                        f"TOD delta from query {TOD_delta} , half = {half_round_trip}")
                    board.round_trip_time = half_round_trip

                    if (tod_flag == 1):
                        # remote side saw it's TOD larger than mine
                        print(f"handle query response round trip far side larger")
                        board.round_trip_time = half_round_trip

                    elif (tod_flag == 0):
                        # remote side saw it's TOD smaller than mine somehow
                        print(
                            f"Handle query respond round trip far side smaller?????")

    def handle_tod_compare(self, board, tod_compare):
        # tod compare has a bunch of data
        decoder_num = tod_compare[0][0]
        remote_tod = tod_compare[0][1]
        local_tods = tod_compare[0][2:]

        # check if this board is in dpll mode tracking this decoder
        cur_dpllmode = board.dpll.modules["DPLL_Config"].read_reg(3,
                                                                  "DPLL_MODE")
        # assume DPLL3 is master, a bit hacky
        cur_reference = board.dpll.modules["Status"].read_reg(0,
                                                              "DPLL3_REF_STATUS")

        if (cur_dpllmode == 0x0 and cur_reference == decoder_num):

            if (hasattr(board, tod_compare_count)):
                if (board.tod_compare_count < 10):
                    return

            print(
                f"Handle tod compare board {board.board_num}, remote={remote_tod} , local={local_tods}")
            for tod in range(4):
                # do a TOD adjustment as well to try to align with this
                # 9 bytes, not upper two handshake bytes
                local_tod_received = local_tods[tod]
                board.adjust_tod(tod, local_tod_received, remote_tod_received)

            # store a variable in the board for my own purpose at higher level
            if hasattr(board, tod_compare_count):
                board.tod_compare_count += 1
            else:
                board.tod_compare_count = 1

    # THIS CODE WORKS!
    # The read and write are kinda messy, conflict resolution is not great

    def dpll_over_fiber_test(self):
        alternate_query_write_flag = 0
        for board in self.boards:
            board.init_pwm_dplloverfiber()

        time_between_queries = 15

        board_query_response = [[], []]
        board_tod_comparison = [[], []]
        time_board_query_response = [0, 0]
        loop_count = 0

        while (True):
            print(
                f"\n\n****** DPLL over fiber Top level loop number {loop_count} *******\n\n")
            for index, board in enumerate(self.boards):
                # Do periodic queries as necessary and possible
                time_debug = time.time() - time_board_query_response[index]
                tx_ready = self.boards[index].dpof.get_chan_tx_ready(0)
                print(f"Board{index} time {time_debug} , tx_ready={tx_ready}")
                if (((time.time() - time_board_query_response[index]) > time_between_queries) and
                        self.boards[index].dpof.get_chan_tx_ready(0)):

                    print(f"Board {board.board_num} start query chan 0")
                    self.boards[index].dpof.dpof_query(0, 0)
                    break

            # run all the dpll loops
            for board in self.boards:
                print(f"\n DPLL Over fiber loop board {board.board_num} \n")
                board.dpll_over_fiber_loop()

            # check the results from all the dpll over fiber loops
            for index, board in enumerate(self.boards):
                query_data = board.dpof.pop_query_data()
                if (len(query_data)):
                    # print(
                    #    f"Board {board.board_num} at top level, got query data {query_data}")
                    board_query_response[index].append(query_data)
                    time_board_query_response[index] = time.time()

                tod_compare_data = board.dpof.get_tod_compare()
                if (len(tod_compare_data) > 0):
                    print(
                        f"Board {board.board_num} at top level, got TOD comparison {tod_compare_data}")
                    board_tod_comparison[index].append(tod_compare_data)

                write_data = board.dpof.pop_write_data()
                if (len(write_data)):
                    print(
                        f"Board {board.board_num} at top level, got write data {write_data}")

            # now do something with the DPLL data
            for index, board in enumerate(self.boards):
                while len(board_tod_comparison[index]) > 0:
                    tod_compare = board_tod_comparison[index].pop(0)
                    self.handle_tod_compare(board, tod_compare)

                while len(board_query_response[index]) > 0:
                    response = board_query_response[index].pop(0)
                    self.handle_query_response(board, response)

            time.sleep(0.25)
            loop_count += 1


#############
# Simple proof of concept debug

    def debug_me(self):
        # Debug SFP connection, make sure I can swap between Phase measurement + DCO mode to DPLL tracking mode
        # assume SFP0 connected between board 0 and board 1
        self.boards[1].setup_dpll_track_and_priority_list([0])  # clk0

        print(f"Debug me, board 1 set clk0 as dpll track and priority")

    def debug_print(self):
        # debugging locking stuff
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(0)
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(1)
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(2)
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(3)
        print(f"\n\n************** BOARD 0 ***********************\n\n")
        self.boards[0].dpll.modules["Status"].print_all_registers(0)
        self.boards[0].dpll.modules["DPLL_Config"].print_all_registers(3)

        print(f"\n\n************** BOARD 1 ***********************\n\n")
        self.boards[1].dpll.modules["Status"].print_all_registers(0)
        self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(3)

        # clear all stickies
        for board in self.boards:
            board.i2c.write_dpll_reg_direct(0xc169, 0x1)



        # for board in self.boards:
        #    print(f"Board {board.board_num}")
        #    board.dpll.modules["Status"].print_register(0,
        #                                                "IN8_MON_FREQ_STATUS_0", True)
        #    board.dpll.modules["Status"].print_register(0,
        #                                                "IN8_MON_FREQ_STATUS_1", True)

        # for i in range(50):
        #    phase = self.boards[1].dpll.modules["Status"].read_reg_mul(0,
        #            "DPLL1_PHASE_STATUS_7_0", 5)
        #    print(f"Board 1 loop {i} phase status {phase}")

        #for board in self.boards:
        #    value = board.dpll.modules["Status"].read_reg_mul(0,
        #                                                      "DPLL3_FILTER_STATUS_7_0", 6)
        #    print(f"Board {board.board_num} FILTER STATUS = {value}")

    def close(self):
        pass

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="MiniPTM top level debug")

    parser.add_argument('command', type=str,
                        help='What command to run, program / debug_dpof / debug_pfm / flash / read / write')
    parser.add_argument('--config_file', type=str,
                        default="8A34002_MiniPTMV3_12-27-2023_Julian_AllPhaseMeas.tcs", help="File to program")
    parser.add_argument('--eeprom_file', type=str,
                        default="8A34002_MiniPTMV3_12-29-2023_Julian_AllPhaseMeas_EEPROM.hex", help="File to flash to EEPROM")
    parser.add_argument('--board_id', type=int, help="Board number to program")
    parser.add_argument("--reg_addr", type=str, default="0xc024",
                        help="Register address to read or write")
    parser.add_argument("--reg_val", type=str, default="0x0",
                        help="Register address to read or write")

    args = parser.parse_args()

    # simple user input handler in separate thread
    # input_thread = threading.Thread(target=wait_for_input, args=(MiniPTM.user_input_str,MiniPTM.user_input_lock))
    # input_thread.start()

    # stop_event = threading.Event()
    # input_thread = threading.Thread(
    #    target=input_thread_func, args=(stop_event,))
    # input_thread.start()

    if args.command == "program":
        top = MiniPTM()
        if args.board_id is not None:
            parsed_config_tcs = parse_dpll_tcs_config_file(args.config_file)
            top.program_one_board(top.boards[args.board_id], parsed_config_tcs)
        else:
            top.program_all_boards(config_file=args.config_file)

        top.set_all_boards_leds_idcode()
    elif args.command == "debug":
        top = MiniPTM()
        top.debug_me()
    elif args.command == "debug_sfp":
        top = MiniPTM()
        for board in top.boards:
            print(
                f"\n *********** SFP Debug Board {board.board_num} ***********\n")
            board.print_sfps_info()
    elif args.command == "debug_print":
        top = MiniPTM()
        top.debug_print()
    elif args.command == "debug_dpof":
        top = MiniPTM()
        top.dpll_over_fiber_test()

    elif args.command == "debug_pfm":
        top = MiniPTM()
        top.do_pfm_sacrifice_dpll3()
        # top.do_pfm_sacrifice_dpll3_use_output_tdc()

    elif args.command == "flash":
        top = MiniPTM()
        if args.board_id is not None:
            top.flash_eeprom_one_board(
                top.boards[args.board_id], args.eeprom_file)
        else:
            top.flash_all_boards_eeprom(args.eeprom_file)
    elif args.command == "read":
        top = MiniPTM()
        print(f"Starting read register")
        if args.board_id is not None:
            val = top.boards[args.board_id].i2c.read_dpll_reg_direct(
                int(args.reg_addr, 16))
            print(
                f"Board {args.board_id} Read Register {args.reg_addr} = 0x{val:02x}")
        else:
            for board in top.boards:
                val = board.i2c.read_dpll_reg_direct(int(args.reg_addr, 16))
                print(
                    f"Board {board.board_num} Read Register {args.reg_addr} = 0x{val:02x}")
    elif args.command == "write":
        top = MiniPTM()
        if args.board_id is not None:
            top.boards[args.board_id].i2c.write_dpll_reg_direct(
                int(args.reg_addr, 16), int(args.reg_val, 16))
            print(
                f"Board {args.board_id} Write Register {args.reg_addr} = {args.reg_val}")
        else:
            for board in top.boards:
                board.i2c.write_dpll_reg_direct(
                    int(args.reg_addr, 16), int(args.reg_val, 16))
                print(
                    f"Board {board.board_num} Write Register {args.reg_addr} = {args.reg_val}")
    else:
        print(f"Invalid command, must be program or debug")

    # stop_event.set()
    # input_thread.join()

    # PFM STUFF
    # top.print_all_pcie_clock_info()
    # top.do_pfm_use_input_tdc()

    # BIG REGISTER DUMP
    # top.print_all_full_dpll_status()
