
#from i2c_clockmatrix import find_i2c_buses
from board_sdr import Single_SDR
from renesas_cm_configfiles import *
import concurrent.futures
import time
import argparse
from scipy import stats
import sys
import threading
import select
from scipy.signal import medfilt
import numpy as np
import random
import statistics



class sdrboard:
    def __init__(self):
        self.boards = [Single_SDR(0,"",com_num=15)]
        pass

    def program_one_board(self, board, data):
        for [address, value] in data:
            print(
                f"Board {board.adap_num} 0x{address:x}=0x{value:x}")
            board.i2c.write_dpll_reg_direct(address, value)
        return board.adap_num

    def flash_eeprom_one_board(
            self,
            board,
            eeprom_file="8A34002_MiniPTMV3_12-29-2023_Julian_AllPhaseMeas_EEPROM.hex"):
        print(f"Start flash board {board.board_num} EEPROM = {eeprom_file}")
        board.write_eeprom_file(eeprom_file)
        print(f"DONE flash board {board.board_num} EEPROM = {eeprom_file}")

    def print_all_full_dpll_status(self):
        for board in self.boards:
            print(
                f"****************** BOARD {board.board_num} STATUS REGISTERS *************")
            board.dpll.modules["Status"].print_all_registers_all_modules()

    def debug_me(self):

        
        return

    def debug_print(self):
        # debugging locking stuff
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(0)
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(1)
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(2)
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(3)
        print(f"\n\n************** BOARD 0 ***********************\n\n")
        self.boards[0].dpll.modules["DPLL_GeneralStatus"].print_all_registers(0)
        self.boards[0].dpll.modules["Status"].print_all_registers(0)
        self.boards[0].dpll.modules["TOD"].print_all_registers(2)
        self.boards[0].dpll.modules["PWMEncoder"].print_all_registers(2)
        self.boards[0].dpll.modules["PWMDecoder"].print_all_registers(4)
        # self.boards[0].dpll.modules["DPLL_Config"].print_all_registers(0)
        # self.boards[0].dpll.modules["DPLL_Config"].print_all_registers(5)

        print(f"\n\n************** BOARD 1 ***********************\n\n")
        self.boards[1].dpll.modules["DPLL_GeneralStatus"].print_all_registers(0)
        self.boards[1].dpll.modules["Status"].print_all_registers(0)
        self.boards[1].dpll.modules["TOD"].print_all_registers(2)
        self.boards[1].dpll.modules["PWMEncoder"].print_all_registers(2)
        self.boards[1].dpll.modules["PWMDecoder"].print_all_registers(4)
        # self.boards[1].dpll.modules["DPLL_Config"].print_all_registers(0)

        # clear stickies
        for board in self.boards:
            board.i2c.write_dpll_reg_direct(0xc164 + 0x5, 0x1)

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

        # for board in self.boards:
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

    parser=argparse.ArgumentParser(description="SDR top level debug")

    parser.add_argument(
        'command',
        type=str,
        help='What command to run, blinktest / program / debug_dpof / debug_pfm / flash / read / write')
    parser.add_argument(
        '--config_file',
        type=str,
        default="8A34002_MiniPTMV3_12-27-2023_Julian_AllPhaseMeas.tcs",
        help="File to program")
    parser.add_argument(
        '--eeprom_file',
        type=str,
        default="8A34002_MiniPTMV3_1-17-2024_Julian_PFM_PMOS_Master_EEPROM.hex",
        help="File to flash to EEPROM")
    parser.add_argument('--board_id', type=int, help="Board number to program")
    parser.add_argument("--reg_addr", type=str, default="0xc024",
                        help="Register address to read or write")
    parser.add_argument("--reg_val", type=str, default="0x0",
                        help="Register address to read or write")

    args=parser.parse_args()

    # simple user input handler in separate thread
    # input_thread = threading.Thread(target=wait_for_input, args=(MiniPTM.user_input_str,MiniPTM.user_input_lock))
    # input_thread.start()

    # stop_event = threading.Event()
    # input_thread = threading.Thread(
    #    target=input_thread_func, args=(stop_event,))
    # input_thread.start()

    if args.command == "program":
        top=sdrboard()
        if args.board_id is not None:
            parsed_config_tcs=parse_dpll_tcs_config_file(args.config_file)
            top.program_one_board(top.boards[args.board_id], parsed_config_tcs)
        else:
            top.program_all_boards(config_file=args.config_file)

        # top.set_all_boards_leds_idcode()
    elif args.command == "blinktest":
        top=sdrboard()
        top.board_led_blink_test()
    elif args.command == "debug":
        top=sdrboard()
        top.debug_me()
    elif args.command == "debug_sfp":
        top=sdrboard()
        for board in top.boards:
            print(
                f"\n *********** SFP Debug Board {board.board_num} ***********\n")
            board.print_sfps_info()
    elif args.command == "debug_print":
        top=sdrboard()
        top.debug_print()
    elif args.command == "debug_dpof":
        top=sdrboard()
        top.dpll_over_fiber_test()

    elif args.command == "debug_pfm":
        top=sdrboard()
        top.calibrate_pfm_sacrifice_dpll()

    elif args.command == "flash":
        top=sdrboard()
        if args.board_id is not None:
            top.flash_eeprom_one_board(
                top.boards[args.board_id], args.eeprom_file)
        else:
            top.flash_all_boards_eeprom(args.eeprom_file)
    elif args.command == "read":
        top=sdrboard()
        #print(f"Starting read register")
        if args.board_id is not None:
            val=top.boards[args.board_id].i2c.read_dpll_reg_direct(
                int(args.reg_addr, 16))
            print(
                f"Board {args.board_id} Read Register {args.reg_addr} = 0x{val:02x}")
        else:
            for board in top.boards:
                val=board.i2c.read_dpll_reg_direct(int(args.reg_addr, 16))
                print(
                    f"Board {board.board_num} Read Register {args.reg_addr} = 0x{val:02x}")
    elif args.command == "write":
        top=sdrboard()
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
