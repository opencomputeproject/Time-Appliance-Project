
from pcie_miniptm import get_miniptm_devices
from i2c_miniptm import find_i2c_buses
from board_miniptm import Single_MiniPTM
from renesas_cm_configfiles import *
class MiniPTM:
    config_file="8A34002_MiniPTMV3_12-24-2023_Julian.tcs"
    def __init__(self):
        miniptm_devs = get_miniptm_devices()
        print(f"Got {len(miniptm_devs)} mini ptm devices: {miniptm_devs}")

        i2c_busses = find_i2c_buses("MiniPTM I2C Adapter")
        print(f"Matching i2c buses: {i2c_busses}")

        if ( len(miniptm_devs) != len(i2c_busses) ):
            print("Mismatch between number of pcie devices and i2c busses, end!")
            return 

        # assume theyre in order, probably terrible assumption but oh well
        self.boards = []

        for i in range(len(miniptm_devs)):
            self.boards.append( Single_MiniPTM( i, miniptm_devs[i], i2c_busses[i] ) )

        # now lets do some initialization if needed. Do a simple check on each board
        # if the GPIOs for LEDs are set for output, then assume the board is configured
        parsed_config_tcs = parse_dpll_tcs_config_file(MiniPTM.config_file)
        #print(f"First few tcs lines: {parsed_config_tcs[:10]}")



        for board in self.boards:
            if not board.is_configured():
                print(f"Board {board.adap_num} not configured, configuring!")
                continue
                for [address,value] in parsed_config_tcs:
                    print(f"Board {board.adap_num} 0x{address:x}=0x{value:x}")
                    board.i2c.write_dpll_reg_direct(address,value)
            else:
                print(f"Board {board.adap_num} already configured!")


        # do ID test with GPIOs, toggle DPLL GPIO, then toggle I225 LEDs
        for index, board in enumerate(self.boards):
            print(f"Board {index} LED test")
            board.led_visual_test()

        # leave LEDs to show what board number it is
        for board in self.boards:
            board.set_led_id_code()


    def close(self):
        pass

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


if __name__ == "__main__":
    top = MiniPTM()
