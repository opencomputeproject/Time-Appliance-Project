def parse_dpll_tcs_config_file(file_path):
    """
    Parses the DPLL configuration file to extract register addresses and values.
    This version tries to be more flexible in identifying lines that are likely to contain register configurations.
    """
    config_data = []

    with open(file_path, 'r') as file:
        for line in file:
            parts = line.split()
            # Look for lines with at least 3 parts and a period in the first part
            #print(parts)
            if len(parts) >= 3 and '.' in parts[0] and '.' in parts[3]:
                # Attempt to parse the register address and value
                try:
                    # Assuming register address is before the period and value is the third part
                    register_part = parts[0].split('.')
                    register_lower = int(register_part[1], 16)
                    register_upper = int(register_part[0], 16) << 8
                    register = register_lower + register_upper
                    value = int(parts[2], 16)
                    config_data.append((register, value))
                except (ValueError, IndexError):
                    # Skip lines that don't match the expected format
                    continue

    return config_data



def parse_intel_hex(file_path):
    """
    Parses an Intel HEX file.
    Returns a dictionary where each key is an address and the value is the data for that address.
    """
    data_records = {}

    with open(file_path, 'r') as file:
        for line in file:
            if line.startswith(':'):
                byte_count = int(line[1:3], 16)
                address = int(line[3:7], 16)
                record_type = int(line[7:9], 16)
                data = line[9:9 + 2 * byte_count]
                checksum = int(line[9 + 2 * byte_count:9 + 2 * byte_count + 2], 16)

                # Calculate and verify the checksum
                calculated_checksum = sum(bytearray.fromhex(line[1:9 + 2 * byte_count])) & 0xFF
                calculated_checksum = (~calculated_checksum + 1) & 0xFF
                if calculated_checksum != checksum:
                    raise ValueError("Checksum mismatch")

                if record_type == 0:  # Data record
                    data_bytes = bytearray.fromhex(data)
                    data_records[address] = data_bytes

    return data_records


def print_human_readable_hex_data(data):
    """
    Prints the parsed Intel HEX data in a human-readable format.
    Displays each start address followed by its corresponding data bytes in hexadecimal notation.
    """
    for address, data_bytes in data.items():
        # Convert the bytearray to a hex string
        hex_data = ' '.join(f'0x{byte:02X}' for byte in data_bytes)
        print(f"Address 0x{address:04X}: {hex_data}")

def print_limited_human_readable_hex_data(data, limit=10):
    """
    Prints a limited number of entries from the parsed Intel HEX data in a human-readable format.
    Displays each start address followed by its corresponding data bytes in hexadecimal notation.
    """
    count = 0
    for address, data_bytes in data.items():
        if count >= limit:
            break
        # Convert the bytearray to a hex string
        hex_data = ' '.join(f'0x{byte:02X}' for byte in data_bytes)
        print(f"Address 0x{address:04X}: {hex_data}")
        count += 1



def parse_intel_hex_file(file_path):
    data_records = {}
    non_data_records = {}
    extended_address = 0

    with open(file_path, 'r') as file:
        for line_number, line in enumerate(file, start=1):
            # Each line is a record
            start_code = line[0:1]
            byte_count = int(line[1:3], 16)
            address = int(line[3:7], 16)
            record_type = int(line[7:9], 16)
            data = line[9:9+2*byte_count]
            checksum = line[9+2*byte_count:9+2*byte_count+2]

            # Process the data
            if record_type == 0:  # Data record
                full_address = extended_address + address
                data_bytes = [int(data[i:i+2], 16) for i in range(0, len(data), 2)]
                data_records[full_address] = data_bytes
            elif record_type == 4:  # Extended linear address record
                extended_address = int(data, 16) << 16
                non_data_records[(line_number, address, record_type)] = data
            else:  # Other non-data records
                non_data_records[(line_number, address, record_type)] = data

    return data_records, non_data_records

if __name__ == "__main__":
    # Define the file path
    file_path = '8A34002_100MHz_out_freerun.tcs'

    # Re-parse the configuration file with the new method
    parsed_config_tcs = parse_dpll_tcs_config_file(file_path)
    
    print("First few config tcs lines")
    print(parsed_config_tcs[:10])
        

    # Define the path to the Intel HEX file
    intel_hex_file_path = "8A34002_100MHz_out_freerun_leds_eeprom.hex"

    # Parse the Intel HEX file
    parsed_hex_data = parse_intel_hex(intel_hex_file_path)

    # Display the first few parsed data records for verification
    list(parsed_hex_data.items())[:10]  # Show first 10 data records

    # Print the parsed Intel HEX data in a human-readable format
    print_limited_human_readable_hex_data(parsed_hex_data)
