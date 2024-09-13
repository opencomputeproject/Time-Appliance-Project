
import shutil
import os


# Define the source file path and the destination path
source_path = 'C:\\path\\to\\your\\file.txt'
source_path = 'C:\\Julians\\Projects\\RCB WWVB\\8bit_counter\\impl1\\8_bit_counter_impl1.bin'
destination_path = os.getcwd()  # Gets the current working directory

# Use shutil.copy() to copy the file
shutil.copy(source_path, destination_path)

print(f"File copied to {destination_path}")

# Python script to convert a binary file to a C++ header file
def bin_to_cpp_array(file_path, array_name, line_length=16):
    try:
        # Open the binary file
        with open(file_path, 'rb') as file:
            data = file.read()

        # Start generating the C++ header content
        header_content = f"unsigned char const {array_name}[] = {{\n    "

        # Convert each byte to its hexadecimal representation and format it
        hex_values = [f"0x{byte:02x}" for byte in data]
        # Group hex values for better readability
        for i in range(0, len(hex_values), line_length):
            line = ", ".join(hex_values[i:i+line_length])
            if i > 0:
                header_content += "    "
            header_content += line + ",\n"

        # Remove the last comma and newline
        if hex_values:
            header_content = header_content.rstrip(",\n") + "\n"

        # Close the array and add the array size
        header_content += f"}};\nunsigned int const {array_name}_size = sizeof({array_name});\n"

        # Output the result to a header file
        with open(f"{array_name}.h", 'w') as header_file:
            header_file.write(header_content)

        print(f"Header file {array_name}.h created successfully.")
    except IOError as e:
        print(f"Error opening or reading file: {e}")

# Example usage
bin_to_cpp_array("8_bit_counter_impl1.bin", "fpgaFirmware")
