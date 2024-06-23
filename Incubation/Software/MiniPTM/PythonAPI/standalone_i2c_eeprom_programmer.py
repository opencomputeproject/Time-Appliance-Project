
from renesas_cm_configfiles import *
from smbus2 import SMBus
import time

eeprom_file="MiniPTMV4_BaseConfig_4-10-2024.hex"
i2c_bus=4
eeprom_addr=0x54
block_select_bit=1
write_delay=0.005 #5ms delay after write



def write_eeprom(bus, device_address, start_memory_address, data_list):
    with SMBus(bus) as smbus:
        for offset, data in enumerate(data_list):
            memory_address = start_memory_address + offset
            block_device_address = device_address | ((memory_address >> 16) & block_select_bit)
            memory_address_low = memory_address & 0xFFFF
            # Preparing address and data bytes
            address_bytes = [memory_address_low >> 8, memory_address_low & 0xFF]
            # Writing using write_i2c_block_data
            #print(f"Block data addr=0x{block_device_address:02x}, addr0=0x{address_bytes[0]:02x}")
            smbus.write_i2c_block_data(block_device_address, address_bytes[0], address_bytes[1:] + [data])
            time.sleep(write_delay)  # Delay after each byte write


hex_file_data, non_data_records_debug = parse_intel_hex_file( eeprom_file)
for addr in hex_file_data.keys():
    print(f"Write addr 0x{addr:02x} = {hex_file_data[addr]}")
    write_eeprom(i2c_bus, eeprom_addr, addr, hex_file_data[addr])

