
import os
import glob
import smbus2
import struct
import math

# Constants for SFP module I2C slave addresses
SFP_ADDRESS_A0 = 0x50  # Address for serial ID and vendor information
SFP_ADDRESS_A2 = 0x51  # Address for diagnostic information




def find_i2c_buses(adapter_name):
    i2c_buses = []
    # Path to the I2C adapter information
    i2c_adapter_path = '/sys/class/i2c-adapter/*'
    for i2c_bus_path in glob.glob(i2c_adapter_path):
        try:
            with open(os.path.join(i2c_bus_path, 'name'), 'r') as file:
                name = file.read().strip()
                if adapter_name in name:
                    # Extract the bus number from the path
                    bus_number = int(i2c_bus_path.split('/')
                                     [-1].replace('i2c-', ''))
                    i2c_buses.append(bus_number)
        except IOError:
            pass  # Ignore if we can't read the file for some reason
    return i2c_buses


class miniptm_i2c:
    def __init__(self, bus_num: int):
        self.bus_num = bus_num
        self.bus = smbus2.SMBus(bus_num)
        self.DPLL_ADDRESS = 0x58
        self.MUX_ADDRESS = 0x70
        self.cur_base_addr = None  # used by DPLL code
        self.cur_mux_open = 0

    def __str__(self):
        return "MiniPTM i2c"

    def open_i2c_dpll(self):
        # Set up I2C mux to access DPLL
        if (self.cur_mux_open != 0x8):
            print(f"Opening mux to DPLL")
            self.bus.write_byte_data(self.MUX_ADDRESS, 0x0, 0x8)
            self.cur_mux_open = 0x8
        self.cur_base_addr = None

    def write_dpll_reg(self, base_addr, offset, value):
        self.open_i2c_dpll()
        full_addr = base_addr + offset
        baseaddr_lower = full_addr & 0xff
        baseaddr_upper = (full_addr >> 8) & 0xff

        #print(f"Write DPLL register, addr {full_addr:#02x} = {value:#02x}")
        if self.cur_base_addr != baseaddr_upper:
            self.bus.write_i2c_block_data(self.DPLL_ADDRESS, 0xfc, [
                                          baseaddr_lower, baseaddr_upper, 0x10, 0x20])
            self.cur_base_addr = baseaddr_upper

        self.bus.write_byte_data(self.DPLL_ADDRESS, baseaddr_lower, value)
        #print(f"Write DPLL register, module {base_addr:#04x}, addr {offset:#02x} = {value:#02x}")

    def write_dpll_reg_direct(self, addr, value):
        self.open_i2c_dpll()
        baseaddr_lower = addr & 0xff
        baseaddr_upper = (addr >> 8) & 0xff

        if self.cur_base_addr != baseaddr_upper:
            #print(f"Write DPLL register direct, addr {addr:#02x} = {value:#02x}")
            self.bus.write_i2c_block_data(self.DPLL_ADDRESS, 0xfc, [
                                          baseaddr_lower, baseaddr_upper, 0x10, 0x20])
            self.cur_base_addr = baseaddr_upper

        self.bus.write_byte_data(self.DPLL_ADDRESS, baseaddr_lower, value)

    def write_dpll_multiple(self, addr, data_bytes):
        self.open_i2c_dpll()
        baseaddr_lower = addr & 0xff
        baseaddr_upper = (addr >> 8) & 0xff

        if self.cur_base_addr != baseaddr_upper:
            #print(f"Write DPLL multiple, addr {addr:#02x} = {data_bytes}")
            self.bus.write_i2c_block_data(self.DPLL_ADDRESS, 0xfc, [
                                          baseaddr_lower, baseaddr_upper, 0x10, 0x20])
            self.cur_base_addr = baseaddr_upper

        self.bus.write_i2c_block_data(
            self.DPLL_ADDRESS, baseaddr_lower, data_bytes)


    def read_dpll_reg(self, base_addr, offset):
        self.open_i2c_dpll()
        full_addr = base_addr + offset
        baseaddr_lower = full_addr & 0xff
        baseaddr_upper = (full_addr >> 8) & 0xff

        if self.cur_base_addr != baseaddr_upper:
            self.bus.write_i2c_block_data(self.DPLL_ADDRESS, 0xfc, [
                                          baseaddr_lower, baseaddr_upper, 0x10, 0x20])
            self.cur_base_addr = baseaddr_upper

        val = self.bus.read_byte_data(self.DPLL_ADDRESS, baseaddr_lower)
        #print(f"Read dpll reg {full_addr:#02x} = {val:#02x}")
        return val

    def read_dpll_reg_direct(self, addr):
        self.open_i2c_dpll()
        baseaddr_lower = addr & 0xff
        baseaddr_upper = (addr >> 8) & 0xff
        #print(f"Read reg direct {addr:02x}, lower={baseaddr_lower:02x}, upper={baseaddr_upper:02x}")
        if self.cur_base_addr != baseaddr_upper:
            self.bus.write_i2c_block_data(self.DPLL_ADDRESS, 0xfc, [
                                          baseaddr_lower, baseaddr_upper, 0x10, 0x20])
            self.cur_base_addr = baseaddr_upper

        val = self.bus.read_byte_data(self.DPLL_ADDRESS, baseaddr_lower)
        #print(f"Read dpll reg direct {addr:02x} = {val:02x}")
        return val


    def read_dpll_reg_multiple_direct(self, addr, length):
        #print(f"Called read dpll reg multiple direct addr={addr} len={length}")
        return self.read_dpll_reg_multiple(addr, 0, length)

    def read_dpll_reg_multiple(self, base_addr, offset, numbytes):
        #print(f"Called read dpll reg multiple base={base_addr} off={offset} num={numbytes}")
        self.open_i2c_dpll()
        full_addr = base_addr + offset
        baseaddr_lower = full_addr & 0xff
        baseaddr_upper = (full_addr >> 8) & 0xff

        #print(f"Read mul full_addr {full_addr}")
        if self.cur_base_addr != baseaddr_upper:
            self.bus.write_i2c_block_data(self.DPLL_ADDRESS, 0xfc, [
                                          baseaddr_lower, baseaddr_upper, 0x10, 0x20])
            self.cur_base_addr = baseaddr_upper

        return self.bus.read_i2c_block_data(self.DPLL_ADDRESS, baseaddr_lower, numbytes)



    # Function to read data from an I2C device

    def read_i2c_data(self, address, start_reg, length):
        try:
            data = self.bus.read_i2c_block_data(address, start_reg, length)
            return data
        except Exception as e:
            print(f"Error reading from I2C device: {e}")
            return None

    # Function to interpret the data
    def interpret_data(self, data):
        if data:
            #print(f"Interpret sfp data: {data}")
            vendor_name = bytes(data[20:36]).decode('utf-8').strip()
            serial_number = bytes(data[68:84]).decode('utf-8').strip()
            vendor_part_number = bytes(data[40:56]).decode('utf-8').strip()
            module_type = data[0]
            return vendor_name, serial_number, vendor_part_number, module_type
        else:
            return None, None, None, None

    # Function to read SFP module information
    def read_sfp_module(self, sfp_num=1):
        if (sfp_num >= 1 and sfp_num <= 4):
            pass
        else:
            print(f"Invalid SFP Num {sfp_num}, should be 1-4")
            return

        mux_val = 0
        if sfp_num == 1:
            mux_val = 0x1
        elif sfp_num == 2:
            mux_val = 0x2
        elif sfp_num == 3:
            mux_val = 0x20
        elif sfp_num == 4:
            mux_val = 0x40

        print(f"\nReading SFP Module {sfp_num}")
        # Set up I2C mux to access SFP
        self.bus.write_byte_data(self.MUX_ADDRESS, 0x0, mux_val)
        self.cur_mux_open = mux_val
        # Read data from address A0
        try:
            data_a0 = []
            data_a0 += self.read_i2c_data(SFP_ADDRESS_A0, 0, 32)
            data_a0 += self.read_i2c_data(SFP_ADDRESS_A0, 32, 32)
            data_a0 += self.read_i2c_data(SFP_ADDRESS_A0, 64, 32)
            vendor_name, serial_number, vendor_part_number, module_type = self.interpret_data(
                data_a0)
            print(f"Full A0 data: {data_a0}")
            print(
                f"Vendor Name: {vendor_name}, Serial Number: {serial_number}")
            print(
                f"Vendor Part Number: {vendor_part_number}, Module Type: {module_type}")
        except:
            print(f"SFP module {sfp_num} not inserted")
            # Close MUX
            self.bus.write_byte_data(self.MUX_ADDRESS, 0x0, 0x0)
            self.cur_mux_open = 0x0
            return

        # Read temperature, Tx and Rx power from address A2
        try:  # this might fail, copper cables for instance don't even implement A2 sometimes
            # Assuming temperature is at register 96
            data_a2 = []
            data_a2 += self.read_i2c_data(SFP_ADDRESS_A2, 0, 32)
            data_a2 += self.read_i2c_data(SFP_ADDRESS_A2, 32, 32)
            data_a2 += self.read_i2c_data(SFP_ADDRESS_A2, 64, 32)
            data_a2 += self.read_i2c_data(SFP_ADDRESS_A2, 96, 32)
            print(f"Full A2 data: {data_a2}")

            temp_data = self.read_i2c_data(SFP_ADDRESS_A2, 96, 2)
            tx_power_data = self.read_i2c_data(
                SFP_ADDRESS_A2, 102, 2)  # Tx power at register 102
            rx_power_data = self.read_i2c_data(
                SFP_ADDRESS_A2, 104, 2)  # Rx power at register 104

            if temp_data:
                temp_raw = struct.unpack('>h', bytes(temp_data))[0]
                temperature = temp_raw / 256.0
                print(f"Temperature: {temperature} C")

            if tx_power_data:
                #print(f"Raw tx power data:{tx_power_data}")
                tx_power_raw = (tx_power_data[0] << 8) + tx_power_data[1]
                tx_power = tx_power_raw * 0.1  # Conversion based on specification to uW
                #print(f"TX Power in uW: {tx_power}")
                tx_power = 10 * math.log10(tx_power / 1000)
                print(f"Transmit Power: {tx_power} dBm")

            if rx_power_data:
                #print(f"Raw rx power data:{rx_power_data}")
                rx_power_raw = (rx_power_data[0] << 8) + rx_power_data[1]
                rx_power = rx_power_raw * 0.1  # Conversion based on specification
                #print(f"RX power in uW: {rx_power}")
                if rx_power == 0:
                    print(f"Receive power: -40 dBm")
                else:
                    rx_power = 10 * math.log10(rx_power / 1000)
                    print(f"Receive Power: {rx_power} dBm")


            # read control / status register
            data = self.bus.read_byte_data(SFP_ADDRESS_A2, 110)
            print(f"Control register 110 = 0x{data:02x}")
        except:
            pass

        # Close MUX
        self.bus.write_byte_data(self.MUX_ADDRESS, 0x0, 0x0)
        self.cur_mux_open = 0x0
