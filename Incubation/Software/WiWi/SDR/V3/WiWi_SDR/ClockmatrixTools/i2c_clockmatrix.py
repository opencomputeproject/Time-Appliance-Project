
import os
import glob
import struct
import math
import serial 
import time


class i2c_cm:
    def __init__(self, com_chan: int):
        self.com_chan = com_chan
        self.ser = serial.Serial(port=f"COM{com_chan}", baudrate=9600)
        print(f"Connected to port COM{com_chan}")
        command = f"cd ..\r\n"
        self.ser.write(command.encode())
        time.sleep(0.001)
        command = f"dpll\r\n"
        self.ser.write(command.encode())
        time.sleep(0.001)
        while self.ser.in_waiting > 0:
            temp = self.ser.read(self.ser.in_waiting).decode()

    def __str__(self):
        return "i2c clock matrix for WiWi SDR v2 board"


    # Function to read until we get the prompt (>) and return the full response
    def read_response(self):
        buffer = ""
        while True:
            if self.ser.in_waiting > 0:
                buffer += self.ser.read(self.ser.in_waiting).decode()  # Read available data
                if '>' in buffer:  # Stop when we receive the prompt
                    break
            #time.sleep(0.0001)  # Small delay to prevent busy-waiting
        return buffer


    def dpll_write_eeprom(self, addr, data):
        addr_hex = f"0x{addr:X}"
        # Convert each integer to a hex string and join them with a space
        hex_string = ' '.join(f'0x{num:X}' for num in data)
        command = f"dpll-eeprom-write {addr_hex} {hex_string}\n"
        self.ser.write(command.encode())

        # Read and process the response
        response = self.read_response()
        if "DPLL EEPROM Wrote address" in response:
            #print("Write success!")
            pass
        else:
            print(f"Write failed: {response}")
        
    # Function to write to the DPLL register
    def dpll_write_reg(self, baseaddr, offset, value):
        # Format baseaddr, offset, and value as 0x-prefixed hex strings
        baseaddr_hex = f"0x{baseaddr:X}"
        offset_hex = f"0x{offset:X}"
        value_hex = f"0x{value:X}"
        
        # Send the dpll-write-reg command
        command = f"dpll-write-reg {baseaddr_hex} {offset_hex} {value_hex}\n"
        self.ser.write(command.encode())

        # Read and process the response
        response = self.read_response()
        if "DPLL write success" in response:
            #print("Write success!")
            pass
        else:
            print(f"Write failed: {response}")

    # Function to read from the DPLL register
    def dpll_read_reg(self, baseaddr, offset):
        # Format baseaddr and offset as 0x-prefixed hex strings
        baseaddr_hex = f"0x{baseaddr:X}"
        offset_hex = f"0x{offset:X}"

        # Send the dpll-read-reg command
        command = f"dpll-read-reg {baseaddr_hex} {offset_hex}\n"
        self.ser.write(command.encode())

        # Read and process the response
        response = self.read_response()
        lines = response.splitlines()
        int_value = 0
        
        for num,line in enumerate(lines):
            #print(f"Line {num}: {line}")
            if "read back" in line:
                start_index = line.find('read back')
                # Extract the part of the string after 'read back '
                hex_value_str = line[start_index + len('read back '):]

                # Strip any extra whitespace or trailing characters if needed
                hex_value_str = hex_value_str.strip()

                # Convert the hex string to an integer
                int_value = int(hex_value_str, 16)
                
                #print(f"Extracted hex value: {hex_value_str}")
                #print(f"Converted to integer: {int_value}")
                
        return int_value   
        #if "DPLL baseaddr" in response:
        #    print(f"Read success! Got {response}")
        #    print(response.strip())  # Print the read result
        #else:
        #    print(f"Read failed: {response}")
    def write_dpll_reg_direct(self, addr, value):
        self.dpll_write_reg(addr, 0, value)
        
    def read_dpll_reg_direct(self, addr):
        return self.dpll_read_reg(addr, 0)
    def read_dpll_reg_multiple_direct(self):
        pass
    def write_dpll_multiple(self):
        pass
    
