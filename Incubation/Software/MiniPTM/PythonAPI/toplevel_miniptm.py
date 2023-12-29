
from pcie_miniptm import get_miniptm_devices
from i2c_miniptm import find_i2c_buses
from board_miniptm import Single_MiniPTM
from renesas_cm_configfiles import *
import concurrent.futures
import time
import argparse


class MiniPTM:
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
        for i in range(100):
            with concurrent.futures.ThreadPoolExecutor() as executor:
                futures = [executor.submit(self.boards[i].read_pcie_clk_phase_measurement, False)
                           for i in range(len(self.boards))]

                results = [future.result()
                           for future in futures]  # get them in order
                for result in results:
                    print(result)
                print(results[0] - results[1])

            time.sleep(0.5)
            continue

            phase_ref = self.boards[0].read_pcie_clk_phase_measurement(False)
            print(f"Phase_ref = {phase_ref}")
            phase_meas = []
            for board in self.boards[1:]:
                phase_meas.append(board.read_pcie_clk_phase_measurement(False))

            print(f"Phase measurements = {phase_meas}")
            phase_err = [x - phase_ref for x in phase_meas]
            print(f"Measured phase error {phase_err}")
            time.sleep(0.5)

    def do_dpll_over_fiber(self):
        for board in self.boards:
            # print(f"\n************** Board {board.board_num} SFP Status *************\n")
            # board.print_sfps_info()
            print(
                f"\n************** Board {board.board_num} PWM Status *************\n")
            board.print_pwm_channel_status()
            print(
                f"\n************** Board {board.board_num} Init DPOF **************\n")
            board.init_pwm_dplloverfiber()
            print(
                f"\n************** Board {board.board_num} Print PWM Config *******\n")
            if (board.board_num == 0):
                board.pwm_switch_listen_channel(0)
            board.dpll.modules["PWMEncoder"].print_all_registers(0)
            board.dpll.modules["PWMDecoder"].print_all_registers(0)

        # hard code just for bring-up
        for i in range(10):
            for j in range(1):
                self.boards[j].read_pwm_tod_incoming()
            time.sleep(0.25)

    def close(self):
        pass

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="MiniPTM top level debug")

    parser.add_argument('command', type=str,
                        help='What command to run, program / debug / flash')
    parser.add_argument('--config_file', type=str,
                        default="8A34002_MiniPTMV3_12-27-2023_Julian_AllPhaseMeas.tcs", help="File to program")
    parser.add_argument('--eeprom_file', type=str,
                        default="8A34002_MiniPTMV3_12-29-2023_Julian_AllPhaseMeas_EEPROM.hex", help="File to flash to EEPROM")
    parser.add_argument('--board_id', type=int, help="Board number to program")

    args = parser.parse_args()
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
        # top.print_all_full_dpll_status()
        # DPLL over fiber stuff
        top.do_dpll_over_fiber()

    elif args.command == "flash":
        top = MiniPTM()
        if args.board_id is not None:
            top.flash_eeprom_one_board(
                top.boards[args.board_id], args.eeprom_file)
        else:
            top.program_all_boards(args.eeprom_file)

    else:
        print(f"Invalid command, must be program or debug")

    # PFM STUFF
    # top.print_all_pcie_clock_info()
    # top.do_pfm_use_input_tdc()

    # BIG REGISTER DUMP
    # top.print_all_full_dpll_status()
