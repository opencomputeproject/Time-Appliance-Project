
file='8A34002_MiniPTMV3_12-29-2023_Julian_AllPhaseMeas_EEPROM.hex'

def parse_intel_hex(file_path):
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

# Example usage
data_records, non_data_records = parse_intel_hex(file)

# Printing the results
print("Data Records:")
for address, data_bytes in data_records.items():
    print(f"Address: {address:06X}, Data: {', '.join(f'{b:02X}' for b in data_bytes)}")

print("\nNon-Data Records:")
for (line_number, address, record_type), data in non_data_records.items():
    print(f"Line: {line_number}, Address: {address:04X}, Record Type: {record_type}, Data: {data}")

