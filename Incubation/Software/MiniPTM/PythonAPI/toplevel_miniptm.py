
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


class MiniPTM:
    user_input_str = []
    user_input_lock = threading.Lock()
    PFM_KP = 10
    PFM_KI = 0.2

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
                if ( len(val) == 0 ):
                    continue
                print(f"User entered {val}")
                if val == "quit":
                    quit()
                else:
                    parts = val.split()
                    print(f"User input in parts, {parts}")
                    if ( parts[0] == 'set' ):
                        if (parts[1] == "kp" ):
                            MiniPTM.PFM_KP = float(parts[2])
                        elif (parts[1] == "ki" ):
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

    def do_pfm_use_input_tdc(self):
        # Use Input TDC from DPLL1 , measure CLK8 (10MHz from PCIe 100MHz divided down)
        #   versus CLK5 (10MHz loopback from Q7 for zero delay)

        if (len(self.boards) <= 1):
            print("PFM only works with two+ boards, ending")
            return

        # simple algorithm, measure both phase values, and

        # hack , use first board as "reference"

        # doesn't quite work , since phase measurement value fluctuates
        results_board_0 = []

        results_board_1 = []
        cur_ppm_adjust = 0

        slope_differences = []
        num_per_slope = 2

        integral = 0

        # start of it , make sure frequency adjust word is zero
        self.boards[1].dpll.modules["DPLL_Freq_Write"].write_reg_mul(3,
                 "DPLL_WR_FREQ_7_0", [0, 0, 0, 0, 0, 0])

        print(f"Start pfm use input tdc")
        # reset it wth synth
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
                #print(f" Got first results, {results_first}")
                start_time = time.time()
                time.sleep(1)  # give it time to accumulate phase

                futures = [executor.submit(self.boards[j].read_pcie_clk_phase_measurement, False, 1)
                           for j in range(len(self.boards))]
                results_second = [future.result()
                                  for future in futures]  # get them in order
                #print(f" Got second results, {results_second}")

                end_time = time.time()
                time_diff = end_time - start_time
                self.check_user_input()

                results_board_0.append(
                    results_first[0] * 50e-12)  # in units of 50ps
                results_board_0.append(
                    results_second[0] * 50e-12)  # in units of 50ps

                print(f"Board 0 measurements {results_board_0[-2]},{results_board_0[-1]}")

                results_board_1.append(
                    results_first[1] * 50e-12)  # in units of 50ps
                results_board_1.append(
                    results_second[1] * 50e-12)  # in units of 50ps

                print(f"Board 1 measurements {results_board_1[-2]},{results_board_1[-1]}")

                board0_change = (
                    results_second[0] * 50e-12) - (results_first[0] * 50e-12)
                board1_change = (
                    results_second[1] * 50e-12) - (results_first[1] * 50e-12)

                # TDC can saturate, do a clear if either board is above a threshold, I'm using 0.5, half a second
                if ( results_board_0[-1] > 0.15 or results_board_1[-1] > 0.15):
                    futures = [executor.submit(self.boards[j].read_pcie_clk_phase_measurement, True, 1)
                               for j in range(len(self.boards))]
                    results_first = [future.result()
                                     for future in futures]  # get them in order

                
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
            self.boards[1].dpll.modules["DPLL_Freq_Write"].write_reg_mul(3,
                                                                         "DPLL_WR_FREQ_7_0", fcw)

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

    def do_dpll_over_fiber_with_ack(self):
        tx_0 = PWM_TX(self.boards[0], 0)
        tx_1 = PWM_TX(self.boards[1], 0)
        rx_0 = PWM_RX(self.boards[0])
        rx_1 = PWM_RX(self.boards[1])

        value = "Hello world"
        chunk_size = tx_0.MAX_PAYLOAD_SIZE
        chunks = [list(value.encode()[i:i + chunk_size])
                  for i in range(0, len(value), chunk_size)]
        chunk_count = 0

        rx_0.enable_port(0)
        rx_1.enable_port(0)

        rx_1_packet_content = []
        for loop_test in range(20):

            value = chunks[chunk_count]
            last_pkt = False
            if (chunk_count == len(chunks)-1):
                last_pkt = True
            chunk_count = (chunk_count + 1) % len(chunks)
            print("\n\n")
            hex_value = [hex(val) for val in value]
            print(f"Value {hex_value}, Step 1, Send from TX0")

            # loop to wait for TX to be ready
            for j in range(20):
                if (tx_0.check_has_tx_gone_out()):
                    break
                else:
                    time.sleep(0.1)

            tx_0.write_single_payload(value, not last_pkt)
            went_out = False
            for i in range(20):
                if (tx_0.check_has_tx_gone_out()):
                    went_out = True
                    break
                time.sleep(0.25)

            if not went_out:
                print(f" Step 1 TX didn't go out, end!")
                return

            print("\n\n")
            print(f"Value {value}, Step 2, has gone from TX0, check RX1")
            got_data = False
            rx_1_rcvd_header = 0
            for i in range(20):
                header, data = rx_1.read_packet()
                if len(header):
                    # got a packet, give it to TX stack in case it needs it
                    tx_1.got_incoming(header[0])

                    # got a packet, check data
                    rx_1_rcvd_header = header

                    hex_val = [hex(val) for val in data]
                    print(f"RX1 got packet data {hex_val}")

                    rx_1_packet_content += data
                    continue_flag = (header[0] >> 3) & 0x1
                    if (continue_flag == 0):
                        print(
                            f"\n\n*************** RX1 GOT FULL PAYLOAD {rx_1_packet_content} ********\n\n")

                    if data == value:
                        print(f"RX1 got ping, now send ack")
                        got_data = True
                        break
                time.sleep(0.25)

            if not got_data:
                print(f" Step 2 failed to get Data on RX1, end!")
                return

            print("\n\n")
            print(
                f"Value {value}, Step 3, has gone from TX0 to RX1, send ACK to TX0")
            # loop to wait for TX to be ready
            for j in range(20):
                if (tx_1.check_has_tx_gone_out()):
                    break
                else:
                    time.sleep(0.1)

            tx_1.write_ack_packet(rx_1_rcvd_header)
            went_out = False
            for i in range(20):
                if (tx_1.check_has_tx_gone_out()):
                    went_out = True
                    break
                time.sleep(0.25)

            if not went_out:
                print(f" Step 3 TX didn't go out, end!")
                return

            print("\n\n")
            print(
                f"Value {value}, Step 4, has gone from TX0 to RX1, TX1 sent ack, check on RX0")
            got_data = False
            for i in range(20):
                header, data = rx_0.read_packet()
                if len(header):
                    # got a packet, give it to TX stack in case it needs
                    tx_0.got_incoming(header[0])
                    if (tx_0.is_tx_idle()):
                        print(f"TX0 got ack from RX1")
                        got_data = True
                        break
                time.sleep(0.25)

            if not got_data:
                print(f" Step 4 failed to get ACK from RX1, end!")
                return

    # Demo simple tx / rx test here using low level DPLL over fiber classes

    def do_dpll_over_fiber_pingpong(self):
        tx_0 = PWM_TX(self.boards[0], 0)
        tx_1 = PWM_TX(self.boards[1], 0)
        rx_0 = PWM_RX(self.boards[0])
        rx_1 = PWM_RX(self.boards[1])

        values = [0xaa, 0xcc, 0xde, 0xba, 0x11]

        rx_0.enable_port(0)
        rx_1.enable_port(0)

        # simple ping pong loop forever
        # setup TX on TX0 and wait for RX1 to get it
        for loop_test in range(10):
            for value in values:

                print("\n\n")
                print(f"Value {value}, Step 1, Send from TX0")
                tx_0.write_single_payload([value], False)
                went_out = False
                for i in range(20):
                    if (tx_0.check_has_tx_gone_out()):
                        went_out = True
                        break
                    time.sleep(0.25)

                if not went_out:
                    print(f" Step 1 TX didn't go out, end!")
                    return

                print("\n\n")
                print(f"Value {value}, Step 2, has gone from TX0, check RX1")
                got_data = False
                for i in range(20):
                    header, data = rx_1.read_packet()
                    if len(header):
                        # got a packet, check data
                        hex_val = [hex(val) for val in data]
                        print(f"RX1 got packet data {hex_val}")
                        if data[0] == value:
                            print(f"RX1 got ping, now send pong")
                            got_data = True

                            # debug hack, continue to read RX1 for some time
                            # for j in range(20):
                            #    print(f"Debug RX1 read PWM incoming more {j}")
                            #    header, data = rx_1.read_packet()
                            #    time.sleep(0.25)
                            break
                    time.sleep(0.25)

                if not got_data:
                    print(f" Step 2 failed to get Data on RX1, end!")
                    return

                print("\n\n")
                print(
                    f"Value {value}, Step 3, has gone from TX0 to RX1, now send from TX1")
                tx_1.write_single_payload([value], False)
                went_out = False
                for i in range(20):
                    if (tx_1.check_has_tx_gone_out()):
                        went_out = True
                        break
                    time.sleep(0.25)

                if not went_out:
                    print(f" Step 3 failed, TX1 didn't send! End")
                    return

                print("\n\n")
                print(
                    f"Value {value}, Step 4, has gone from TX0 to RX1 out TX1, check RX0")
                got_data = False
                for i in range(20):
                    header, data = rx_0.read_packet()
                    if len(header):
                        # got a packet, check data
                        hex_val = [hex(val) for val in data]
                        print(f"RX0 got packet data {hex_val}")
                        if data[0] == value:
                            print(f"RX0 got pong!")
                            got_data = True
                            # debug hack, continue to read RX0 for some time
                            # for j in range(20):
                            #    print(f"Debug RX0 read PWM incoming more {j}")
                            #    header, data = rx_0.read_packet()
                            #    time.sleep(0.25)
                            break
                    time.sleep(0.25)

                if not got_data:
                    print(f" Step 4 failed! RX0 Didn't receive, end!")
                    return

    def debug_dpll_over_fiber(self):

        # TOD debug, write it and read it back over and over
        self.boards[0].write_tod_absolute(0, 0, 0, 0xffcc00000000)
        self.boards[1].write_tod_absolute(0, 0, 0, 0xffaa00000000)

        # disable all decoders
        for board in self.boards:
            for i in range(len(board.dpll.modules["PWMDecoder"].BASE_ADDRESSES)):
                board.dpll.modules["PWMDecoder"].write_field(i,
                                                             "PWM_DECODER_CMD", "ENABLE", 0)
            # enable just decoder 0
            board.dpll.modules["PWMDecoder"].write_field(0,
                                                         "PWM_DECODER_CMD", "ENABLE", 1)

        values = [0xaa, 0xcc, 0xde, 0xba, 0x11]
        for index, value in enumerate(values):
            zero_val = (0xff << (5*8)) + (value << (4*8))
            one_val = (0xff << (5*8)) + \
                (values[len(values) - 1 - index] << (4*8))
            self.boards[0].write_tod_absolute(0, 0, 0, zero_val)
            self.boards[1].write_tod_absolute(0, 0, 0, one_val)

            for i in range(10):
                print(f"\n Debug dpll over fiber loop {i} \n")
                for board in self.boards:

                    #  read TOD immediately
                    board.dpll.modules["TODReadPrimary"].write_reg(0,
                                                                   "TOD_READ_PRIMARY_CMD", 0x0)  # single shot, immediate
                    board.dpll.modules["TODReadPrimary"].write_reg(0,
                                                                   "TOD_READ_PRIMARY_CMD", 0x1)  # single shot, immediate

                    # this works
                    # self.boards[0].dpll.modules["TODReadPrimary"].print_all_registers(0)

                    # this works now as well
                    read_count = board.dpll.modules["TODReadPrimary"].read_reg(0,
                                                                               "TOD_READ_PRIMARY_COUNTER")
                    cur_tod = board.dpll.modules["TODReadPrimary"].read_reg_mul(0,
                                                                                "TOD_READ_PRIMARY_SECONDS_0_7", 6)
                    hex_val = [hex(val) for val in cur_tod]
                    print(
                        f"Board {board.board_num} Read read_count {read_count} cur_tod {hex_val} <-> {cur_tod}")

                    cur_pwm = board.i2c.read_dpll_reg_multiple(0xce80, 0x5, 6)
                    cur_pwm.reverse()
                    hex_val = [hex(val) for val in cur_pwm]
                    print(f"Board {board.board_num} current pwm {hex_val}")
                time.sleep(0.25)

        # for i in range(len(board.dpll.modules["PWMEncoder"].BASE_ADDRESSES)):
        #    board.dpll.modules["PWMEncoder"].print_all_registers(i)
        # for i in range(len(board.dpll.modules["PWMDecoder"].BASE_ADDRESSES)):
        #    board.dpll.modules["PWMDecoder"].print_all_registers(i)
        # print(f"\n************** Board {board.board_num} SFP Status *************\n")
        # board.print_sfps_info()
        # print(
        #    f"\n************** Board {board.board_num} PWM Status *************\n")
        # board.print_pwm_channel_status()
        # print(
        #    f"\n************** Board {board.board_num} Init DPOF **************\n")
        # board.init_pwm_dplloverfiber()

        # print(
        #    f"\n************** Board {board.board_num} Print PWM Config *******\n")
        # board.dpll.modules["PWMEncoder"].print_all_registers(0)
        # board.dpll.modules["PWMDecoder"].print_all_registers(0)

    # THIS CODE WORKS!
    # The read and write are kinda messy, conflict resolution is not great

    def dpll_over_fiber_test(self):
        alternate_query_write_flag = 0
        for board in self.boards:
            board.init_pwm_dplloverfiber()

        for i in range(100):
            # constant stimulus as possible,
            # alternate between query on channel 0 query ID 0 and write

            if (self.boards[0].dpof.get_chan_tx_ready(0)):
                if (alternate_query_write_flag == 0):
                    print(f"Starting send query!")
                    self.boards[0].dpof.dpof_query(0, 0)
                    alternate_query_write_flag = 1
                else:
                    print(f"Starting write data!")
                    # 0x1 is write
                    self.boards[0].dpof.dpof_write(
                        0, 1, [0xde, 0xad, 0xbe, 0xef])
                    alternate_query_write_flag = 0

            for board in self.boards:
                print(f"\n DPLL Over fiber loop board {board.board_num} \n")
                board.dpll_over_fiber_loop()
                query_data = board.dpof.pop_query_data()
                if (len(query_data)):
                    print(
                        f"Board {board.board_num} at top level, got query data {query_data}")
                write_data = board.dpof.pop_write_data()
                if (len(write_data)):
                    print(
                        f"Board {board.board_num} at top level, got write data {write_data}")
                tod_compare_data = board.dpof.get_tod_compare()
                if (len(tod_compare_data) > 0):
                    print(
                        f"Board {board.board_num} at top level, got TOD comparison {tod_compare_data}")

            time.sleep(0.25)
            print(
                f"\n\n****** DPLL over fiber Top level loop number {i} *******\n\n")

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

    stop_event = threading.Event()
    input_thread = threading.Thread(
        target=input_thread_func, args=(stop_event,))
    input_thread.start()

    if args.command == "program":
        top = MiniPTM()
        if args.board_id is not None:
            parsed_config_tcs = parse_dpll_tcs_config_file(args.config_file)
            top.program_one_board(top.boards[args.board_id], parsed_config_tcs)
        else:
            top.program_all_boards(config_file=args.config_file)

        top.set_all_boards_leds_idcode()
    elif args.command == "debug_dpof":
        top = MiniPTM()
        top.dpll_over_fiber_test()

        # top.debug_dpll_over_fiber()
        # top.print_all_full_dpll_status()
        # DPLL over fiber stuff
        # top.do_dpll_over_fiber_pingpong() -> basic proof of concept
        # top.do_dpll_over_fiber_with_ack() -> Another proof of concept

    elif args.command == "debug_pfm":
        top = MiniPTM()
        top.do_pfm_use_input_tdc()

    elif args.command == "flash":
        top = MiniPTM()
        if args.board_id is not None:
            top.flash_eeprom_one_board(
                top.boards[args.board_id], args.eeprom_file)
        else:
            top.flash_all_boards_eeprom(args.eeprom_file)
    elif args.command == "read":
        top = MiniPTM()
        if args.board_id is not None:
            val = top.boards[args.board_id].i2c.read_dpll_reg_direct(
                int(args.reg_addr, 16))
            val = int(val, 16)
            print(
                f"Board {args.board_id} Read Register {args.reg_addr:02x} = {val:02x}")
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

    stop_event.set()
    input_thread.join()

    # PFM STUFF
    # top.print_all_pcie_clock_info()
    # top.do_pfm_use_input_tdc()

    # BIG REGISTER DUMP
    # top.print_all_full_dpll_status()
