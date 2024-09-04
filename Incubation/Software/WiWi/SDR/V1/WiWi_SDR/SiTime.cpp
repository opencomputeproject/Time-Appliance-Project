

#include "SiTime.h"

float frequency_offset = 0;

static float sitime_center_freq = 0;
static uint8_t sitime_pull_range = 0;
static float sitime_freq_resolution = 5e-12;

static uint8_t i2c_tx_buf[32];
static uint8_t i2c_rx_buf[32];
static SoftWire SIT5501_I2C(SIT5501_SDA, SIT5501_SCL);

void init_sitime(float center_freq) {

  /**** Store center frequency ****/
  sitime_center_freq = center_freq;



    // enable DCTCXO , this must succeed, otherwise loop, means dctcxo not installed on the board most likely
  int i2c_status = 0;

  
  wwvb_gpio_pinmode(SIT5501_SDA, INPUT);
  wwvb_gpio_pinmode(SIT5501_SCL, INPUT);
  // just do i2c scan 
  //https://github.com/stevemarple/SoftWire/blob/master/examples/ListDevices/ListDevices.ino
  /*
  SIT5501_I2C.setTimeout_ms(40);
  SIT5501_I2C.enablePullups(0);
  SIT5501_I2C.setTxBuffer(i2c_tx_buf, 32);
  SIT5501_I2C.setRxBuffer(i2c_rx_buf, 32);
  SIT5501_I2C.begin();
  const uint8_t firstAddr = 1;
  const uint8_t lastAddr = 0x7F;
  Serial.println();
  Serial.print("Interrogating all addresses in range 0x");
  Serial.print(firstAddr, HEX);
  Serial.print(" - 0x");
  Serial.print(lastAddr, HEX);
  Serial.println(" (inclusive) ...");

  for (uint8_t addr = firstAddr; addr <= lastAddr; addr++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delayMicroseconds(50);

    uint8_t startResult = SIT5501_I2C.llStart((addr << 1) + 1); // Signal a read
    SIT5501_I2C.stop();

    if (startResult == 0) {
      Serial.print("\rDevice found at 0x");
      Serial.println(addr, HEX);
      Serial.flush();
    }
  }

  */
  do {
    SIT5501_I2C.beginTransmission(DCTCXO_ADDR);
    SIT5501_I2C.write(0x1); // register address
    SIT5501_I2C.write(1<<2); // Output enable, bit 10
    SIT5501_I2C.write(0x0); // LSB, default to zero
    i2c_status = SIT5501_I2C.endTransmission();
    Serial.print("DCTCXO Enable I2c status: 0x");
    Serial.println(i2c_status,HEX);
    delay(500);
  } while (i2c_status != 0 || 1);
  // DCTCXO is enabled

  // set pull range as well 
  SIT5501_I2C.beginTransmission(DCTCXO_ADDR);
  SIT5501_I2C.write(0x2); // register address
  SIT5501_I2C.write(0x0); // not used
  SIT5501_I2C.write(0x9); // +/- 200ppm
  SIT5501_I2C.endTransmission();


  /* FIGURE THIS OUT LATER 
  // read back the max pull range
  Serial.printf("Reading pull range from sitime\r\n");
  Wire.beginTransmission(DCTCXO_ADDR);
  Wire.write(0x2); // register address
  
  if ( Wire.requestFrom(DCTCXO_ADDR, 1) == 1 ) { // read one byte
    sitime_pull_range = Wire.read() & 0xf;
    Serial.printf("Sitime init read back pull range 0x%x\r\n", sitime_pull_range);
  } 
  Wire.endTransmission();
  */
  sitime_pull_range = RNG_200_PPM; // hard code
  Serial.println("Done sitime init\r\n");

}

// takes delta frequency to apply to nominal frequency in Hertz
void apply_freq_change(float del_freq_hz){
  int32_t toRet;

  // frequency_offset is from manual commands
  // del_freq_hz is normal inputs
  float del_freq = ( (del_freq_hz + frequency_offset) / sitime_center_freq) * 1e6;  // in PPM

  if (del_freq > 0) { // positive ppm
    toRet = (int32_t) round( (del_freq / 200) * ( (1 << 25) - 1)); // 200ppm
  } else { // negative ppm
    toRet = (int32_t) round( (del_freq / 200) * (1 << 25)); // 200ppm 
  }

  sprintf(print_buffer, "Writing : %f , 0x%x, %d, %f Hz\r\n", sitime_center_freq, toRet, toRet, del_freq_hz+frequency_offset);
  Serial.print(print_buffer);


  SIT5501_I2C.beginTransmission(DCTCXO_ADDR);
  SIT5501_I2C.write(0x0); // register address

  // Follow fig 37 in SIT5501 datasheet
  SIT5501_I2C.write((toRet >> 8) & 255); // bits [8,15], 65280
  SIT5501_I2C.write(toRet & 255); // bits [0, 7]
  SIT5501_I2C.write((0x1 << 2) | ((toRet >> 24) & 3)); // OE, bits[24, 25]
  SIT5501_I2C.write((toRet >> 16) & 255); // bits [16, 23], 16711680
  int i2c_status = SIT5501_I2C.endTransmission();


  sprintf(print_buffer, "Freq setting Status : 0x%x\r\n", i2c_status);
  Serial.print(print_buffer);

  sprintf(print_buffer, "Current Frequency: %f\r\n", (del_freq_hz + frequency_offset));
  Serial.print(print_buffer);

}

