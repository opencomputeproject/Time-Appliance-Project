#include "SI5341B.h"


static uint8_t cur_page = 0xff;
static uint8_t i2c_status = 0xff;

static uint8_t i2c_tx_buf[32];
static uint8_t i2c_rx_buf[32];
static SoftWire SI5341_I2C(PLL_SDA, PLL_SCL);

int si5341b_write_reg(uint8_t page, uint8_t reg, uint8_t val) {
  if ( page != cur_page ) {
    // write page
    SI5341_I2C.beginTransmission(SI5341_I2C_ADDR);
    SI5341_I2C.write(PAGE_REG);
    SI5341_I2C.write(page);
    i2c_status = SI5341_I2C.endTransmission();
    if ( i2c_status != 0x0 ) {
      Serial.print("Si5341b write reg page write failed\r\n");
      return i2c_status;
    }
    cur_page = page;
  }

  SI5341_I2C.beginTransmission(SI5341_I2C_ADDR);
  SI5341_I2C.write(reg);
  SI5341_I2C.write(val);
  i2c_status = SI5341_I2C.endTransmission();
  if ( i2c_status != 0x0 ) {
    Serial.print("Si5341b write reg failed\r\n");
  }
  return i2c_status;
}
int si5341b_read_reg(uint8_t page, uint8_t reg, uint8_t * val) {
  if ( val == 0 ) return -1;

  if ( page != cur_page ) {
    // write page
    SI5341_I2C.beginTransmission(SI5341_I2C_ADDR);
    SI5341_I2C.write(PAGE_REG);
    SI5341_I2C.write(page);
    i2c_status = SI5341_I2C.endTransmission();
    if ( i2c_status != 0x0 ) {
      Serial.print("Si5341b read reg page write failed\r\n");
      return i2c_status;
    }
    cur_page = page;
  }

  SI5341_I2C.beginTransmission(SI5341_I2C_ADDR);
  SI5341_I2C.write(reg);
  SI5341_I2C.endTransmission(false);

  SI5341_I2C.requestFrom(SI5341_I2C_ADDR, 1);
  byte readByte = SI5341_I2C.read();
  //i2c_status = SI5341_I2C.endTransmission();
  //if ( i2c_status != 0x0 ) {
  //  Serial.print("Si5341b read reg failed\r\n");
  //} else {
  *val = (uint8_t) readByte;
  //}
  return 0;
}

int si5341b_print_reg(uint8_t page, uint8_t reg) {
  uint8_t val = 0;
  int ret_val = 0;
  ret_val = si5341b_read_reg(page, reg, &val);
  Serial.print("SI5341B page 0x");
  Serial.print(page, HEX);
  Serial.print(" reg = 0x");
  Serial.print(reg, HEX);

  if ( ret_val != 0 ){
    Serial.println(" FAILED");
    return ret_val;
  }
  Serial.print(" => 0x");
  Serial.println(val,HEX);
}


bool init_si5341b()
{

  int i2c_status = 0;

  wwvb_gpio_pinmode(PLL_RST, OUTPUT);
  // toggle reset quick
  wwvb_digital_write(PLL_RST, LOW); 
  delay(10);
  wwvb_digital_write(PLL_RST, HIGH);
  delay(1000);

  wwvb_gpio_pinmode(PLL_SDA, INPUT);
  wwvb_gpio_pinmode(PLL_SCL, INPUT);
  // just do i2c scan during debug
  //https://github.com/stevemarple/SoftWire/blob/master/examples/ListDevices/ListDevices.ino
  SI5341_I2C.setTimeout_ms(40);
  SI5341_I2C.enablePullups(0);
  SI5341_I2C.setTxBuffer(i2c_tx_buf, 32);
  SI5341_I2C.setRxBuffer(i2c_rx_buf, 32);
  SI5341_I2C.begin();

  // try to read a register
  uint8_t id_reg0 = 0;
  uint8_t id_reg1 = 0;
  if ( si5341b_read_reg(0x0, 0x2, &id_reg0) == 0 &&
        si5341b_read_reg(0x0, 0x3, &id_reg1) == 0 )
  {
    if ( id_reg0 == 0x41 && id_reg1 == 0x53 ) {
      Serial.println("Si5341B ID register check PASS!");
    } else {
      Serial.println("Si5341B ID registers not what expected, FAIL!");
      return 1;
    }

  } else {
    Serial.println("Si5341B ID register check failed to read, FAIL!");
    return 1;
  }

  si5341b_print_reg(0,0);
  si5341b_print_reg(0,4);
  si5341b_print_reg(0,5);
  si5341b_print_reg(0,0xa);
  si5341b_print_reg(0,0xb);
  si5341b_print_reg(0,0xc);



  // now write registers
  uint8_t page = 0;
  uint8_t reg = 0;
  uint8_t val = 0;
  bool found_first_data_reg = 0;
  for (int i = 0; i < SI5341_REVD_REG_CONFIG_NUM_REGS; i++ ) {
    val = si5341_revd_registers[i].value;
    page = (uint8_t) ((si5341_revd_registers[i].address & 0xff00) >> 8);
    reg = (uint8_t) ( si5341_revd_registers[i].address & 0xff );

    if ( page == 0x0 && !found_first_data_reg ){
      Serial.println("Waiting before writing first data registers");
      found_first_data_reg = 1;
      delay(300); // according to register header file, need delay before first data registers
    }
    Serial.print("SI5341B Write config ");
    Serial.print(i);
    Serial.print(", Page=0x");
    Serial.print(page,HEX);
    Serial.print(", reg=0x");
    Serial.print(reg, HEX);
    Serial.print(" = 0x");
    Serial.print(val, HEX);

    if ( si5341b_write_reg(page, reg, val) != 0x0 ) {
      Serial.println(" FAILED");
      return 1;
    } else {
      Serial.println("");
    }
  }



  Serial.print("Si5341B init end\r\n");

  return 0;
  
}