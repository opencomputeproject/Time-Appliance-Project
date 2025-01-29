
#include "clockmatrix.h"


static SoftWire cm_i2c(PLL_SDA, PLL_SCL);
static uint8_t i2c_tx_buf[32];
static uint8_t i2c_rx_buf[32];


// dpll speed up optimization, store current base addr
static uint16_t cur_dpll_base_addr = 0x0;
static uint8_t dpll_addr = 0x58;

void dpll_control_reset(bool reset_pin_val)
{
  wwvb_digital_write(PLL_RST, reset_pin_val);
  wwvb_gpio_pinmode(PLL_RST, OUTPUT);
  wwvb_digital_write(PLL_RST, reset_pin_val);
}

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
  uint8_t read_val = 0;
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

  delay(20); // just fixed delay, reading back status didnt work when I tried it
  
  /*
  // read back EEPROM STATUS and verify it's 0x8000
  // just need to read lower byte
  read_val = 0x1;
  int start_time = millis();
  while ( (millis() - start_time < 100) && read_val != 0x0 ) {
    dpll_read_reg(0xc014, 0x8, &read_val);
  }
  */
  
  return 1;
}

void dpll_read_eeprom(uint32_t addr, uint8_t * data, int count)
{
  uint32_t cur_addr = addr;

  for ( int i = 0; i < count; i++ ) {
    cur_addr = addr + i;
    if ( cur_addr > 0xffff ) {
      init_eeprom_addr(1);
    } else {
      init_eeprom_addr(0);
    }
    uint16_t offset = addr & 0xffff;
    // 16-bit address 
    dpll_write_reg(0xcf68, 0x2, offset & 0xff ); // EEPROM_OFFSET_LOW
    dpll_write_reg(0xcf68, 0x3, (offset >> 8) & 0xff ); //EEPROM_OFFSET_HIGH
    dpll_write_reg(0xcf68, 0x1, 1); // number of bytes, EEPROM_SIZE
    dpll_write_reg(0xcf68, 0x4, 0x1); // EEPROM_CMD_LOW
    dpll_write_reg(0xcf68, 0x5, 0xee); // EEPROM_CMD_HIGH

    delay(10); 
    // read back data
    dpll_read_reg(0xcf80, 0x0, &data[i]);
  }
  return;

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

void onDpllEepromRead(EmbeddedCli *cli, char *args, void *context)
{
  uint32_t addr = 0;
  int value_count = 0;
  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("DPLL EEPROM Read no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) < 2 ) {
    Serial.println("DPLL EEPROM Read needs at least 2 arguments!");
    return;
  } 
  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 1), &addr ) ) {
    Serial.println("Failed to parse first argument for uint32_t");
    return;
  }
  value_count = atoi( embeddedCliGetToken(args, 2) );

  dpll_read_eeprom(addr, eeprom_values, value_count);

  for ( int i = 0; i < value_count; i++ ) {
    if ( i % 16 == 0 ) {
      sprintf(print_buffer, "0x%08X: ", addr + i);
      Serial.print(print_buffer);
    }
    sprintf(print_buffer, "%02X ", eeprom_values[i]);
    Serial.print(print_buffer);
    if ( i % 16 == 15 ) Serial.println("");
  }
  Serial.println("");

}

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


/************* Xmodem for flashing DPLL EEPROM ************/
// If this time takes too long still and you want to optimize
// Do 128 bytes of EEPROM writes at a time
// right now its basically 16 bytes at a time, then 10 millisecond delay
// app note says you can do 128 bytes at a time and then add some delay, they use 100ms
// there's overhead to every eeprom access so this may speed it up some

XModem xmodem_cm;

// Global variable to track the base address from type 04 records
static uint32_t flash_addr = 0;
uint8_t first_block[128];
uint8_t last_block[128];

// Buffer size for holding partial lines
#define LINE_BUFFER_SIZE 128

// Partial line buffer to hold unprocessed data
static char line_buffer[LINE_BUFFER_SIZE];
static int line_buffer_index = 0;

char first_line_debug[128];
bool got_first_line = 0;
char last_line_debug[128];
// Function to process a line from the Intel HEX file
bool processHexFileLine(const char *line) {

  char temp_string[16];
  // Ensure the line starts with ':'
  if (line[0] != ':') {
      return false;
  }
  if ( !got_first_line ) {
    strcpy(first_line_debug, line);
    got_first_line = 1;
  }
  strcpy(last_line_debug, line);
  // Parse the record length, address, and record type
  temp_string[0] = *(line + 1);
  temp_string[1] = *(line + 2);
  temp_string[2] = 0;
  uint8_t byteCount = strtoul(temp_string, NULL, 16);
  temp_string[0] = *(line + 3);
  temp_string[1] = *(line + 4);
  temp_string[2] = *(line + 5);
  temp_string[3] = *(line + 6);
  temp_string[4] = 0;
  uint16_t offsetAddress = strtoul(temp_string, NULL, 16);
  temp_string[0] = *(line + 7);
  temp_string[1] = *(line + 8);
  temp_string[2] = 0;
  uint8_t recordType = strtoul(temp_string, NULL, 16);

  // Parse data bytes
  uint8_t data[16];  // Buffer for up to 16 bytes of data

  for (uint8_t i = 0; i < byteCount; i++) {
    temp_string[0] = *(line + 9 + (i * 2) );
    temp_string[1] = *(line + 10 + (i * 2) );
    temp_string[2] = 0;
    data[i] = strtoul(temp_string, NULL, 16);
  }
  // ignoring checksum

  // Handle the different record types
  switch (recordType) {
      case 0x00: {  // Data Record
          uint32_t fullAddress = flash_addr + offsetAddress;
          dpll_write_eeprom(fullAddress, data, byteCount);
          break;
      }
      case 0x04: {  // Extended Linear Address Record
          uint16_t highAddress = (data[0] << 8) | data[1];
          flash_addr = ((uint32_t)highAddress) << 16;
          break;
      }
      case 0x01: {  // End-of-File Record
          // No specific action needed for EOF
          return true;
          break;
      }
      default:
          // Unsupported record type, ignore
          break;
  }
  return false;
}

int line_count = 0; // debug
bool process_block_cm(void *blk_id, size_t idSize, byte *data, size_t dataSize) {

  if ( line_count == 0 ) {
    memcpy(first_block, data, dataSize);
  }
  memcpy(last_block, data, dataSize);
  if ( line_count == 2880 ) {
    
    //return false;
  }
  
  line_count++;

  /*** Logic, use single buffer line_buffer, indexed by line_buffer_index
  1. copy characters into line buffer from data until you find '\n'
  2. once you find another ':', null terminate line buffer, try to parse it as a string
  3. then reset line buffer and start more
  ***/
  for ( int i = 0; i < dataSize; i++ )
  {
    // check if character is '\n'
    if ( data[i] == '\n' ) {
      line_buffer[line_buffer_index] = 0; // null terminate current string
      if ( processHexFileLine(line_buffer) ) { // return true on final record
        return false; // return false, got end of file
      } // process the line
      line_buffer_index = 0; // reset this
    } else {
      // most common case, just normal copy character
      line_buffer[line_buffer_index++] = data[i];
    }
  }

  return true; 
}


void onDpllXmodemEeprom(EmbeddedCli *cli, char *args, void *context)
{

  sprintf(print_buffer, "Xmodem transfer to DPLL EEPROM hex file\r\n");
  Serial.print(print_buffer);
  flash_addr = 0;
  line_buffer_index = 0;
  line_count = 0;
  memset(first_line_debug, 0, 128);
  memset(last_line_debug, 0, 128);
  xmodem_cm.begin(Serial, XModem::ProtocolType::XMODEM);
  xmodem_cm.setRecieveBlockHandler(process_block_cm);
  while ( xmodem_cm.receive() ) {}
  delay(1000);
  Serial.println("Done with DPLL Xmodem flash!");  
  
  Serial.println("First bytes:");
  for ( int i = 0; i < 128; i++ ) {
    sprintf(print_buffer, "%02X ", first_block[i]);
    Serial.print(print_buffer);
    if ( i % 16 == 15 ) Serial.println("");
  }
  Serial.println("");
  Serial.println("Last bytes:");
  for ( int i = 0; i < 128; i++ ) {
    sprintf(print_buffer, "%02X ", last_block[i]);
    Serial.print(print_buffer);
    if ( i % 16 == 15 ) Serial.println("");
  }
  Serial.println("");

  sprintf(print_buffer, "First line: %s\r\n", first_line_debug);
  Serial.print(print_buffer);
  sprintf(print_buffer, "Last line: %s\r\n", last_line_debug);
  Serial.print(print_buffer);
}


#ifdef INTERNAL_DPLL_FIRMWARE
void onDpllInternalFlashEeprom(EmbeddedCli *cli, char *args, void *context)
{
  Serial.println("Starting eeprom flashing from internal file");

  uint8_t val = 0;
  for ( uint32_t i = 0; i < sizeof(dpllFirmware); i += 16 ) {
    Serial.println("");
    sprintf(print_buffer, "%08X: ", i); Serial.print(print_buffer);
    for ( int j = 0; j < 16; j++ ) {
      sprintf(print_buffer, "%02X ", dpllFirmware[i+j]);
      Serial.print(print_buffer);
    }
    //val = dpllFirmware[i];
    dpll_write_eeprom(i, ( (uint8_t*) &dpllFirmware[i] ), 16);

  }

  Serial.println("");
  Serial.println("Done DPLL EEPROM flash!");

}
#endif

void onDpllResetControl(EmbeddedCli *cli, char *args, void *context)
{
  if ( embeddedCliGetTokenCount(args) != 1 )
  {
    Serial.println("DPLL Reset control needs 1 argument!");
    return;
  }
  bool value;
  value = (bool) (atoi( embeddedCliGetToken(args, 1) ) );
  if ( value ) {
    Serial.println("Setting reset to 1");
  } else {
    Serial.println("Setting reset to 0");
  }
  dpll_control_reset(value);

}

static Node dpll_reset_control_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "reset_control",
    "Control reset pin, pass value to set reset pin",
    true,
    nullptr,
    onDpllResetControl
  } 
};

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
    "Write to DPLL EEPROM, pass EEPROM address (16-bit) / list of values up to 128 count (8-bit)  ex: dpll-eeprom-write 0x1234 0x10 0x10 0x20 0x30....",
    true,
    nullptr,
    onDpllEepromWrite
  }
};

static Node dpll_eeprom_read_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "dpll-eeprom-read",
    "Read from DPLL EEPROM, pass EEPROM address (16-bit) / count of values to read  ex: dpll-eeprom-read 0x1234 10",
    true,
    nullptr,
    onDpllEepromRead
  }
};

static Node dpll_xmodem_eeprom_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "xmodem_eeprom",
    "Write to DPLL EEPROM via Xmodem, pass in .hex file generated from Timing Commander",
    true,
    nullptr,
    onDpllXmodemEeprom
  }
};

#ifdef INTERNAL_DPLL_FIRMWARE
static Node dpll_eeprom_int_flash_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "internal_eeprom_flash",
    "Write to DPLL EEPROM with header file stored",
    true,
    nullptr,
    onDpllInternalFlashEeprom
  }
};
#endif

void dpll_dir_operation(EmbeddedCli *cli, char *args, void *context);

static Node * dpll_files[] = { &dpll_write_reg_node, &dpll_read_reg_node, &dpll_eeprom_write_node,
  &dpll_xmodem_eeprom_node, 
  &dpll_reset_control_node,
#ifdef INTERNAL_DPLL_FIRMWARE
  &dpll_eeprom_int_flash_node
#endif
   };

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

  /* DONT DO THIS, interrupts normal DPLL bootup from eeprom
  // set reset pin 
  wwvb_gpio_pinmode(PLL_RST, OUTPUT);
  wwvb_digital_write(PLL_RST, 1);
  delay(100);
  wwvb_digital_write(PLL_RST, 0);
  delay(100);
  wwvb_digital_write(PLL_RST, 1);
  delay(100);
  */
  //delay(150); // add some delay for clockmatrix to download image
  // maybe not necessary, but helps with cold boot 


  // probe i2c bus

  cm_i2c.setTimeout_ms(200);
  cm_i2c.setTxBuffer(i2c_tx_buf, 32);
  cm_i2c.setRxBuffer(i2c_rx_buf, 32);
  cm_i2c.begin();

  // clock matrix i2c is at 0x58


  // expose DPLL CLI
  dpll_fs_init();
  Serial.println("Clock matrix initialized!");

}