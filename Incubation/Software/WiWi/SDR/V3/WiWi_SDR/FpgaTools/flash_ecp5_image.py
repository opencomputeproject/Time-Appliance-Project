
import shutil
import os
import sys
import serial



# Define the source file path and the destination path
source_path = 'C:\\Users\\julianstj\\Desktop\\WorkNotes\\Projects\\PTP\\WiWi\\SDR\\Software\\ECP5_Test\\impl1\\wiwisdr_ecp5_test_impl1.bit'
#destination_path = os.getcwd()  # Gets the current working directory

# Use shutil.copy() to copy the file
#shutil.copy(source_path, destination_path)

#print(f"File copied to {destination_path}")



# Function to send an "erase" command
def send_erase(serial_port, address):
    print(f"********send_erase start address 0x{address:08X} *********")
    command = f"erase 0x{address:08X}\r\n"
    #print(command)
    serial_port.reset_input_buffer()
    serial_port.write(command.encode())
    serial_port.flush()
    response = serial_port.read_until(b'my_qspi>')
    #print(f"Erase command response: {response.decode().strip()}")
    serial_port.reset_input_buffer()
    #print("***** Done erase *****")

# Function to send a "byteWrite" command
def send_byte_write(serial_port, address, chunk):
    #print("********* send_byte_write start **********")
    # Format the command
    values = ' '.join(f"0x{val:02X}" for val in chunk)
    command = f"byteWrite 0x{address:08X} {values}\r\n"
    #print(command)
    serial_port.reset_input_buffer()
    serial_port.write(command.encode())
    serial_port.flush()
    response = serial_port.read_until(b'my_qspi>')
    #print(f"Write command response: {response.decode().strip()}")
    serial_port.reset_input_buffer()
    #print("****** Done write *********")

# Flash the data to the SPI flash
def flash_spi(serial_port, data):
    sector_size = 4096  # Sector size in bytes
    page_size = 16     # Page size in bytes
    address = 0         # Start address in flash

    for i in range(0, len(data), page_size):
        # If the address crosses a sector boundary, send an erase command
        if address % sector_size == 0:
            send_erase(serial_port, address)

        # Get the current 256-byte chunk (or less if at the end of the list)
        chunk = data[i:i + page_size]

        # Send the byteWrite command
        send_byte_write(serial_port, address, chunk)

        # Increment the address by the page size
        address += len(chunk)


def main():
    # Check if the COM port is provided as an argument
    if len(sys.argv) < 2:
        print("Usage: python script.py <COM_PORT>")
        sys.exit(1)

    # Get the COM port from the command-line argument
    com_port = sys.argv[1]

    try:
        # Open the serial port
        serial_port = serial.Serial(
            port=com_port,    # Use the provided COM port
            baudrate=9600,    # Set the baud rate to match your device
            timeout=2         # Timeout in seconds for read operations
        )
        print(f"Opened serial port {com_port} successfully.")
        
        response = serial_port.read_until(expected=b"my_qspi add cli")
        print(response)
        print("************ Done opening **********")

        # Send a command to the serial device
        serial_port.write(b"cd ..\r\n")  # Example command

        # Read until the terminator (or timeout)
        response = serial_port.read_until(expected=b">")
        # Print the response
        print("********* cd response **********")
        print(response)
        
        
        # Open the binary file
        data = []
        with open(source_path, 'rb') as file:
            data = file.read()
        # Convert each byte to a hexadecimal string and print
        print(' '.join(f'{byte:02x}' for byte in data[:10]))
        
        # now have binary data

        serial_port.write(b"my_qspi\r\n")
        response = serial_port.read_until(expected=b"my_qspi>")
        print("********* my_qspi response **********")
        print(response)
        
        
        flash_spi(serial_port, data)
        
        
    
    except serial.SerialException as e:
        print(f"Error: Could not open serial port {com_port}: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")
    finally:
        # Close the serial port if it was opened
        if 'serial_port' in locals() and serial_port.is_open:
            serial_port.close()
            print("Serial port closed.")

if __name__ == "__main__":
    main()














