import serial

class RP2040I2C:
    def __init__(self, port='/dev/ttyUSB0', baudrate=9600):
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


def main():
    rp2040 = RP2040I2C()
    print("RP2040 Initialized")

    # Set SDA and SCL pins (customize these based on your hardware setup)
    rp2040.set_pins(4, 5)

    # Example: Write a single byte to an I2C slave
    slave_address = 0x50
    register_address = 0x00
    value = 0xAA
    response = rp2040.write_byte(slave_address, register_address, value)
    if "OK" in response:
        print("Write Byte: Successful")
    else:
        print("Write Byte: Failed")

    # Example: Read a single byte from an I2C slave
    response = rp2040.read_byte(slave_address, register_address)
    if response.startswith("Read Byte: "):
        read_value = int(response.split(":")[1].strip(), 16)
        print(f"Read Byte: {hex(read_value)}")
    else:
        print("Read Byte: Failed")

    # Example: Write a bulk of data to an I2C slave
    bulk_data = [0xA5, 0x7A, 0x33, 0x0F]
    response = rp2040.write_bulk(slave_address, bulk_data)
    if "OK" in response:
        print("Write Bulk: Successful")
    else:
        print("Write Bulk: Failed")

    # Example: Read a bulk of data from an I2C slave
    num_bytes = 4
    response = rp2040.read_bulk(slave_address, num_bytes)
    if response:
        print(f"Read Bulk: {response}")
    else:
        print("Read Bulk: Failed")

    rp2040.close()
    print("RP2040 Communication Closed")

if __name__ == "__main__":
    main()
