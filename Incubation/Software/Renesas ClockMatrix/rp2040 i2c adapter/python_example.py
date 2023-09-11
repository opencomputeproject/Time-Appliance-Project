import serial
import platform

class RP2040I2C:
    def __init__(self, port=None, baudrate=115200):
        if port is None:
            if platform.system() == "Windows":
                port = "COM3"  # Change this to the appropriate COM port on Windows
            elif platform.system() == "Linux":
                port = "/dev/ttyUSB0"  # Change this to the appropriate serial port on Linux

        self.ser = serial.Serial(port, baudrate)
        self.ser.flush()

    def send_command(self, command):
        self.ser.write(command.encode() + b'\n')
        response = self.ser.readline().decode().strip()
        return response

    def set_pins(self, sda_pin, scl_pin):
        command = f"set_pins sda_pin={sda_pin} scl_pin={scl_pin}"
        return self.send_command(command)

    def write_byte(self, slave_address, register_address, value):
        command = f"write_byte {slave_address} {register_address} {value}"
        return self.send_command(command)

    def read_byte(self, slave_address, register_address):
        command = f"read_byte {slave_address} {register_address}"
        return self.send_command(command)

    def write_bulk(self, slave_address, data):
        data_str = " ".join([f"{byte:02X}" for byte in data])
        command = f"write_bulk {slave_address} {data_str}"
        return self.send_command(command)

    def read_bulk(self, slave_address, num_bytes):
        command = f"read_bulk {slave_address} {num_bytes}"
        response = self.send_command(command)
        if response:
            return [int(byte, 16) for byte in response.split()]
        else:
            return []

    def close(self):
        self.ser.close()


def print_help():
    print("\nCommands:")
    print("1. write_byte <slave_address> <register_address> <value>")
    print("2. read_byte <slave_address> <register_address>")
    print("3. write_bulk <slave_address> <data_byte1> <data_byte2> ... <data_byteN>")
    print("4. read_bulk <slave_address> <num_bytes>")
    print("5. set_pins <sda_pin> <scl_pin>")
    print("6. exit")

def main():
    rp2040 = RP2040I2C()
    print("RP2040 Initialized")

    while True:
        print("\nRP2040 I2C Interface")
        print_help()
        command = input("\nEnter a command: ").strip()

        if command.startswith("write_byte"):
            parts = command.split()
            if len(parts) == 5:
                _, slave_address, register_address, value = parts
                response = rp2040.write_byte(int(slave_address, 16), int(register_address, 16), int(value, 16))
                print(response)
            else:
                print("Invalid command format. Use 'write_byte <slave_address> <register_address> <value>'")

        elif command.startswith("read_byte"):
            parts = command.split()
            if len(parts) == 4:
                _, slave_address, register_address = parts
                response = rp2040.read_byte(int(slave_address, 16), int(register_address, 16))
                print(response)
            else:
                print("Invalid command format. Use 'read_byte <slave_address> <register_address>'")

        elif command.startswith("write_bulk"):
            parts = command.split()
            if len(parts) >= 4:
                _, slave_address = parts[:2]
                data_bytes = [int(b, 16) for b in parts[2:]]
                response = rp2040.write_bulk(int(slave_address, 16), data_bytes)
                print(response)
            else:
                print("Invalid command format. Use 'write_bulk <slave_address> <data_byte1> <data_byte2> ... <data_byteN>'")

        elif command.startswith("read_bulk"):
            parts = command.split()
            if len(parts) == 4:
                _, slave_address, num_bytes = parts
                response = rp2040.read_bulk(int(slave_address, 16), int(num_bytes, 10))
                print(response)
            else:
                print("Invalid command format. Use 'read_bulk <slave_address> <num_bytes>'")

        elif command.startswith("set_pins"):
            parts = command.split()
            if len(parts) == 4:
                _, sda_pin, scl_pin = parts
                response = rp2040.set_pins(int(sda_pin), int(scl_pin))
                print(response)
            else:
                print("Invalid command format. Use 'set_pins <sda_pin> <scl_pin>'")

        elif command == "exit":
            break

        else:
            print("Invalid command. Type 'help' for a list of commands.")

    rp2040.close()
    print("RP2040 Communication Closed")

if __name__ == "__main__":
    main()
