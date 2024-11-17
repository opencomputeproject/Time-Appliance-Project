
#include "clockmatrix.h"


SoftWire cm_i2c(PLL_SDA, PLL_SCL);
static uint8_t i2c_tx_buf[32];
static uint8_t i2c_rx_buf[32];


// dpll speed up optimization, store current base addr
static uint16_t cur_dpll_base_addr = 0x0;
static uint8_t dpll_addr = 0x58;

bool dpll_read_reg(uint16_t baseaddr, uint16_t offset, uint8_t * val)
{
  uint16_t full_addr;
  uint8_t baseaddr_lower;
  uint8_t baseaddr_upper;
  uint8_t num_read = 0;

  full_addr = baseaddr + offset;
  baseaddr_lower = (uint8_t)(full_addr & 0xff);
  baseaddr_upper = (uint8_t)((full_addr >> 8) & 0xff);

  if ( cur_dpll_base_addr != baseaddr_upper || 1 ) {
    // write base address, DPLL slave addr -> 0xfc -> baseaddr_lower -> baseaddr_upper -> 0x10 -> 0x20
    cm_i2c.beginTransmission(dpll_addr);
    cm_i2c.write(0xfc);
    cm_i2c.write(baseaddr_lower);
    cm_i2c.write(baseaddr_upper);
    cm_i2c.write(0x10);
    cm_i2c.write(0x20);
    if ( cm_i2c.endTransmission() != 0x0 ) {
      Serial.println("Failed to write baseaddr to DPLL in DPLL write!");
      return 0;
    }
    //Serial.println("DPLL read , set base address successfully");
    cur_dpll_base_addr = baseaddr_upper;
    delayMicroseconds(5);
  }
  // read register
  // DPLL slave addr -> baseaddr_lower -> DPLL slave addr request -> value
  cm_i2c.beginTransmission(dpll_addr);
  cm_i2c.write(baseaddr_lower);
  cm_i2c.endTransmission(false);
  delayMicroseconds(5);

  num_read = cm_i2c.requestFrom(dpll_addr,1);
  if ( num_read == 0 ) {
    Serial.println("Failed to read 1 byte from DPLL");
    return 0;
  }
  if ( val != 0 ) {
    *val = (uint8_t) cm_i2c.read(); // read the 1 byte out
  }
  
  //Serial.println("DPLL read success");
  return 1;
}


bool dpll_write_reg(uint16_t baseaddr, uint16_t offset, uint8_t val)
{
  uint16_t full_addr;
  uint8_t baseaddr_lower;
  uint8_t baseaddr_upper;

  full_addr = baseaddr + offset;
  baseaddr_lower = (uint8_t)(full_addr & 0xff);
  baseaddr_upper = (uint8_t)((full_addr >> 8) & 0xff);

  if ( cur_dpll_base_addr != baseaddr_upper || 1 ) {
    // write base address, DPLL slave addr -> 0xfc -> baseaddr_lower -> baseaddr_upper -> 0x10 -> 0x20
    cm_i2c.beginTransmission(dpll_addr);
    cm_i2c.write(0xfc);
    cm_i2c.write(baseaddr_lower);
    cm_i2c.write(baseaddr_upper);
    cm_i2c.write(0x10);
    cm_i2c.write(0x20);
    if ( cm_i2c.endTransmission() != 0x0 ) {
      Serial.println("Failed to write baseaddr to DPLL in DPLL write!");
      return 0;
    } 
    delayMicroseconds(5);
    cur_dpll_base_addr = baseaddr_upper;
  }
  // write register
  // DPLL slave addr -> baseaddr_lower -> value
  cm_i2c.beginTransmission(dpll_addr);
  cm_i2c.write(baseaddr_lower);
  cm_i2c.write(val);
  if ( cm_i2c.endTransmission() != 0x0 ) {
    //Serial.println("DPLL write failed!");
    return 0;
  }
  //Serial.println("DPLL write success");
  return 1;

}



void init_eeprom_addr(int block) {
  uint8_t addr = 0x0;
  static uint8_t eeprom_addr = 0;
  if ( block == 0 ) {
    addr = 0x54;
  } else {
    addr = 0x55;
  }
  if ( eeprom_addr != addr ) {
    // write EEPROM I2C address field of EEPROM block
    dpll_write_reg(0xcf68, 0x0, addr);
    eeprom_addr = addr;
  }
}

bool dpll_write_eeprom(uint32_t addr, uint8_t * data, int count)
{
  // setup eeprom address
  if ( addr > 0xffff ) {
    init_eeprom_addr(1);
  } else {
    init_eeprom_addr(0);
  }
  uint16_t offset = addr & 0xffff;
  // 16-bit address 
  dpll_write_reg(0xcf68, 0x2, offset & 0xff ); // EEPROM_OFFSET_LOW
  dpll_write_reg(0xcf68, 0x3, (offset >> 8) & 0xff ); //EEPROM_OFFSET_HIGH
  // size
  if ( count > 128 ) {
    count = 128;
  }
  dpll_write_reg(0xcf68, 0x1, count); // number of bytes, EEPROM_SIZE

  for ( int i = 0; i < count; i++ ) {
    // write data
    dpll_write_reg(0xcf80, i, data[i]);
  }
  dpll_write_reg(0xcf68, 0x4, 0x2); // EEPROM_CMD_LOW
  dpll_write_reg(0xcf68, 0x5, 0xee); // EEPROM_CMD_HIGH
  return 1;
}


void onDpllWrite(EmbeddedCli *cli, char *args, void *context)
{
  uint16_t baseaddr = 0;
  uint16_t offset = 0;
  uint8_t writeval = 0;
  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("DPLL write no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 3 ) {
    Serial.println("DPLL write needs 3 arguments!");
    return;
  } 

  if ( !try_parse_hex_uint16t( embeddedCliGetToken(args, 1), &baseaddr ) ) {
    Serial.println("Failed to parse first argument for uint16_t");
    return;
  }
  if ( !try_parse_hex_uint16t( embeddedCliGetToken(args, 2), &offset ) ) {
    Serial.println("Failed to parse second argument for uint16_t");
    return;
  }
  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 3), &writeval ) ) {
    Serial.println("Failed to parse third argument for uint8_t");
    return;
  }

  // parsed everything , now do the write


  if ( dpll_write_reg(baseaddr, offset, writeval) ) {
    Serial.println("DPLL write arguments good!");
  } else {
    Serial.println("DPLL write failed!");
  }

}

void onDpllRead(EmbeddedCli *cli, char *args, void *context)
{
  uint16_t baseaddr = 0;
  uint16_t offset = 0;
  uint8_t read_val = 0;
  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("DPLL read no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 2 ) {
    Serial.println("DPLL read needs 2 arguments!");
    return;
  } 

  if ( !try_parse_hex_uint16t( embeddedCliGetToken(args, 1), &baseaddr ) ) {
    Serial.println("Failed to parse first argument for uint16_t");
    return;
  }
  if ( !try_parse_hex_uint16t( embeddedCliGetToken(args, 2), &offset ) ) {
    Serial.println("Failed to parse second argument for uint16_t");
    return;
  }

  // parsed everything , now do the write
  //Serial.println("DPLL read arguments good!");

  if ( dpll_read_reg(baseaddr, offset, &read_val) ) {
    sprintf(print_buffer, "DPLL baseaddr=0x%x offset=0x%x, read back 0x%x\r\n", 
      baseaddr, offset, read_val);
    Serial.print(print_buffer);
  } else {
    Serial.println("DPLL read failed!");
  }
}

static uint8_t eeprom_values[128];
void onDpllEepromWrite(EmbeddedCli *cli, char *args, void *context)
{
  uint32_t addr = 0;
  int value_count = 0;

  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("DPLL EEPROM Write no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) < 2 ) {
    Serial.println("DPLL EEPROM Write needs at least 2 arguments!");
    return;
  } 
  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 1), &addr ) ) {
    Serial.println("Failed to parse first argument for uint32_t");
    return;
  }
  for ( int i = 2; i <= embeddedCliGetTokenCount(args); i++ ) {
    if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, i), &eeprom_values[i-2] ) ) {
      Serial.println("Failed to parse second argument for uint16_t");
      return;
    }
    value_count++;
  }
  if ( dpll_write_eeprom(addr, eeprom_values, value_count) ) {
    sprintf(print_buffer,"DPLL EEPROM Wrote address 0x%lx numvalues %d\r\n", addr, value_count);
    Serial.print(print_buffer);
  } else {
    Serial.println("DPLL EEPROM writing failed");
  }
}


/*
void onDpllEepromRead(EmbeddedCli *cli, char *args, void *context)
{
  uint32_t addr = 0;
  uint8_t read_count = 0;
  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("DPLL read no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 2 ) {
    Serial.println("DPLL read needs 2 arguments!");
    return;
  } 

  if ( !try_parse_hex_uint16t( embeddedCliGetToken(args, 1), &baseaddr ) ) {
    Serial.println("Failed to parse first argument for uint16_t");
    return;
  }
  if ( !try_parse_hex_uint16t( embeddedCliGetToken(args, 2), &offset ) ) {
    Serial.println("Failed to parse second argument for uint16_t");
    return;
  }
}
*/




static Node dpll_write_reg_node = { .name = "dpll-write-reg", 
  .type = MY_FILE, 
  .cliBinding = {
    "dpll-write-reg",
    "Write a DPLL register, pass base address (16-bit) / offset (16-bit) / value (8-bit) ex: dpll-write-reg 0xc03c 0x101 0x0",
    true,
    nullptr,
    onDpllWrite
  } 
};

static Node dpll_read_reg_node = { .name = "dpll-read-reg", 
  .type = MY_FILE, 
  .cliBinding = {
    "dpll-read-reg",
    "Read a DPLL register, pass base address (16-bit) / offset (16-bit)  ex: dpll-read-reg 0xc03c 0x101",
    true,
    nullptr,
    onDpllRead
  }
};

static Node dpll_eeprom_write_node = { .name = "dpll-eeprom-write", 
  .type = MY_FILE, 
  .cliBinding = {
    "dpll-eeprom-write",
    "Write to DPLL EEPROM, pass EEPROM address (17-bit) / list of values up to 128 count (8-bit)  ex: dpll-read-reg 0x1234 0x10 0x10 0x20 0x30....",
    true,
    nullptr,
    onDpllEepromWrite
  }
};



void dpll_dir_operation(EmbeddedCli *cli, char *args, void *context);

static Node * dpll_files[] = { &dpll_write_reg_node, &dpll_read_reg_node, &dpll_eeprom_write_node };

static Node dpll_dir = {
    .name = "dpll",
    .type = MY_DIRECTORY,
    .cliBinding = {"dpll",
          "DPLL mode",
          true,
          nullptr,
          dpll_dir_operation},
    .parent = 0,
    .children = dpll_files,
    .num_children = sizeof(dpll_files) / sizeof(dpll_files[0])
};

void dpll_dir_operation(EmbeddedCli *cli, char *args, void *context) {
  change_to_node(&dpll_dir);
}

// Initialize function to set the parent pointers if needed
void dpll_fs_init() {
  for (int i = 0; i < dpll_dir.num_children; i++) {
    dpll_files[i]->parent = &dpll_dir;
  }
  add_root_filesystem(&dpll_dir);
}







void init_clockmatrix()
{
  Serial.println("Init clock matrix");

  // set reset pin 
  wwvb_gpio_pinmode(PLL_RST, OUTPUT);
  wwvb_digital_write(PLL_RST, 1);

  // probe i2c bus

  cm_i2c.setTimeout_ms(200);
  cm_i2c.setTxBuffer(i2c_tx_buf, 32);
  cm_i2c.setRxBuffer(i2c_rx_buf, 32);
  cm_i2c.begin();

  // clock matrix i2c is at 0x58


  // expose DPLL CLI
  dpll_fs_init();

}