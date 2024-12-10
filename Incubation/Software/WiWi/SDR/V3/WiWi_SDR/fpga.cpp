#include "fpga.h"


static SoftWire fpga_i2c(PLL_SDA, PLL_SCL);
static uint8_t i2c_tx_buf[32];
static uint8_t i2c_rx_buf[32];
static uint8_t fpga_i2c_address = 0x8;


bool fpga_i2c_read(uint8_t address, uint8_t * val)
{
  uint8_t num_read = 0;
  fpga_i2c.beginTransmission(fpga_i2c_address);
  fpga_i2c.write(address);
  fpga_i2c.endTransmission(false);
  delayMicroseconds(5);

  num_read = fpga_i2c.requestFrom(fpga_i2c_address,1);
  if ( num_read == 0 ) {
    Serial.println("Failed to read 1 byte from DPLL");
    return 0;
  }
  if ( val != 0 ) {
    *val = (uint8_t) fpga_i2c.read(); // read the 1 byte out
  }
  return 1;
}

bool fpga_i2c_write(uint8_t address, uint8_t val)
{
  fpga_i2c.beginTransmission(fpga_i2c_address);
  fpga_i2c.write(address);
  fpga_i2c.write(val);
  if ( fpga_i2c.endTransmission() != 0x0 ) {
    Serial.println("Failed to write to FPGA!");
    return 0;
  }
  return 1;
}

void enable_subg_shift()
{
  wwvb_gpio_pinmode(STM_FPGA_SPARE1, OUTPUT);
  wwvb_digital_write(STM_FPGA_SPARE1, 1);
}
void disable_subg_shift()
{
  wwvb_gpio_pinmode(STM_FPGA_SPARE1, OUTPUT);
  wwvb_digital_write(STM_FPGA_SPARE1, 0);
}
void enable_wifi_shift()
{
  wwvb_gpio_pinmode(STM_FPGA_SPARE2, OUTPUT);
  wwvb_digital_write(STM_FPGA_SPARE2, 1);
}
void disable_wifi_shift()
{
  wwvb_gpio_pinmode(STM_FPGA_SPARE2, OUTPUT);
  wwvb_digital_write(STM_FPGA_SPARE2, 0);
}



/******************** CLI Functions **********************/


void onFpgaProgramN(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t value = 0;

  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("Fpga ProgramN no arguments!");
    return;
  } 

  if ( embeddedCliGetTokenCount(args) == 0 ) {
    // read back programN value
  } else if ( embeddedCliGetTokenCount(args) == 1 ) {
    value = atoi(embeddedCliGetToken(args, 1));
    if ( value ) {
      Serial.println("FPGA ProgramN Setting high!");
      wwvb_gpio_pinmode(FPGA_PROGRAMN, OUTPUT);
      wwvb_digital_write(FPGA_PROGRAMN, 1);
    } else {
      Serial.println("FPGA ProgramN setting low!");
      wwvb_gpio_pinmode(FPGA_PROGRAMN, OUTPUT);
      wwvb_digital_write(FPGA_PROGRAMN, 0);
    }
  } else {
    Serial.println("FPGA ProgramN needs 1 argument!");
  }

}

void onFpgaStatus(EmbeddedCli *cli, char *args, void *context)
{

  sprintf(print_buffer, "FPGA InitN value: %d\r\n", wwvb_digital_read(FPGA_INITN)); 
  Serial.print(print_buffer);
  sprintf(print_buffer, "FPGA Done value: %d\r\n", wwvb_digital_read(FPGA_DONE)); 
  Serial.print(print_buffer);
}

void onFpgaI2cWrite(EmbeddedCli *cli, char *args, void *context)
{
  if (embeddedCliGetTokenCount(args) != 2) {
    Serial.println("Fpga I2c write needs 2 arguments!");
    return;
  } 
  uint8_t address = 0;
  uint8_t value = 0;
  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 1), &address ) ) {
    Serial.println("Failed to parse first argument for uint8_t");
    return;
  } 
  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 2), &value ) ) {
    Serial.println("Failed to parse second argument for uint8_t");
    return;
  }  
  sprintf(print_buffer, "Writing I2C FPGA Register 0x%02X = 0x%02X\r\n", address, value);
  Serial.print(print_buffer);

  if ( fpga_i2c_write(address, value) ) {
    Serial.println("Write success!");
  } else {
    Serial.println("Write failed!");
  }
}

void onFpgaI2cRead(EmbeddedCli *cli, char *args, void *context)
{
  if (embeddedCliGetTokenCount(args) != 1) {
    Serial.println("Fpga I2c read needs 1 argument!");
    return;
  } 
  uint8_t address = 0;
  uint8_t value = 0;
  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 1), &address ) ) {
    Serial.println("Failed to parse first argument for uint8_t");
    return;
  } 
  if ( fpga_i2c_read(address, &value) ) {
    sprintf(print_buffer, "I2C FPGA Register 0x%02X = 0x%02X\r\n", address, value);
    Serial.print(print_buffer);
  } else {
    Serial.println("Read failed!");
  }
}

void fpga_restart()
{
  
  // A little hacky, but need to do this to make it work
  // Disable QSPI interface
  // toggle FPGA program n, wait for some time for for done to go high
  // then re-enable QSPI interface
  Serial.println("Bringing up FPGA, work-around for QSPI conflict");
  wwvb_gpio_pinmode(FPGA_PROGRAMN, OUTPUT);
  wwvb_digital_write(FPGA_PROGRAMN, 0); // hold FPGA in programn mode to make sure

  // disable QSPI interface, stm32 clock output is always driving, conflicts with FPGA master
  QSPI_Disable();

  delay(10); // small delay

  // re-enable programn
  wwvb_digital_write(FPGA_PROGRAMN, 1);

  int start_time = millis();
  Serial.println("Waiting for FPGA download");
  while ( (millis() - start_time < 2000 ) && !wwvb_digital_read(FPGA_DONE) ) {
    delay(10);
  } // 2 second timeout

  if ( !wwvb_digital_read(FPGA_DONE) ) {
    Serial.println("FPGA did not download, likely needs firmware update!");
  } else {
    Serial.println("FPGA download successful!");
  }

  // re-enable QSPI interface
  BSP_QSPI_Init();

  wwvb_digital_write(STM_FPGA_SPARE5, 0);
  delay(10);
  wwvb_digital_write(STM_FPGA_SPARE5, 1);
  delay(10);


}

void onFpgaRestart(EmbeddedCli *cli, char *args, void *context)
{
  fpga_restart();  
}

void onFpgaUpdateXmodem(EmbeddedCli *cli, char *args, void *context)
{
  int total_bytes = 0;
  
  if ( embeddedCliGetTokenCount(args) != 1 ) {
    Serial.println("FPGA Update over Xmodem, No argument using default file size 582683");
    total_bytes = 582683;
  } else {
    total_bytes = atoi( embeddedCliGetToken(args, 1) );
    if ( total_bytes == 0 ) {
      Serial.println("Xmodem flash didn't receive number of bytes!");
      return;
    }
  }

  wwvb_gpio_pinmode(FPGA_PROGRAMN, OUTPUT);
  wwvb_digital_write(FPGA_PROGRAMN, 0); // hold FPGA in programn mode to make sure


  start_xmodem_flash(total_bytes);

  fpga_restart(); // do full fpga restart process after flashing new image

  Serial.println("Done FPGA update!");

  

}

static Node fpga_update_xmodem_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "update_xmodem",
    "Update FPGA image with .bin file sent over xmodem",
    true,
    nullptr,
    onFpgaUpdateXmodem
  }
};

static Node fpga_programn_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "manual_programn",
    "Manually control the programn fpga pin",
    true,
    nullptr,
    onFpgaProgramN
  }
};
static Node fpga_restart_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "restart",
    "Forces fpga redownload and reset",
    true,
    nullptr,
    onFpgaRestart
  }
};

static Node fpga_status_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "status",
    "Print status of FPGA",
    true,
    nullptr,
    onFpgaStatus
  }
};

static Node fpga_i2c_write_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "write_i2c",
    "Write a register on the FPGA over i2c",
    true,
    nullptr,
    onFpgaI2cWrite
  }
};

static Node fpga_i2c_read_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "read_i2c",
    "Read a register on the FPGA over i2c",
    true,
    nullptr,
    onFpgaI2cRead
  }
};


/********************* Boiler plate CLI stuff ***********/

void fpga_dir_operation(EmbeddedCli *cli, char *args, void *context); // forward declaration


static Node * fpga_files[] = { &fpga_programn_node, &fpga_status_node, &fpga_i2c_write_node, 
  &fpga_i2c_read_node, &fpga_restart_node, &fpga_update_xmodem_node };

static Node fpga_dir = {
    .name = "fpga",
    .type = MY_DIRECTORY,
    .cliBinding = {"fpga",
          "fpga mode",
          true,
          nullptr,
          fpga_dir_operation},
    .parent = 0,
    .children = fpga_files,
    .num_children = sizeof(fpga_files) / sizeof(fpga_files[0])
};

void fpga_dir_operation(EmbeddedCli *cli, char *args, void *context) {
  change_to_node(&fpga_dir);
}

// Initialize function to set the parent pointers if needed
void fpga_fs_init() {
  for (int i = 0; i < fpga_dir.num_children; i++) {
    fpga_files[i]->parent = &fpga_dir;
  }
  add_root_filesystem(&fpga_dir);
}







void init_fpga_cli()
{
  //wwvb_gpio_pinmode(FPGA_PROGRAMN, INPUT);
  wwvb_gpio_pinmode(FPGA_INITN, INPUT);
  wwvb_gpio_pinmode(FPGA_DONE, INPUT);
  wwvb_gpio_pinmode(FPGA_PROGRAMN, OUTPUT);
  wwvb_digital_write(FPGA_PROGRAMN, 0); // hold FPGA in reset by default
  fpga_fs_init();

  wwvb_gpio_pinmode(STM_FPGA_SPARE5, OUTPUT); // SPARE5 is logical reset to FPGA, active low
  wwvb_digital_write(STM_FPGA_SPARE5, 0);
  
  // setup the spares
  wwvb_gpio_pinmode(STM_FPGA_SPARE1, INPUT);
  wwvb_gpio_pinmode(STM_FPGA_SPARE2, INPUT);
  wwvb_gpio_pinmode(STM_FPGA_SPARE3, INPUT);
  wwvb_gpio_pinmode(STM_FPGA_SPARE4, INPUT);
  
  wwvb_gpio_pinmode(STM_FPGA_SPARE6, INPUT);


  fpga_restart(); 


  // initialize the i2c controller to the fpga   
  fpga_i2c.setTimeout_ms(200);
  fpga_i2c.setTxBuffer(i2c_tx_buf, 32);
  fpga_i2c.setRxBuffer(i2c_rx_buf, 32);
  fpga_i2c.begin();

  enable_subg_shift();
  enable_wifi_shift();

}