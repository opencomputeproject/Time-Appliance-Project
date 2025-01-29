def hexdump_to_file(input_file, output_file):
    try:
        with open(input_file, "rb") as infile, open(output_file, "w") as outfile:
            offset = 0
            while chunk := infile.read(16):  # Read 16 bytes per line
                hex_values = " ".join(f"{byte:02X}" for byte in chunk)
                ascii_values = "".join(chr(byte) if 32 <= byte <= 126 else "." for byte in chunk)
                outfile.write(f"{offset:08X}  {hex_values:<48}  {ascii_values}\n")
                offset += 16
        print(f"Hexdump saved to {output_file}")
    except FileNotFoundError:
        print(f"Error: The file '{input_file}' does not exist.")
    except Exception as e:
        print(f"Error: {e}")

# Usage example
input_file = "wiwisdr_ecp5_test_impl1.bit"   # Replace with your binary file
output_file = "example_dump.txt"  # Replace with your output text file
hexdump_to_file(input_file, output_file)
