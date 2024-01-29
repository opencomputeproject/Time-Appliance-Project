
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
        #print(f"Got {len(miniptm_devs)} mini ptm devices: {miniptm_devs}")

        i2c_busses = find_i2c_buses("MiniPTM I2C Adapter")
        #print(f"Matching i2c buses: {i2c_busses}")

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

        print(f"Board {board.board_num} handle query response {query_data}, tod_delta = {TOD_delta}")
        # switch to it if not already
        cur_dpllmode = board.dpll.modules["DPLL_Config"].read_reg(2,
                                                                  "DPLL_MODE")
        cur_reference = board.dpll.modules["Status"].read_reg(0,
                                                              "DPLL2_REF_STATUS")

        if (clock_quality < board.best_clock_quality_seen):  # found a better clock
            if (cur_dpllmode != 0 or cur_reference != decoder_num):
                print(
                    f"Board {board.board_num} changing to track input {decoder_num} clock quality {clock_quality}!")
                #setup all three channels
                board.setup_dpll_track_and_priority_list(0,[decoder_num])
                board.setup_dpll_track_and_priority_list(1,[decoder_num])
                board.setup_dpll_track_and_priority_list(2,[decoder_num])
                board.dpof.inform_new_master(decoder_num)
                time.sleep(0.1)
                board.best_clock_quality_seen = clock_quality

                # reset this variable upon starting to track a clock
                board.tod_compare_count = 0
                board.tod_average_count = 0

        if (clock_quality == board.best_clock_quality_seen):
            if (cur_dpllmode == 0 and cur_reference == decoder_num):
                # this query is for the decoder I'm tracking
                print(f"Board {board.board_num} got query data for board tracking")


    def handle_tod_compare(self, board, tod_compare):
        # tod compare has a bunch of data
        decoder_num = tod_compare[0][0]
        remote_tod = tod_compare[0][1]
        local_tods = tod_compare[0][2:]


        # check if this board is in dpll mode tracking this decoder
        cur_dpllmode = board.dpll.modules["DPLL_Config"].read_reg(2,
                                                                  "DPLL_MODE")
        # assume DPLL2 is master, a bit hacky
        cur_reference = board.dpll.modules["Status"].read_reg(0,
                                                              "DPLL2_REF_STATUS")

        # store a variable in the board for my own purpose at higher level
        if not hasattr(board, "tod_average_count"):
            board.tod_average_count = 0
        if not hasattr(board, "tod_compare_count"):
            board.tod_compare_count = 0

        print(f"Board {board.board_num} Top level handle tod compare dpllmode={cur_dpllmode}, cur_ref = {cur_reference}, decoder_num={decoder_num}")
        if (cur_dpllmode == 0x0 and cur_reference == decoder_num):

            print(
                f"Handle tod compare board {board.board_num} count {board.tod_compare_count}, remote={remote_tod} , local={local_tods}")

            # hack to the code, skip the first one
            if ( board.tod_compare_count == 0 ):
                print("HACK skip first tod value")
                board.tod_compare_count += 1
                return
            # add to average
            for tod in range(3):
                # do a TOD adjustment as well to try to align with this
                # 9 bytes, not upper two handshake bytes
                local_tod_received = local_tods[tod]
                # keep an average for each TOD
                board.dpof.add_to_average_tod_error(tod, 
                        local_tod_received, remote_tod, True)
            board.tod_average_count += 1

            if ( (board.tod_average_count % 5) == 0 ): # how many samples to average
                for index in range(1):
                    avg_error = board.dpof.get_average_tod_error(index, True)
                    print(f"Board {board.board_num} channel {index} average tod error {avg_error}")
                    # avg_error is a signed nanosecond value
                    # decoder already included in the values that go into the average
                    board.dpof.adjust_tod_signed_nanoseconds(index, avg_error)
                board.tod_average_count = 0

            if ( board.tod_compare_count >= 5 ):
                # give it a few tods to align and do jumps, should be pretty stable after that
                # tell master that I'm now a slave

                print(f"Did TOD Compare count more than enough, tell master I'm following {decoder_num}")
                board.dpof.start_follow_far_side(decoder_num)

            board.tod_compare_count += 1
        else:
            # this will be the master side
            print(f"Board {board.board_num} TOD compare data, remote_tod={remote_tod}, local={local_tods}, align TOD0")
            # hack
            # align all my tods with TOD 0
            if ( board.tod_compare_count >= 5 ):
                # just do this alignment once
                return
            for tod in range(1,4):
                to_follow_tod = local_tods[0]
                to_discipline_tod = local_tods[tod]
                # keep an average
                count = board.dpof.average_tod_errors[tod].get_count()
                print(f"Board {board.board_num} Add to average tod {tod} count {count}, to_disc={to_discipline_tod}, to_follow={to_follow_tod}")
                board.dpof.add_to_average_tod_error(tod,
                        to_discipline_tod, to_follow_tod, False)
               
            board.tod_average_count += 1
            if ( (board.tod_average_count % 5) == 0 ): # how many samples to average
                for index in range(1,4):
                    avg_error = board.dpof.get_average_tod_error(index, True)
                    print(f"Board {board.board_num} channel {index} average tod error {avg_error}")
                    # avg_error is a signed nanosecond value
                    # decoder already included in the values that go into the average
                    board.dpof.adjust_tod_signed_nanoseconds(index, avg_error)
                board.tod_average_count = 0
               
            board.tod_compare_count += 1
            # compare TOD0 with incoming
            tod_diff, flag = time_difference_with_flag(local_tods[0], remote_tod, True)
            print(f"Board {board.board_num} incoming TOD diff {tod_diff} flag {flag}")



    # THIS CODE WORKS!
    # The read and write are kinda messy, conflict resolution is not great

    def dpll_over_fiber_test(self):
        alternate_query_write_flag = 0
        for board in self.boards:
            board.init_pwm_dplloverfiber()

        time_between_queries = 45

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
                #print(f"\n DPLL Over fiber loop board {board.board_num} \n")
                board.dpll_over_fiber_loop()

            # check the results from all the dpll over fiber loops
            for index, board in enumerate(self.boards):
                query_data = board.dpof.pop_query_data()
                if (len(query_data)):
                    print(f"Board {board.board_num} at top level, got query data {query_data}")
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
                    print(f"Got board {index} tod comparison")
                    tod_compare = board_tod_comparison[index].pop(0)
                    self.handle_tod_compare(board, tod_compare)

                while len(board_query_response[index]) > 0:
                    print(f"Got board {index} query response")
                    response = board_query_response[index].pop(0)
                    self.handle_query_response(board, response)


            # check for channels where acting as master
            for index, board in enumerate(self.boards):
                master_channels = board.dpof.get_channels_following()
                if len(master_channels):
                    print(f"Board {index} has master channels {master_channels}")


            time.sleep(0.25)
            loop_count += 1


#############
# Simple proof of concept debug


    def debug_me_small(self):
        # hack, shift outputs
        diff_ns = (100 * 1000 * 1000) // 2
        diff_ns_total = diff_ns
        for i in range(20):
            diff_ns_fod_bytes = to_twos_complement_bytes(diff_ns_total, 32)

            print(f"Slave adjusting output FODs {diff_ns} nanoseconds, {diff_ns_fod_bytes}")
            self.boards[0].dpll.modules["Output"].write_reg_mul(1, "OUT_PHASE_ADJ_7_0", diff_ns_fod_bytes)
            self.boards[0].dpll.modules["Output"].write_reg_mul(3, "OUT_PHASE_ADJ_7_0", diff_ns_fod_bytes)
            self.boards[0].dpll.modules["Output"].write_reg_mul(5, "OUT_PHASE_ADJ_7_0", diff_ns_fod_bytes)

            diff_ns_total += diff_ns
            time.sleep(10)

    def debug_me_tod(self):
        board_num = 0
        tod = 0
        self.boards[board_num].dpll.modules["TODReadSecondary"].write_reg(tod,
                "TOD_READ_SECONDARY_CMD", 0x0)
        #get current count to see when it increments
        start_count = self.boards[board_num].dpll.modules["TODReadSecondary"].read_reg(tod,
                "TOD_READ_SECONDARY_COUNTER")
        self.boards[board_num].dpll.modules["TODReadSecondary"].write_field(tod,
                "TOD_READ_SECONDARY_SEL_CFG_0", "REF_INDEX", 13)
        self.boards[board_num].dpll.modules["TODReadSecondary"].write_field(tod,
                "TOD_READ_SECONDARY_SEL_CFG_0", "PWM_DECODER_INDEX", 0)
        # enable read secondary on CLK5 continous
        self.boards[board_num].dpll.modules["TODReadSecondary"].write_reg(tod,
                "TOD_READ_SECONDARY_CMD", 0x13)

        cur_count = start_count
        for i in range(20):
            while ( start_count == cur_count ):
                cur_count = self.boards[board_num].dpll.modules["TODReadSecondary"].read_reg(tod,
                        "TOD_READ_SECONDARY_COUNTER")
                time.sleep(0.1)

            print(f"Start count = {start_count}, cur_count = {cur_count}")
            cur_tod = self.boards[board_num].dpll.modules["TODReadSecondary"].read_reg_mul(tod,
                    "TOD_READ_SECONDARY_SUBNS", 11)
            print(f"Got new TOD: {cur_tod}")
            start_count = cur_count
                
        self.boards[board_num].dpll.modules["TODReadSecondary"].write_reg(tod,
                "TOD_READ_SECONDARY_CMD", 0)



    def debug_me_old1(self):

        # Step 1. On master board (1) , align TOD0 with CLK5
        # disable any triggers
        board = self.boards[1]
        for i in range(5):
            print(f"\nStart debug, align TOD0 on board {board.board_num}")
            board.dpll.modules["TODReadSecondary"].write_reg(0,
                    "TOD_READ_SECONDARY_CMD", 0x0)
            board.dpll.modules["TODReadSecondary"].write_reg(3,
                    "TOD_READ_SECONDARY_CMD", 0x0)

            #get current count to see when it increments
            start_count = board.dpll.modules["TODReadSecondary"].read_reg(0,
                    "TOD_READ_SECONDARY_COUNTER")

            board.dpll.modules["TODReadPrimary"].write_field(0, 
                    "TOD_READ_PRIMARY_SEL_CFG_0", "PWM_DECODER_INDEX", 5)
            board.dpll.modules["TODReadPrimary"].write_field(3, 
                    "TOD_READ_PRIMARY_SEL_CFG_0", "PWM_DECODER_INDEX", 5)

            # enable read secondary on CLK5 single shot
            board.dpll.modules["TODReadSecondary"].write_reg(0,
                    "TOD_READ_SECONDARY_CMD", 0x4) #0x4 decoder, 0x3 clock
            board.dpll.modules["TODReadSecondary"].write_reg(3,
                    "TOD_READ_SECONDARY_CMD", 0x4) #0x4 decoder, 0x3 clock

            while True:
                cur_count = board.dpll.modules["TODReadSecondary"].read_reg(0,
                        "TOD_READ_SECONDARY_COUNTER")

                if ( start_count != cur_count ):
                    break
                time.sleep(0.1)

            # got new TOD trigger
            tod0 = board.dpll.modules["TODReadSecondary"].read_reg_mul(0,
                    "TOD_READ_SECONDARY_SUBNS", 11)
            tod3 = board.dpll.modules["TODReadSecondary"].read_reg_mul(3,
                    "TOD_READ_SECONDARY_SUBNS", 11)
            print(f"TOD0 = {tod0} , ideal_tod = {tod3}")

            board.dpof.adjust_tod(0, tod0, tod3, True)

            time.sleep(1)


        return
        # Step 2. On slave board (0)
        # Trigger TOD0 Secondary on CLK13
        # Trigger TOD0 Primary on CLK0 Decoder
        # Adjust Outputs 1/3/5 OUT_PHASE_ADJ to make Secondary closer to primary
        # second value doesn't matter, just sub second

        print(f"\nDebug step2, align slave PPS to incoming PWM PPS")
        dpll = self.boards[0].dpll
        
        #disable triggers
        dpll.modules["TODReadSecondary"].write_reg(2, "TOD_READ_SECONDARY_CMD", 0x0)
        dpll.modules["TODReadPrimary"].write_reg(2, "TOD_READ_PRIMARY_CMD", 0x0)

        start_sec_cnt = dpll.modules["TODReadSecondary"].read_reg(2, "TOD_READ_SECONDARY_COUNTER")
        start_pri_cnt = dpll.modules["TODReadPrimary"].read_reg(2, "TOD_READ_PRIMARY_COUNTER")

        dpll.modules["TODReadSecondary"].write_field(2, "TOD_READ_SECONDARY_SEL_CFG_0", "REF_INDEX", 13)
        dpll.modules["TODReadPrimary"].write_field(2, "TOD_READ_PRIMARY_SEL_CFG_0", "PWM_DECODER_INDEX", 0)

        # enable triggers
        dpll.modules["TODReadSecondary"].write_reg(2, "TOD_READ_SECONDARY_CMD", 0x3) #clk input
        dpll.modules["TODReadPrimary"].write_reg(2, "TOD_READ_PRIMARY_CMD", 0x4) # decoder input
        while True:
            sec_cnt = dpll.modules["TODReadSecondary"].read_reg(2, "TOD_READ_SECONDARY_COUNTER")
            pri_cnt = dpll.modules["TODReadPrimary"].read_reg(2, "TOD_READ_PRIMARY_COUNTER")

            if ( start_sec_cnt != sec_cnt and start_pri_cnt != pri_cnt ):
                print(f"Slave board count changes, {start_sec_cnt} -> {sec_cnt}, {start_pri_cnt} -> {pri_cnt}")
                break
            else:
                print(f"Start_sec = {start_sec_cnt} , cur = {sec_cnt}, Start pri = {start_pri_cnt} , cur = {pri_cnt}")
            time.sleep(0.1)

        print(f"Slave board got primary and secondary TOD0 read triggers")
        
        sec_tod = dpll.modules["TODReadSecondary"].read_reg_mul(2, "TOD_READ_SECONDARY_SUBNS", 11)
        pri_tod = dpll.modules["TODReadPrimary"].read_reg_mul(2, "TOD_READ_PRIMARY_SUBNS", 11)

        hex_sec = [hex(val) for val in sec_tod]
        hex_pri = [hex(val) for val in pri_tod]
        print(f"Slave read secondary TOD {hex_sec}, primary TOD {hex_pri}") 

        sec_ns = time_to_nanoseconds(sec_tod)
        pri_ns = time_to_nanoseconds(pri_tod) - ( ( 118 * ( 1 / 25e6 ) ) * 1e9 ) # decoder delay


        diff_ns = sec_ns - pri_ns # how many nanoseconds difference need to adjust output phase

        if ( diff_ns > 0 ):
            diff_ns = diff_ns % 1000000000 # keep only sub seconds
            if ( diff_ns > 0.5e9 ):
                diff_ns = 1e9 - diff_ns
        else:
            diff_ns = abs(diff_ns) % 1000000000
            diff_ns *= -1
            if ( diff_ns < 0.5e9 ):
                diff_ns = 1e9 + diff_ns

        print(f"Secondary ns {sec_ns}, Primary ns {pri_ns}, diff_ns = {diff_ns}")

        FOD_CLK_PER_NS = 2 # 500MHz FODs, just hard coding for now
        diff_ns_fod = int(diff_ns / FOD_CLK_PER_NS) # how much to add to output phase adjust register

        # read back current value, this is a relative shift
        cur_shift = dpll.modules["Output"].read_reg_mul(1, "OUT_PHASE_ADJ_7_0", 4)
        cur_shift_val = 0
        for index, val in enumerate(cur_shift):
            cur_shift_val += val << (8*index)

        cur_shift_val = int_to_signed_nbit(cur_shift_val, 32) 

        print(f"Read current shift {cur_shift} = {cur_shift_val}")
        cur_shift_val = (cur_shift_val + diff_ns_fod) 
        if ( cur_shift_val > 250 * 1000 * 1000 ):
            cur_shift_val = 0.5e9 - cur_shift_val
        print(f"Adjust current shift {cur_shift_val}")



        diff_ns_fod_bytes = to_twos_complement_bytes(cur_shift_val, 32)
        print(f"Slave adjusting output FODs {diff_ns} nanoseconds, {diff_ns_fod_bytes}")
        #dpll.modules["Output"].write_reg_mul(1, "OUT_PHASE_ADJ_7_0", diff_ns_fod_bytes)
        #dpll.modules["Output"].write_reg_mul(3, "OUT_PHASE_ADJ_7_0", diff_ns_fod_bytes)
        #dpll.modules["Output"].write_reg_mul(5, "OUT_PHASE_ADJ_7_0", diff_ns_fod_bytes)





    def debug_me_tod_sync(self):


        board_id = 1
        #self.boards[board_id].setup_dpll_track_and_priority_list([0])  # clk0
        #print(f"Debug me, board {board_id} set clk0 as dpll track and priority")

        # debug PPS output from TOD


        # enable TOD synchronization for DPLLs 0 / 1, assume rest of TOD sync already configed
        # to sync to TOD2
        self.boards[board_id].dpll.modules["DPLL_Config"].write_field(0, 
                "DPLL_TOD_SYNC_CFG", "TOD_SYNC_EN", 1)
        self.boards[board_id].dpll.modules["DPLL_Config"].write_field(1, 
                "DPLL_TOD_SYNC_CFG", "TOD_SYNC_EN", 1)
        self.boards[board_id].dpll.modules["DPLL_Config"].write_field(2, 
                "DPLL_TOD_SYNC_CFG", "TOD_SYNC_EN", 1)

        for i in range(1):

            # read output tdc phase
            

            # disable output TDC prior to large jump
            for j in range(2):
                phase_val = self.boards[board_id].dpll.modules["Status"].read_reg_mul(0,
                        f"OUTPUT_TDC{j}_MEASUREMENT_7_0", 6)
                print(f"Output TDC{j} phase = {phase_val}")

                # filter=en, align=reset/clear, continous, alignment, disabled
                self.boards[board_id].dpll.modules["OUTPUT_TDC"].write_reg(j,
                        "OUTPUT_TDC_CTRL_4", 0x8e)

            # disable output tdc globally, experiment
            self.boards[board_id].dpll.modules["OUTPUT_TDC_CFG"].write_reg(0,
                    "OUTPUT_TDC_CFG_GBL_2", 0x2)

            # write TOD with TOD sync output enabled and aligning channel0/1 to TOD2
            if ( i % 2 ):
                self.boards[board_id].dpof.write_tod_relative(2, 0, 1000000, 0, True)
            else:
                self.boards[board_id].dpof.write_tod_relative(2, 0, 1000000, 0, False)


            # when doing large TOD shifts on the slave side, the output frequency unlocks
            # for some time due to output TDC
            
            time.sleep(4) # some time for output phase to go through


            return

            # disable output TOD synchronization on channels 0 / 1
            print(f"Disable output TOD synchronization")
            self.boards[board_id].dpll.modules["DPLL_Config"].write_field(0, 
                    "DPLL_TOD_SYNC_CFG", "TOD_SYNC_EN", 0)
            self.boards[board_id].dpll.modules["DPLL_Config"].write_field(1, 
                    "DPLL_TOD_SYNC_CFG", "TOD_SYNC_EN", 0)
            self.boards[board_id].dpll.modules["DPLL_Config"].write_field(2, 
                    "DPLL_TOD_SYNC_CFG", "TOD_SYNC_EN", 0)


            print(f"Restart output TDCs")
            # restart the TDC
            self.boards[board_id].dpll.modules["OUTPUT_TDC_CFG"].write_reg(0,
                    "OUTPUT_TDC_CFG_GBL_2", 0x3)
            time.sleep(1)
            for j in range(2):
                phase_val = self.boards[board_id].dpll.modules["Status"].read_reg_mul(0,
                        f"OUTPUT_TDC{j}_MEASUREMENT_7_0", 6)
                print(f"Output TDC{j} phase = {phase_val}")
                # filter=en, align=reset/clear, continous, alignment, enabled
                self.boards[board_id].dpll.modules["OUTPUT_TDC"].write_reg(j,
                        "OUTPUT_TDC_CTRL_4", 0x8f)


            start_time = time.time()
            for j in range(1000):

                """
                # DPLLs are considered basically free-running since combo slaves to channel 2
                # nothing to do with DPLL status
                all_dplls_locked = True

                for dpll_index in range(3):
                    dpll_status = self.boards[board_id].dpll.modules["Status"].read_field(0,
                            f"DPLL{dpll_index}_STATUS", "DPLL_STATE")
                    print(f"DPLL{dpll_index} state after tod shift 0x{dpll_status:02x}")
                    if ( dpll_status != 0x3 ):
                        all_dplls_locked = False

                if ( all_dplls_locked ):
                    break
                """

                # check output TDC status
                all_tdcs_valid = True
                for tdc_index in range(2):
                    phase_val = self.boards[board_id].dpll.modules["Status"].read_reg_mul(0,
                            f"OUTPUT_TDC{tdc_index}_MEASUREMENT_7_0", 6)
                    tdc_status = self.boards[board_id].dpll.modules["Status"].read_reg(0,
                            f"OUTPUT_TDC{tdc_index}_STATUS")
                    print(f"Output tdc{tdc_index} status after tod shift 0x{tdc_status:02x}, phase = {phase_val}")
                    if ( tdc_status != 1 ):
                        all_tdcs_valid = False

                if ( all_tdcs_valid ):
                    break

                time.sleep(0.25)
            end_time = time.time()
            print(f"Time to lock after large TOD shift: {end_time - start_time}")
                
            time.sleep(3)

        return


        # debug TOD , just read it back
        for i in range(50):
            print(f"\n Debug TOD print {i} \n")
            for board in self.boards:
                tod_num = 0
                board.dpll.modules["TODReadPrimary"].write_field(
                    tod_num, "TOD_READ_PRIMARY_CMD", "TOD_READ_TRIGGER", 0x0)
                board.dpll.modules["TODReadPrimary"].write_reg(tod_num,
                    "TOD_READ_PRIMARY_CMD", 0x1)

                cur_tod = board.dpll.modules["TODReadPrimary"].read_reg_mul(tod_num,
                     "TOD_READ_PRIMARY_SUBNS", 11)

                print(f"Board {board.board_num} TOD0 = {cur_tod}")

                print(f"Board {board.board_num} add 20 seconds to TOD")
                board.dpof.write_tod_relative(0, 0, 0, 20, True)

            time.sleep(0.25)





    def debug_me_fine(self):
        # just the fine adjustment part after start
        print(f"Fine alignment start")


        # on master, use input TDC. 
        # Channel 2 Q5 -> CLK13
        # Any phase measurement block is fine, just use Channel 0
        # assume high precision TDC, FILTER_STATUS
        # THINK ABOUT THIS, if apply on master, it will shift measurement outputs too
        # does this matter?
        diff_ns = 0
        for i in range(20):

            # hack, if I adjust master loopback path, do I see input TDC change?? -> YES
            #diff_ns = 5 * i
            #diff_ns = diff_ns // 2
            #diff_ns_fod = diff_ns // 2 # 500MHz FOD, 2ns period, put into units of 2ns
            #diff_ns_fod_bytes = to_twos_complement_bytes( int(diff_ns_fod) , 32)
            #self.boards[1].dpll.modules["Output"].write_reg_mul(5, 
            #        "OUT_PHASE_ADJ_7_0", diff_ns_fod_bytes)


            # need to restart the input phase measurement, input TDC is continous measurement
            self.boards[1].dpll.modules["DPLL_Config"].write_reg(0,
                    "DPLL_MODE", 0x28)

            tdc_freq = 625e6
            tdc_period = (1/tdc_freq)/32 # in seconds
            print("")

            #fast_phase_meas = self.boards[1].dpll.modules["Status"].read_reg_mul(0,
            #        "DPLL0_PHASE_STATUS_7_0", 5)
            #fast_phase_meas_val = 0
            #for index,val in enumerate(fast_phase_meas):
            #    fast_phase_meas_val += val << (8*index)
            #print(f"Raw phase meas val {fast_phase_meas_val}")
            #fast_phase_meas_val = int_to_signed_nbit(fast_phase_meas_val, 36)
            #fast_phase_meas_val_sec = fast_phase_meas_val * tdc_period
            #print(f"Signed raw phase meas val {fast_phase_meas_val}, sec = {fast_phase_meas_val_sec}")

            phase_meas = self.boards[1].dpll.modules["Status"].read_reg_mul(0,
                    "DPLL0_FILTER_STATUS_7_0", 6)
            print(f"Read input TDC filter status {phase_meas}")

            phase_meas_val = 0
            for index,val in enumerate(phase_meas):
                phase_meas_val += val << (8*index)
            print(f"Raw phase meas val {phase_meas_val}")
            phase_meas_val = int_to_signed_nbit(phase_meas_val, 48)

            print(f"Signed raw phase meas val {phase_meas_val}")
            # this is in ITDC_UI / 128 units
            phase_meas_val_sec = phase_meas_val * ( tdc_period / 128 )
            

            # feedback = CLK0, from slave
            # reference = CLK13, loopback on board
            # a positive sign means feedback clock leads reference clock
            #   need to fall back Q0
            # a negative sign means feedback clock lags reference clock
            #   need to step forward Q0

            # just feed this into out phase adjustment for debug
            phase_meas_val_ns =  int( (phase_meas_val_sec * 1e9) / 2 )
            print(f"Phase meas val sec = {phase_meas_val_sec}, ns to apply {phase_meas_val_ns}")

            time.sleep(0.25)

            continue
            # coarse phase adjustment sucks but it does work mostly
            self.boards[1].add_output_phase_offset(0, phase_meas_val_ns)

            # FINE_PHASE_ADVANCE has max value of 0x3fff, not great
            # keep the math here but not used
            # use this as error value , adjust FINE_PHASE_ADVANCE
            # fine_phase_advance in units of TDC period / 4096
            phase_adj_resolution = tdc_period / 4096 # in seconds
            #print(f"Phase_meas_val={phase_meas_val}, sec={phase_meas_val_sec}")
            # get how much to adjust
            adj_val = int( phase_meas_val_sec / phase_adj_resolution )
            #print(f"Phase adj resolution {phase_adj_resolution}, adj_val = {adj_val}")

            # DPLL_PHASE_OFFSET_CFG , does this work?
            # units of ITDC_UI
            # this works, but it's a constant phase offset
            # if you leave it , it will continue to shift the outputs
            # do a fraction of error like proportional value, then clear it
            # divide by two because this is round trip value
            dpll_phase_offset_val = int( ( (phase_meas_val * 128) / 2 ) * 0.7 )
            meas_clk_per = 1/(25e6) # seconds
            dpll_phase_offset_bytes = to_twos_complement_bytes(dpll_phase_offset_val, 36)
            print(f"Doing DPLL PHASE OFFSET CFG val {dpll_phase_offset_val} -> {dpll_phase_offset_bytes}")
            self.boards[1].dpll.modules["DPLL_Ctrl"].write_reg_mul(0,
                    "DPLL_PHASE_OFFSET_CFG_7_0", dpll_phase_offset_bytes)
            time.sleep(0.1)
            self.boards[1].dpll.modules["DPLL_Ctrl"].write_reg_mul(0,
                    "DPLL_PHASE_OFFSET_CFG_7_0", [0]*6 )

            time.sleep(0.1)

    def debug_me_coarse(self):
        # Step 1. On master board (1), align all outputs with write to TOD2
        # doesn't need to be any value, just a TOD write triggers alignment
        #re-enable output sync

        # clear any previous output phase adjustment
        for i in range(6):
            self.boards[1].dpll.modules["Output"].write_reg_mul(i, 
                    "OUT_PHASE_ADJ_7_0", [0]*4)

        # make sure slave frame sync mode is enabled
        for i in range(3):
            self.boards[0].dpll.modules["DPLL_Config"].write_field(i,
                    "DPLL_CTRL_2", "FRAME_SYNC_MODE", 1)

        # force sync master board using TOD sync
        self.boards[1].dpll.modules["DPLL_Config"].write_field(0, "DPLL_TOD_SYNC_CFG", "TOD_SYNC_EN", 1)
        self.boards[1].dpll.modules["DPLL_Config"].write_field(2, "DPLL_TOD_SYNC_CFG", "TOD_SYNC_EN", 1)
        self.boards[1].dpll.modules["DPLL_Config"].write_reg(0, "DPLL_MODE", 0x28)
        self.boards[1].dpll.modules["DPLL_Config"].write_reg(2, "DPLL_MODE", 0x28) #trigger registers
        
        self.boards[1].dpof.write_tod_relative(2, 0, 0, 0, False)


        time.sleep(2)
        # step 2. wait some time for this shift to propagate and slave to align
        # probably need a good way to check when slave has properly aligned to frame
        # sync pulse, dpll status maybe
        # dpll status goes to lockrec then back to locked then still takes a few cycles
        # before output PPS values recover
        for i in range(200):
            dpll_status = self.boards[0].dpll.modules["Status"].read_reg(0,
                    "DPLL1_STATUS")
            print(f"After master TOD shift alignment, slave DPLL1 status = 0x{dpll_status:02x}")
            time.sleep(0.1)

        
        # Step 3. On slave board (0)
        # trigger read of TOD0 with PWM Decoder 0 PPS
        # TOD0 because that's the SFP channel I'm using in my setup
        for i in range(2):
            dpll = self.boards[0].dpll
            dpll.modules["TODReadSecondary"].write_reg(0, "TOD_READ_SECONDARY_CMD", 0x0)
            start_sec_cnt = dpll.modules["TODReadSecondary"].read_reg(0, 
                    "TOD_READ_SECONDARY_COUNTER")
            dpll.modules["TODReadSecondary"].write_field(0, 
                    "TOD_READ_SECONDARY_SEL_CFG_0", "PWM_DECODER_INDEX", 0)
            dpll.modules["TODReadSecondary"].write_reg(0, 
                    "TOD_READ_SECONDARY_CMD", 0x4) # 0x4 for decoder input

            while True:
                sec_cnt = dpll.modules["TODReadSecondary"].read_reg(0, 
                        "TOD_READ_SECONDARY_COUNTER")
                if ( start_sec_cnt != sec_cnt ):
                    print(f"Slave board count changes, {start_sec_cnt} -> {sec_cnt}")
                    break
                else:
                    print(f"Slave board {start_sec_cnt}, {sec_cnt}")
                    pass
                time.sleep(0.1)

            # read secondary TOD0 value
            sec_tod = dpll.modules["TODReadSecondary"].read_reg_mul(0, 
                    "TOD_READ_SECONDARY_SUBNS", 11)

            # read the PWM frame received from PWM FIFO
            rcvd_tod = self.boards[0].i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)

            # adjust TOD0 to match received TOD
            print(f"Slave board TOD0 on trigger {sec_tod} , PWM FIFO frame {rcvd_tod}")

            self.boards[0].dpof.adjust_tod(0, sec_tod, rcvd_tod, True)

            # give this some time
            time.sleep(4)



        # Step 4. Round trip on master
        # now read back TOD2 as triggered by PWM Decoder 0 on master board (1)
        # also read back TOD1 , discipline to this
        board = self.boards[1]
        dpll = self.boards[1].dpll

        dpll.modules["TODReadSecondary"].write_reg(2, "TOD_READ_SECONDARY_CMD", 0x0)
        start_sec_cnt = dpll.modules["TODReadSecondary"].read_reg(2, 
                "TOD_READ_SECONDARY_COUNTER")
        dpll.modules["TODReadSecondary"].write_field(2, 
                "TOD_READ_SECONDARY_SEL_CFG_0", "PWM_DECODER_INDEX", 0)
        dpll.modules["TODReadSecondary"].write_reg(2, 
                "TOD_READ_SECONDARY_CMD", 0x4) # 0x4 for decoder input


        dpll.modules["TODReadSecondary"].write_reg(1, "TOD_READ_SECONDARY_CMD", 0x0)
        dpll.modules["TODReadSecondary"].write_field(1, 
                "TOD_READ_SECONDARY_SEL_CFG_0", "PWM_DECODER_INDEX", 0)
        dpll.modules["TODReadSecondary"].write_reg(1, 
                "TOD_READ_SECONDARY_CMD", 0x4) # 0x4 for decoder input

        while True:
            sec_cnt = dpll.modules["TODReadSecondary"].read_reg(2, 
                    "TOD_READ_SECONDARY_COUNTER")
            if ( start_sec_cnt != sec_cnt ):
                print(f"Master board count changes, {start_sec_cnt} -> {sec_cnt}")
                break
            else:
                #print(f"Master board {start_sec_cnt}, {sec_cnt}")
                pass
            time.sleep(0.1)

        # read secondary TOD0 value
        sec_tod = dpll.modules["TODReadSecondary"].read_reg_mul(2, 
                "TOD_READ_SECONDARY_SUBNS", 11)

        # read the PWM frame got back as well
        rcvd_tod = self.boards[1].i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)

        # also discipline TOD1 with this on master, for when master channel 1 is tracking slave
        tod1 = dpll.modules["TODReadSecondary"].read_reg_mul(1,
                "TOD_READ_SECONDARY_SUBNS", 11)

        self.boards[1].dpof.adjust_tod(1, tod1, rcvd_tod, False)

        # calculate difference 
        sec_tod_ns = time_to_nanoseconds(sec_tod)
        rcvd_tod_ns = time_to_nanoseconds(rcvd_tod)
        
        # include decoder difference in sec_tod_ns 
        diff_ns = -1 * int( sec_tod_ns - rcvd_tod_ns - ( ( 118 * ( 1/25e6 ) ) * 1e9 ) )
        if ( diff_ns > 1e9 ):
            while ( diff_ns > 1e9 ):
                diff_ns -= 1e9
        if ( diff_ns < -1e9 ):
            while ( diff_ns < -1e9):
                diff_ns += 1e9

       
        print(f"Master saw TOD round trip approximately {sec_tod}, rcvd_tod = {rcvd_tod}, diff={diff_ns}")
    
        # Disable master TOD output synchronization now, decoupling outputs from TOD
        # dont think thsi is necessary, not how TOD alignment works
        print(f"Disable master TOD sync output enable, alignment complete")
        self.boards[1].dpll.modules["DPLL_Config"].write_field(0, 
                "DPLL_TOD_SYNC_CFG", "TOD_SYNC_EN", 0)
        self.boards[1].dpll.modules["DPLL_Config"].write_field(2, 
                "DPLL_TOD_SYNC_CFG", "TOD_SYNC_EN", 0)

        # now apply half this round trip master and slave will follow 
        # Potential options:
        # 1. DPLL_PHASE_OFFSET_CFG
        # 2. FINE_PHASE_ADVANCE
        # 3. OUT_PHASE_ADJ 
       
        # only apply the shift on the PWM carrier to slave and feedback path
        # since this measurement is with TOD, and resolution is low-ish
        # use out_phase_adj, should give enough
        #### THIS CODE DOESNT WORK WELL UNLESS FRESH PROGRAM
        #### NEEDS TO BE RELATIVE ADJUST TO CURRENT VALUE OF OUTPUT PHASE ADJUST
        diff_ns = diff_ns // 2 # half for round trip
        print(f"Writing output phase adjustment on master {diff_ns}")
        self.boards[1].add_output_phase_offset(0, diff_ns)
        self.boards[1].add_output_phase_offset(5, diff_ns)


        # give this time to propagate
        time.sleep(2)

        # disable slave side frame sync mode now, pps is aligned within
        # 1 cycle of PWM carrier, don't want the jitter of decoder
        # I want to track the incoming PWM carrier phase now
        print(f"Decoupling slave from PWM PPS, coarse alignment complete")
        for i in range(3):
            self.boards[0].dpll.modules["DPLL_Config"].write_field(i,
                    "DPLL_CTRL_2", "FRAME_SYNC_MODE", 0)
            self.boards[0].dpll.modules["DPLL_Config"].write_reg(i,
                    "DPLL_MODE", 0)


        return


    def debug_master_frame_sync(self):
        # board 1 (master) channel 1 should be locking to CLK0 from board 0 Q0
        # but for some reason the frame pulse out of Q3 is not aligned
        # what do I change to move Q3 on the master board?


        # idea 5: turn off frame alignment mode on slave DPLLs
        # and reset dpll1 on master
        self.boards[0].dpll.modules["DPLL_Config"].write_field(0,
                "DPLL_CTRL_2", "FRAME_SYNC_MODE", 0)
        self.boards[0].dpll.modules["DPLL_Config"].write_field(1,
                "DPLL_CTRL_2", "FRAME_SYNC_MODE", 0)
        self.boards[0].dpll.modules["DPLL_Config"].write_field(2,
                "DPLL_CTRL_2", "FRAME_SYNC_MODE", 0)
        # write trigger register as well
        self.boards[0].dpll.modules["DPLL_Config"].write_reg(0,
                "DPLL_MODE", 0)
        self.boards[0].dpll.modules["DPLL_Config"].write_reg(1,
                "DPLL_MODE", 0)
        self.boards[0].dpll.modules["DPLL_Config"].write_reg(2,
                "DPLL_MODE", 0)



        self.boards[1].dpll.modules["PWMDecoder"].write_field(0,
                "PWM_DECODER_CMD", "ENABLE", 0)
        # turn off dpll sync mode as well
        self.boards[1].dpll.modules["DPLL_Config"].write_field(1,
                "DPLL_CTRL_2", "FRAME_SYNC_MODE", 0)

        self.boards[1].dpll.modules["DPLL_Config"].write_field(1,
                "DPLL_MODE", "PLL_MODE", 0x0) # write trigger register

        time.sleep(10)

        self.boards[1].dpll.modules["PWMDecoder"].write_field(0,
                "PWM_DECODER_CMD", "ENABLE", 1)

        self.boards[1].dpll.modules["DPLL_Config"].write_field(1,
                "DPLL_CTRL_2", "FRAME_SYNC_MODE", 1)

        self.boards[1].dpll.modules["DPLL_Config"].write_field(1,
                "DPLL_MODE", "PLL_MODE", 0x0) # PLL mode

        self.boards[1].dpll.modules["DPLL_Ctrl"].write_reg(1,
                "DPLL_FRAME_PULSE_SYNC", 1)
        return


        # idea 4: debug, try reading TOD2 based on PWM decoder 0 PPS -> looks normal, wtf
        for i in range(10):
            board = self.boards[1]
            dpll = self.boards[1].dpll
            dpll.modules["TODReadSecondary"].write_reg(2, "TOD_READ_SECONDARY_CMD", 0x0)
            start_sec_cnt = dpll.modules["TODReadSecondary"].read_reg(2, 
                    "TOD_READ_SECONDARY_COUNTER")
            dpll.modules["TODReadSecondary"].write_field(2, 
                    "TOD_READ_SECONDARY_SEL_CFG_0", "PWM_DECODER_INDEX", 0)
            dpll.modules["TODReadSecondary"].write_reg(2, 
                    "TOD_READ_SECONDARY_CMD", 0x4) # 0x4 for decoder input

            while True:
                sec_cnt = dpll.modules["TODReadSecondary"].read_reg(2, 
                        "TOD_READ_SECONDARY_COUNTER")
                if ( start_sec_cnt != sec_cnt ):
                    print(f"Master board count changes, {start_sec_cnt} -> {sec_cnt}")
                    break
                else:
                    #print(f"Master board {start_sec_cnt}, {sec_cnt}")
                    pass
                time.sleep(0.1)

            # read secondary TOD0 value
            sec_tod = dpll.modules["TODReadSecondary"].read_reg_mul(2, 
                    "TOD_READ_SECONDARY_SUBNS", 11)

            # read the PWM frame got back as well
            rcvd_tod = self.boards[1].i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)

            # calculate difference 
            sec_tod_ns = time_to_nanoseconds(sec_tod)
            rcvd_tod_ns = time_to_nanoseconds(rcvd_tod)
            diff_ns = sec_tod_ns - rcvd_tod_ns

            print(f"Master saw TOD round trip approximately {sec_tod}, rcvd_tod = {rcvd_tod}, diff={diff_ns}")

        return

        # idea 3: turn off PWM decoder on master and turn it back on -> NOPE
        self.boards[1].dpll.modules["PWMDecoder"].write_field(0,
                "PWM_DECODER_CMD", "ENABLE", 0)

        # turn off dpll sync mode as well
        self.boards[1].dpll.modules["DPLL_Config"].write_field(1,
                "DPLL_CTRL_2", "FRAME_SYNC_MODE", 0)

        time.sleep(2)

        self.boards[1].dpll.modules["PWMDecoder"].write_field(0,
                "PWM_DECODER_CMD", "ENABLE", 1)

        self.boards[1].dpll.modules["DPLL_Config"].write_field(1,
                "DPLL_CTRL_2", "FRAME_SYNC_MODE", 1)


        return

        # Idea 1: Adjust TOD on slave board -> NOPE, doesn't do anything
        for i in range(10):
            self.boards[0].dpof.adjust_tod_signed_nanoseconds(0, 50 * 1000 * 1000, False)
            self.boards[0].dpof.adjust_tod_signed_nanoseconds(1, 50 * 1000 * 1000, False)
            self.boards[0].dpof.adjust_tod_signed_nanoseconds(2, 50 * 1000 * 1000, False)
            self.boards[0].dpof.adjust_tod_signed_nanoseconds(3, 50 * 1000 * 1000, False)
            time.sleep(5)

        return

        # Idea 2: Adjust TOD on master board -> it works but also screws up slave pulses
        for i in range(10):
            self.boards[1].dpof.adjust_tod_signed_nanoseconds(0, 50 * 1000 * 1000, False)
            self.boards[1].dpof.adjust_tod_signed_nanoseconds(1, 50 * 1000 * 1000, False)
            self.boards[1].dpof.adjust_tod_signed_nanoseconds(2, 50 * 1000 * 1000, False)
            self.boards[1].dpof.adjust_tod_signed_nanoseconds(3, 50 * 1000 * 1000, False)
            time.sleep(5)



    def debug_adjust_method(self):




        # fine phase advance, what does it do?
        # this works, but can only do 0 - 3 nanoseconds, only for very fine adjustment
        # works really good for this range though, very high resolution
        # it affects SMA output on v3 board as well, make sure to enable another sma out
        # its almost too fine I can barely see difference on scope
        # DPLL_Ctrl, every register is trigger register
        cur_val = self.boards[1].dpll.modules["DPLL_Ctrl"].read_reg_mul(0,
                "DPLL_FINE_PHASE_ADV_CFG_7_0", 2)
        print(f"Debug adjust method, current fine phase adjustment = {cur_val}")

        #self.boards[1].dpll.modules["DPLL_Ctrl"].write_reg_mul(0,
        #   "DPLL_FINE_PHASE_ADV_CFG_7_0", [0xff, 0x1f])
        self.boards[1].dpll.modules["DPLL_Ctrl"].write_reg_mul(0,
               "DPLL_FINE_PHASE_ADV_CFG_7_0", [0x0, 0x0])

       
        # dpll_phase_offset_cfg test
        # same as I saw before, it's a constant offset
        # so in phase measurement mode, it will continue to move with this
        print(f"Writing dpll phase offset cfg")
        self.boards[1].dpll.modules["DPLL_Ctrl"].write_reg_mul(0,
                "DPLL_PHASE_OFFSET_CFG_7_0", [0] * 6)
        #self.boards[1].dpll.modules["DPLL_Ctrl"].write_reg_mul(0,
        #        "DPLL_PHASE_OFFSET_CFG_7_0", [0xff,0xff] + [0] * 4)






    def debug_frame_sync_trigger_from_second_chan(self):
        print(f"Trying to fix frame sync on master loopback")

        # toggle output
        #self.boards[1].dpll.modules["Output"].write_reg(0,
        #        "OUT_CTRL_1", 0x4) # output squelch enable

        #time.sleep(1)

        #self.boards[1].dpll.modules["Output"].write_reg(0,
        #        "OUT_CTRL_1", 0x24) # output squelch disable

        #time.sleep(5)

        # frame pulse sync does not work!
        #self.boards[1].dpll.modules["DPLL_Ctrl"].write_reg(1,
        #        "DPLL_FRAME_PULSE_SYNC", 1)

        self.boards[1].dpll.modules["Input"].write_reg(0,
                "INPUT_IN_MODE", 0x30) #disabled

        time.sleep(1)

        # just squelch CLK0 on master and enable it back
        self.boards[1].dpll.modules["Input"].write_reg(0,
                "INPUT_IN_MODE", 0x31) #normal setting


    def debug_me_frame_sync(self):

        # Do everything with 1pps measurements, TOD not necessary
        # 1. Wait for round trip PPS measurement to stabilize
        #       A.  Q7 -> CLK5 loopback on master is the "golden" 1PPS
        #           Sync Channel 0 with Channel 3 with TOD sync one time in beginning
        #       B.  Q5 -> CLK13 loopback on master is the round trip 1PPS based on 
        #           frame sync and slave in constant resync mode
        #
        # Channel 0 = Phase measurement mode, CLK13 vs CLK5 , round trip measurement

        slave_num = 1
        master_num = 0

        # Debug step. Master, Sync channel 0 / 1 / 2 with write to TOD0
        self.boards[master_num].dpof.write_tod_absolute(0)

        time.sleep(10)
        self.boards[slave_num].dpll.modules["DPLL_Ctrl"].write_reg(0,
                "DPLL_FRAME_PULSE_SYNC", 0x1)


        return



        # Real step 1. 
        # On the slave, wait for DPLL to lock to master for a few seconds
        # just clear sticky bit and make sure it stays locked and no sticky change
        stable_count = 0
        stable_count_pass = 100
        while True: 
            dpll_status = self.boards[slave_num].dpll.modules["Status"].read_reg(0,
                    "DPLL0_STATUS")
            if ( (dpll_status & 0xf) == 0x3 ):
                if ( dpll_status & 0x10 ):
                    #lock status change
                    stable_count = 0
                    # clear lock status
                    self.boards[slave_num].i2c.write_dpll_reg_direct(0xc164 + 0x2,
                            0x1)
                else:
                    stable_count = stable_count + 1

            if ( stable_count >= stable_count_pass ):
                print("Slave DPLL0 is stable!")
                break
            else:
                print(f"Waiting for DPLL0 stable, status=0x{dpll_status:02x}, {stable_count}")
                time.sleep(0.1)

        time.sleep(1) # give it a grace period even

        # Real step 2.
        # On Slave, Trigger TOD0 (or whichever TOD is clocked by master) with slave frame sync output loopback
        # For MiniPTMv3, this means read TOD0 with Q5 -> CLK13 loopback path
        # manually force frame resync, then read TOD trigger 
        # wait for the difference to be exactly 1 second for a number of times
        # then disable frame sync mode, and make sure its still 1 second
        # wrote a helper function for this

        frame_sync_stable = self.boards[slave_num].wait_for_frame_sync_loopback_stable(0,
                [0,2], 13, 20)

        if ( not frame_sync_stable ):
            print(f"Slave could not stablize frame sync!")
            return
        else:
            print(f"Slave stabilized frame sync loopback!")
       
        return
        tod_vals = []
        count = 0
        good_count = 0
        good_count_threshold = 5

        dpll = self.boards[slave_num].dpll
        board = self.boards[slave_num]
        # force resync on slave channel 0 and channel 2
        dpll.modules["DPLL_Ctrl"].write_reg(0, 
                "DPLL_FRAME_PULSE_SYNC", 0x1)
        dpll.modules["DPLL_Ctrl"].write_reg(2, 
                "DPLL_FRAME_PULSE_SYNC", 0x1)
        while True:
            # setup TOD
            tod_val = board.get_tod_trigger_from_pps(0, True, False, 13)
            if not len(tod_val):
                continue # didnt get a valid reading, go again

            # got a valid reading
            tod_ns = time_to_nanoseconds(tod_val)
            print(f"Count {count}, After force resync on slave, TOD0 = {tod_ns}")
            count += 1
            tod_vals.append(tod_ns)
            if len(tod_vals) > 2:
                diff = tod_vals[-1] - tod_vals[-2]
                print(f"Difference: {diff}")
                if ( diff == 1e9 ):
                    # difference is exactly 1e9 nanoseconds, as it should be
                    good_count += 1

            if ( good_count >= good_count_threshold ):
                # seen at least some number of exact 1e9 nanosecond tod values
                # disable frame sync mode on both channels
                print(f"Slave saw 5 exact 1e9 frame sync pulses, trying disable frame sync")
                dpll.modules["DPLL_Ctrl"].write_field(0,
                        "DPLL_CTRL_2", "FRAME_SYNC_MODE", 0)
                dpll.modules["DPLL_Ctrl"].write_field(2,
                        "DPLL_CTRL_2", "FRAME_SYNC_MODE", 0)
                 
                # check tod again
                tod_val = board.get_tod_trigger_from_pps(0, True, False, 13)
                tod_ns = time_to_nanoseconds(tod_val)
                tod_diff = tod_ns - tod_vals[-2]
                if not len(tod_val) or tod_diff != 1e9:
                    print(f"After frame sync disable, didn't get 1e9 tiemstamp, restart")
                    dpll.modules["DPLL_Ctrl"].write_field(0,
                            "DPLL_CTRL_2", "FRAME_SYNC_MODE", 1)
                    dpll.modules["DPLL_Ctrl"].write_field(2,
                            "DPLL_CTRL_2", "FRAME_SYNC_MODE", 1)
                    dpll.modules["DPLL_Ctrl"].write_reg(0, 
                            "DPLL_FRAME_PULSE_SYNC", 0x1)
                    dpll.modules["DPLL_Ctrl"].write_reg(2, 
                            "DPLL_FRAME_PULSE_SYNC", 0x1)
                    good_count = 0
                    tod_vals = []
                    count = 0
                else:
                    print(f"After frame sync disable, got 1e9 timestamp, slave frame sync good!")
                    break




            # Real step 3.
            # Slave frame sync pulse is aligned and stable
            # On master, do same procedure













        # Clear on Read phase measurement from channel 0, wait for it to stabilize 

        phase_vals = []
        stable_length_required = 20
        while True:
           phase = self.boards[master_num].dpll.modules["Status"].read_reg_mul(0,
                   "DPLL0_PHASE_STATUS_7_0", 5)
           filt_phase = self.boards[master_num].dpll.modules["Status"].read_reg_mul(0,
                   "DPLL0_FILTER_STATUS_7_0", 6)
           print(f"Master phase status {phase}, filter = {filt_phase}")
           phase_val = 0
           for i, byte in enumerate(phase):
               phase_val += byte << (8*i)
           phase_val_ns = (int_to_signed_nbit(phase_val, 36) * ( 50e-12 )) * 1e9
           phase_vals.append(phase_val_ns)
           print(f"Phase_val_ns = {phase_val_ns}")

           # take last 10 values, if its stable to within 20ns it's good
           abs_diff = max(phase_vals) - min(phase_vals)
           if ( len(phase_vals) == stable_length_required and abs_diff <= 20 ):
               print(f"Phase measurement stabilized, good, history {phase_vals}")
               break
           
           phase_vals = phase_vals[-1*(stable_length_required-1):]
           
           time.sleep(0.5)


        return

    def debug_frame_sync_working(self):


        slave_num = 1
        master_num = 0
        
        # Debug step. Master, Sync channel 0 / 1 / 2 with write to TOD0
        self.boards[master_num].dpof.write_tod_absolute(0)

        print(f"Waiting for master PWM encoders to set to new TOD")

        for i in range(30):
            # read the PWM frame received from PWM FIFO
            rcvd_tod = self.boards[slave_num].i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)
            print(f"Count {i} slave received tod {rcvd_tod}")
            time.sleep(1)


        print(f"Discipling slave to master")
        for i in range(2):
            dpll = self.boards[slave_num].dpll
            dpll.modules["TODReadSecondary"].write_reg(0, "TOD_READ_SECONDARY_CMD", 0x0)
            start_sec_cnt = dpll.modules["TODReadSecondary"].read_reg(0, 
                    "TOD_READ_SECONDARY_COUNTER")
            dpll.modules["TODReadSecondary"].write_field(0, 
                    "TOD_READ_SECONDARY_SEL_CFG_0", "PWM_DECODER_INDEX", 0)
            dpll.modules["TODReadSecondary"].write_reg(0, 
                    "TOD_READ_SECONDARY_CMD", 0x4) # 0x4 for decoder input

            while True:
                sec_cnt = dpll.modules["TODReadSecondary"].read_reg(0, 
                        "TOD_READ_SECONDARY_COUNTER")
                if ( start_sec_cnt != sec_cnt ):
                    print(f"Slave board count changes, {start_sec_cnt} -> {sec_cnt}")
                    break
                else:
                    print(f"Slave board {start_sec_cnt}, {sec_cnt}")
                    pass
                time.sleep(0.1)

            # read secondary TOD0 value
            sec_tod = dpll.modules["TODReadSecondary"].read_reg_mul(0, 
                    "TOD_READ_SECONDARY_SUBNS", 11)

            # read the PWM frame received from PWM FIFO
            rcvd_tod = self.boards[slave_num].i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)

            # adjust TOD0 to match received TOD
            print(f"Slave board TOD0 on trigger {sec_tod} , PWM FIFO frame {rcvd_tod}")

            self.boards[slave_num].dpof.adjust_tod(0, sec_tod, rcvd_tod, True)

            # give this some time
            print("Slave wait for encoder to pick up new time")
            for j in range(25):
                # read the PWM frame received from PWM FIFO
                rcvd_tod = self.boards[master_num].i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)
                print(f"Count {j} Master received PWM TOD: {rcvd_tod}")

                time.sleep(1)

        # TOD0 on slave is disciplined to master
        # Step 1b. Read what master TOD is getting back
        # compared to its own TOD at the same point
        # using alternate PPS, its always the same TOD as the channel
        # so here TOD0

        for test_num in range(5):
            board = self.boards[master_num]
            dpll = self.boards[master_num].dpll

            for tod_num in range(4):

                dpll.modules["TODReadSecondary"].write_reg(tod_num, "TOD_READ_SECONDARY_CMD", 0x0)
                start_sec_cnt = dpll.modules["TODReadSecondary"].read_reg(tod_num, 
                        "TOD_READ_SECONDARY_COUNTER")
                dpll.modules["TODReadSecondary"].write_field(tod_num, 
                        "TOD_READ_SECONDARY_SEL_CFG_0", "PWM_DECODER_INDEX", 0)
                dpll.modules["TODReadSecondary"].write_reg(tod_num, 
                        "TOD_READ_SECONDARY_CMD", 0x4) # 0x4 for decoder input

            # just need to check one counter
            start_sec_cnt = dpll.modules["TODReadSecondary"].read_reg(0, 
                    "TOD_READ_SECONDARY_COUNTER")


            while True:
                sec_cnt = dpll.modules["TODReadSecondary"].read_reg(0, 
                        "TOD_READ_SECONDARY_COUNTER")
                if ( start_sec_cnt != sec_cnt ):
                    print(f"Master board count changes, {start_sec_cnt} -> {sec_cnt}")
                    break
                else:
                    #print(f"Master board {start_sec_cnt}, {sec_cnt}")
                    pass
                time.sleep(0.1)

            # read secondary TOD0 value
            sec_tod = dpll.modules["TODReadSecondary"].read_reg_mul(0, 
                    "TOD_READ_SECONDARY_SUBNS", 11)

            for tod_num in range(4):
                sec_tod = dpll.modules["TODReadSecondary"].read_reg_mul(tod_num, 
                        "TOD_READ_SECONDARY_SUBNS", 11)
                print(f"TOD{tod_num} master read back {sec_tod}")



            # read the PWM frame got back as well
            rcvd_tod = self.boards[master_num].i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)

            # calculate difference 
            sec_tod_ns = time_to_nanoseconds(sec_tod)
            rcvd_tod_ns = time_to_nanoseconds(rcvd_tod)
            
            # include decoder difference in sec_tod_ns 
            diff_ns = -1 * int( sec_tod_ns - rcvd_tod_ns - ( ( 118 * ( 1/25e6 ) ) * 1e9 ) )
            if ( diff_ns > 1e9 ):
                while ( diff_ns > 1e9 ):
                    diff_ns -= 1e9
            if ( diff_ns < -1e9 ):
                while ( diff_ns < -1e9):
                    diff_ns += 1e9

           
            print(f"Master saw TOD round trip approximately {sec_tod}, rcvd_tod = {rcvd_tod}, diff={diff_ns}")





    def debug_tod_both_boards(self):
        master_num = 0
        slave_num = 0
        for i in range(100):
            master_rcvd_tod = self.boards[master_num].i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)
            slave_rcvd_tod = self.boards[slave_num].i2c.read_dpll_reg_multiple(0xce80, 0x0, 11)
            print(f"Debug TOD loop {i}, master={master_rcvd_tod}, slave={slave_rcvd_tod}")
            time.sleep(0.4)


    def debug_me(self):

        self.debug_tod_both_boards()
        return

        self.debug_frame_sync_working()
        return

        self.debug_me_frame_sync()
        return


        self.debug_me_coarse()
        #time.sleep(10)
        #self.debug_master_frame_sync()
        #self.debug_adjust_method()
        #self.debug_me_fine()



    def debug_print(self):
        # debugging locking stuff
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(0)
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(1)
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(2)
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(3)
        print(f"\n\n************** BOARD 0 ***********************\n\n")
        self.boards[0].dpll.modules["Status"].print_all_registers(0)
        self.boards[0].dpll.modules["DPLL_Config"].print_all_registers(2)

        print(f"\n\n************** BOARD 1 ***********************\n\n")
        self.boards[1].dpll.modules["Status"].print_all_registers(0)
        self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(2)

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
                        default="8A34002_MiniPTMV3_1-17-2024_Julian_PFM_PMOS_Master_EEPROM.hex", help="File to flash to EEPROM")
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
