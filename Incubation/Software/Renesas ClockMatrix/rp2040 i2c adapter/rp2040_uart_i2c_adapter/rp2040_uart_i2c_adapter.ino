#include <Wire.h>

int sdaPin = 4;  // Default SDA pin
int sclPin = 5;  // Default SCL pin

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.setSDA(sdaPin);
  Wire.setSCL(sclPin);
  Wire.begin();
  Serial.begin();
  while (!Serial);

  Serial.println("I2C Master initialized.");
}

double last_blink_time = 0;
bool led_val = 0;

void loop() {
  if ( millis() - last_blink_time ) {
    digitalWrite(LED_BUILTIN, led_val);
    led_val = !led_val;
    last_blink_time = millis();
  }

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("read_byte")) {
      byte slaveAddress = parseHexByte(command, 2);
      byte registerAddress = parseHexByte(command, 5);
      byte data;
      bool success = readByteFromI2C(slaveAddress, registerAddress, data);
      if (success) {
        Serial.print("Read Byte from Slave 0x");
        Serial.print(slaveAddress, HEX);
        Serial.print(", Register 0x");
        Serial.print(registerAddress, HEX);
        Serial.print(": 0x");
        Serial.println(data, HEX);
      } else {
        Serial.println("Read Byte: Failed");
      }
    }
    else if (command.startsWith("write_byte")) {
      byte slaveAddress = parseHexByte(command, 2);
      byte registerAddress = parseHexByte(command, 5);
      byte value = parseHexByte(command, 8);
      bool success = writeByteToI2C(slaveAddress, registerAddress, value);
      if (success) {
        Serial.print("Write Byte to Slave 0x");
        Serial.print(slaveAddress, HEX);
        Serial.print(", Register 0x");
        Serial.print(registerAddress, HEX);
        Serial.println(": OK");
      } else {
        Serial.println("Write Byte: Failed");
      }
    }
    else if (command.startsWith("read_word")) {
      byte slaveAddress = parseHexByte(command, 2);
      byte registerAddress = parseHexByte(command, 5);
      word data;
      bool success = readWordFromI2C(slaveAddress, registerAddress, data);
      if (success) {
        Serial.print("Read Word from Slave 0x");
        Serial.print(slaveAddress, HEX);
        Serial.print(", Register 0x");
        Serial.print(registerAddress, HEX);
        Serial.print(": 0x");
        Serial.println(data, HEX);
      } else {
        Serial.println("Read Word: Failed");
      }
    }
    else if (command.startsWith("write_word")) {
      byte slaveAddress = parseHexByte(command, 2);
      byte registerAddress = parseHexByte(command, 5);
      word value = parseHexWord(command, 8);
      bool success = writeWordToI2C(slaveAddress, registerAddress, value);
      if (success) {
        Serial.print("Write Word to Slave 0x");
        Serial.print(slaveAddress, HEX);
        Serial.print(", Register 0x");
        Serial.print(registerAddress, HEX);
        Serial.println(": OK");
      } else {
        Serial.println("Write Word: Failed");
      }
    }
    else if (command.startsWith("set_pins")) {
      int newSdaPin = parsePin(command, "sda_pin=");
      int newSclPin = parsePin(command, "scl_pin=");

      if (newSdaPin >= 0 && newSclPin >= 0) {
        Wire.end();
        Wire.setSDA(sdaPin);
        Wire.setSCL(sclPin);
        Wire.begin();
        Serial.println("Pin configuration updated.");
      } else {
        Serial.println("Invalid pin configuration.");
      }
    }
    else if (command.startsWith("write_bulk")) {
      bool success = writeBulkToI2C(command);
      if (success) {
        Serial.println("Write Bulk: OK");
      } else {
        Serial.println("Write Bulk: Failed");
      }
    }
    else if (command.startsWith("read_bulk")) {
      String response = readBulkFromI2C(command);
      if (response.length() > 0) {
        Serial.println(response);
      } else {
        Serial.println("Read Bulk: Failed");
      }
    }
    else {
      Serial.println("Invalid command.");
    }
  }
}

byte parseHexByte(String command, int startIndex) {
  String hexValue = command.substring(startIndex, startIndex + 2);
  return strtol(hexValue.c_str(), NULL, 16);
}

word parseHexWord(String command, int startIndex) {
  String hexValue = command.substring(startIndex, startIndex + 4);
  return strtol(hexValue.c_str(), NULL, 16);
}

int parsePin(String command, String keyword) {
  int index = command.indexOf(keyword);
  if (index != -1) {
    String value = command.substring(index + keyword.length());
    return value.toInt();
  }
  return -1;
}

bool readByteFromI2C(byte slaveAddress, byte registerAddress, byte &data) {
  Wire.beginTransmission(slaveAddress);
  Wire.write(registerAddress);
  byte error = Wire.endTransmission(false);
  if (error == 0) {
    Wire.requestFrom(slaveAddress, 1);
    if (Wire.available()) {
      data = Wire.read();
      return true;
    }
  }
  return false;
}

bool writeByteToI2C(byte slaveAddress, byte registerAddress, byte value) {
  Wire.beginTransmission(slaveAddress);
  Wire.write(registerAddress);
  Wire.write(value);
  byte error = Wire.endTransmission();
  return (error == 0);
}

bool readWordFromI2C(byte slaveAddress, byte registerAddress, word &data) {
  Wire.beginTransmission(slaveAddress);
  Wire.write(registerAddress);
  byte error = Wire.endTransmission(false);
  if (error == 0) {
    Wire.requestFrom(slaveAddress, 2);
    if (Wire.available() >= 2) {
      byte lowByte = Wire.read();
      byte highByte = Wire.read();
      data = (word(highByte) << 8) | lowByte;
      return true;
    }
  }
  return false;
}

bool writeWordToI2C(byte slaveAddress, byte registerAddress, word value) {
  Wire.beginTransmission(slaveAddress);
  Wire.write(registerAddress);
  Wire.write(lowByte(value));
  Wire.write(highByte(value));
  byte error = Wire.endTransmission();
  return (error == 0);
}



bool writeBulkToI2C(String command) {
  int spaceIndex = command.indexOf(' ');
  if (spaceIndex == -1) {
    return false;
  }

  byte slaveAddress = parseHexByte(command, spaceIndex + 1);
  String dataString = command.substring(spaceIndex + 4);  // Skip "<slave_address> "

  // Split the dataString into individual data bytes
  while (dataString.length() >= 4) {
    byte dataByte = parseHexByte(dataString, 0);
    if (!writeByteToI2C(slaveAddress, 0x00, dataByte)) {
      return false;  // If any write fails, return false
    }
    dataString = dataString.substring(5);  // Move to the next data byte
  }

  return true;
}


String readBulkFromI2C(String command) {
  int spaceIndex = command.indexOf(' ');
  if (spaceIndex == -1) {
    return "";
  }

  byte slaveAddress = parseHexByte(command, spaceIndex + 1);
  int byteCount = Serial.parseInt();  // Read the number of bytes to read
  String response = "";

  Wire.beginTransmission(slaveAddress);
  byte error = Wire.endTransmission(false);

  if (error == 0) {
    
    for (int i = 0; i < byteCount; i++) {
      Wire.requestFrom(slaveAddress, 1);
      if (Wire.available()) {
        byte dataByte = Wire.read();
        response += "0x";
        response += String(dataByte, HEX);
        if (i < byteCount - 1) {
          response += " ";
        }
      } else {
        response = "";
        break;
      }
    }
  }

  return response;
}
