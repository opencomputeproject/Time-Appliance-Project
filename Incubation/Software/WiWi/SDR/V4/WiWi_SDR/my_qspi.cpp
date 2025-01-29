#include "my_qspi.h"



/*************** QSPI STM32 APIs and functions ********/

// QSPI handle (global variable)
QSPI_HandleTypeDef hqspi;


// Constants for QSPI flash memory
#define PAGE_SIZE 256        // Maximum bytes per QSPI page write
#define SECTOR_SIZE 4096     // QSPI sector size for erases


// JEDEC Commands for QSPI Flash
#define READ_CMD             0x03    // Standard Read command
#define FAST_READ_CMD        0x0B    // Fast Read command
#define PAGE_PROG_CMD        0x02    // Page Program command
#define SECTOR_ERASE_CMD     0x20    // Erase a 4KB sector
#define BLOCK_ERASE_CMD      0xD8    // Erase a 64KB block
#define CHIP_ERASE_CMD       0xC7    // Chip Erase command
#define WRITE_ENABLE_CMD     0x06    // Write Enable command
#define WRITE_DISABLE_CMD    0x04    // Write Disable command
#define READ_STATUS_REG_CMD  0x05    // Read Status Register command
#define READ_ID_CMD          0x9F    // Read JEDEC ID command



QSPI_CommandTypeDef s_command;


void QSPI_WaitForWriteCompletion(void) {
  uint8_t reg;
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = 0x05; // Read Status Register command
  s_command.AddressMode = QSPI_ADDRESS_NONE;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.DummyCycles = 0;
  s_command.NbData = 1;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  do {
      if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        Serial.println("QSPI WaitForWrite failed on command!");
        return;
      }
      if (HAL_QSPI_Receive(&hqspi, &reg, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        Serial.println("QSPI WaitforWrite failed on receive!");
        return;
      }
  } while (reg & 0x01); // Check if write/erase is in progress
}

void QSPI_Read(uint8_t *pData, uint32_t address, uint32_t size) {
  // Configure the read command
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = 0x03; // Standard SPI Read command (datasheet-specific)
  s_command.AddressMode = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  s_command.Address = address;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.DummyCycles = 0;
  s_command.NbData = size;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // Execute the command
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    Serial.println("QSPI Read failed on command!");
    return;
  }

  // Receive the data
  if (HAL_QSPI_Receive(&hqspi, pData, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    Serial.println("QSPI Read failed on receive!");
  }
}

// Write Enable API
void QSPI_WriteEnable(void) {
  // Configure the Write Enable (WREN) command
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE; // Command sent on 1 line
  s_command.Instruction = 0x06; // WREN command (refer to datasheet)
  s_command.AddressMode = QSPI_ADDRESS_NONE; // No address phase
  s_command.DataMode = QSPI_DATA_NONE; // No data phase
  s_command.DummyCycles = 0; // No dummy cycles
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // Send the WREN command
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    Serial.println("QSPI WriteEnable failed on command!");
    return;
  }
  /*
  // Optionally, check if WEL bit is set in the status register
  uint8_t reg;
  s_command.Instruction = 0x05; // Read Status Register (RDSR) command
  s_command.NbData = 1; // Read 1 byte
  s_command.DataMode = QSPI_DATA_1_LINE;

  do {
      if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
          Error_Handler();
      }
      if (HAL_QSPI_Receive(&hqspi, &reg, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
          Error_Handler();
      }
  } while ((reg & 0x02) == 0); // Wait until WEL bit (bit 1) is set
  */
}




void QSPI_Erase(uint32_t address) {

  QSPI_WriteEnable();
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = 0x20; // Sector Erase command (datasheet-specific)
  s_command.AddressMode = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  s_command.Address = address;
  s_command.DataMode = QSPI_DATA_NONE;
  s_command.DummyCycles = 0;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // Execute the command
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    Serial.println("QSPI Erase failed on command!");
    return;
  }

  // Wait for the erase to complete
  QSPI_WaitForWriteCompletion();
}

void QSPI_FullChipErase() {

  QSPI_WriteEnable();

   // Configure the Write Enable (WREN) command
  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE; // Command sent on 1 line
  s_command.Instruction = 0x60; // WREN command (refer to datasheet)
  s_command.AddressMode = QSPI_ADDRESS_NONE; // No address phase
  s_command.DataMode = QSPI_DATA_NONE; // No data phase
  s_command.DummyCycles = 0; // No dummy cycles
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // Send the WREN command
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    Serial.println("QSPI WriteEnable failed on command!");
    return;
  }

  QSPI_WaitForWriteCompletion();

}

int QSPI_Write(uint8_t *pData, uint32_t address, uint32_t size) {

  QSPI_WriteEnable();

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
  s_command.Instruction = 0x02; // Page Program command (datasheet-specific)
  s_command.AddressMode = QSPI_ADDRESS_1_LINE;
  s_command.AddressSize = QSPI_ADDRESS_24_BITS;
  s_command.Address = address;
  s_command.DataMode = QSPI_DATA_1_LINE;
  s_command.DummyCycles = 0;
  s_command.NbData = size;
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

  // Execute the command
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    Serial.println("QSPI Write failed on command!");
    return -1;
  }

  // Transmit the data
  if (HAL_QSPI_Transmit(&hqspi, pData, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    Serial.println("QSPI write failed on transmit!");
    return -1;
  }
  QSPI_WaitForWriteCompletion();
  //delay(2); // data sheet page program 1.2ms max, putting 2 here
  return 0;
}


int QSPI_Write_PageSafe(uint8_t *pData, uint32_t address, uint32_t size) {

  /*
  // very lazy code
  for ( int i = 0; i < size; i++ ) {
    QSPI_Write(pData+i, address+i, 1);
  }
  return 0;
  */


    uint32_t current_address = address;  // Start address for the write operation
    uint32_t bytes_left = size;          // Total bytes left to write

    while (bytes_left > 0) {
        // Calculate the offset within the current page
        uint32_t page_offset = current_address % PAGE_SIZE;

        // Calculate the space available in the current page
        // For example:
        // - If current_address = 0x12FE, page_offset = 254, space_in_page = 256 - 254 = 2
        // - If current_address = 0x1300, page_offset = 0, space_in_page = 256
        uint32_t space_in_page = PAGE_SIZE - page_offset;

        // Determine how many bytes to write in this iteration:
        // - We can write up to `bytes_left` (remaining data) or `space_in_page` (space in the current page), whichever is smaller
        uint32_t bytes_to_write = (bytes_left < space_in_page) ? bytes_left : space_in_page;

        // Perform the write operation using the raw QSPI_Write API
        int status = QSPI_Write(pData, current_address, bytes_to_write);
        if (status != 0) {
            return status;  // Return an error code if the write operation fails
        }

        // Update the current address for the next write iteration
        // - Move forward by `bytes_to_write`
        current_address += bytes_to_write;

        // Advance the pointer to the remaining data in the buffer
        // - Skip the bytes that were just written
        pData += bytes_to_write;

        // Update the number of bytes left to write
        // - Subtract the number of bytes just written
        bytes_left -= bytes_to_write;
    }

    return 0;  // Success
}


void QSPI_ReadChipID(void) {
    uint8_t chip_id[3]; // JEDEC ID is typically 3 bytes: Manufacturer, Memory Type, Capacity

    // Configure the read ID command
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE; // Command sent on 1 line
    s_command.Instruction = 0x9F; // JEDEC ID Read command
    s_command.AddressMode = QSPI_ADDRESS_NONE; // No address phase
    s_command.DataMode = QSPI_DATA_1_LINE; // Data received on 1 line
    s_command.DummyCycles = 0; // No dummy cycles
    s_command.NbData = sizeof(chip_id); // Number of bytes to read
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    // Send the command
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      Serial.println("QSPI ReadChipID failed on command!");
      return;
    }

    // Receive the JEDEC ID
    if (HAL_QSPI_Receive(&hqspi, chip_id, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      Serial.println("QSPI ReadChipID failed on receive!");
      return;
    }

    // Print the JEDEC ID
    sprintf(print_buffer, "JEDEC ID: Manufacturer: 0x%02X, Memory Type: 0x%02X, Capacity: 0x%02X\r\n",
           chip_id[0], chip_id[1], chip_id[2]);
    Serial.print(print_buffer);
}

void QSPI_Disable()
{
  // disconnect all the pins for QSPI interface
  wwvb_gpio_pinmode(QSPI_FPGA_SCLK, INPUT);
  wwvb_gpio_pinmode(QSPI_FPGA_CS, INPUT);
  wwvb_gpio_pinmode(QSPI_FPGA_MOSI, INPUT);
  wwvb_gpio_pinmode(QSPI_FPGA_MISO, INPUT);
  wwvb_gpio_pinmode(QSPI_FPGA_WP, INPUT);
  wwvb_gpio_pinmode(QSPI_FPGA_RESET, INPUT);
  __HAL_RCC_QSPI_FORCE_RESET();
}

void QSPI_MspInit()
{

  __HAL_RCC_QSPI_FORCE_RESET();
  __HAL_RCC_QSPI_RELEASE_RESET();

  // setup QSPI pins
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // 2. Configure QSPI CLK (Clock Pin)
  GPIO_InitStruct.Pin       = QSPI_FPGA_SCLK_N;  // Example pin, adjust based on your hardware
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;  // AF for QSPI on STM32H7
  HAL_GPIO_Init(QSPI_FPGA_SCLK_G, &GPIO_InitStruct);  // Initialize on GPIOB

  // 3. Configure QSPI NCS (Chip Select Pin)
  GPIO_InitStruct.Pin       = QSPI_FPGA_CS_N;   // Example pin, adjust based on your hardware
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;  // Alternate function for QSPI
  HAL_GPIO_Init(QSPI_FPGA_CS_G, &GPIO_InitStruct);  // Initialize on GPIOB

  // 4. Configure QSPI DQ0 (Data Line 0)
  GPIO_InitStruct.Pin       = QSPI_FPGA_MOSI_N;  // Example pin, adjust based on your hardware
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
  HAL_GPIO_Init(QSPI_FPGA_MOSI_G, &GPIO_InitStruct);  // Initialize on GPIOD

  // 5. Configure QSPI DQ1 (Data Line 1)
  GPIO_InitStruct.Pin       = QSPI_FPGA_MISO_N;  
  GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
  HAL_GPIO_Init(QSPI_FPGA_MISO_G, &GPIO_InitStruct); 

  // 6. Configure QSPI DQ2 (Data Line 2)
  GPIO_InitStruct.Pin       = QSPI_FPGA_WP_N; 
  GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;  
  HAL_GPIO_Init(QSPI_FPGA_WP_G, &GPIO_InitStruct);  

  // 7. Configure QSPI DQ3 (Data Line 3)
  GPIO_InitStruct.Pin       = QSPI_FPGA_RESET_N; 
  GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
  HAL_GPIO_Init(QSPI_FPGA_RESET_G, &GPIO_InitStruct);  

  Serial.println("FPGA QSPI Init done!");
}

int MX_QSPI_Init() 
{
  // Initialize the QSPI interface
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler     = 20;        // Set prescaler for QSPI clock
  hqspi.Init.FifoThreshold      = 1;
  hqspi.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_NONE; // Is this right????
  hqspi.Init.FlashSize          = 23;       // Size of the flash memory (e.g., 128MB -> 23)
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
  hqspi.Init.ClockMode          = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID            = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;


  if (HAL_QSPI_Init(&hqspi) != HAL_OK) {
      Serial.println("QSPI Init failed!");
  }
}

int BSP_QSPI_Init()
{
  QSPI_MspInit();

  MX_QSPI_Init();
  return 0;

}

/*************** CLI functions **************/
void onQspiReadID(EmbeddedCli *cli, char *args, void *context)
{
  QSPI_ReadChipID();
}

void onQspiChipErase(EmbeddedCli *cli, char *args, void *context)
{
  Serial.println("Doing full chip erase on QSPI flash!");
  QSPI_FullChipErase();
}

void onQspiErase(EmbeddedCli *cli, char *args, void *context)
{
  uint32_t addr = 0;
  int retval = 0;

  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("Qspi Erase sector no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 1 ) {
    Serial.println("Qspi Erase sector needs 1 argument!");
    return;
  } 

  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 1), &addr ) ) {
    Serial.println("Failed to parse first argument for uint32_t");
    return;
  }
  QSPI_Erase(addr);

  sprintf(print_buffer, "Qspi Erase sector starting at 0x%lx, retval = 0x%x\r\n", addr, retval);
  Serial.print(print_buffer);
}



// Current state for QSPI operations
uint32_t flash_address = 0;  // Current write address in QSPI flash
int num_bytes_receiving = 0;

uint8_t debug_first_bytes[16];
int first_byte_count = 0;

uint8_t debug_last_bytes[128];
int block_counter = 0;

int dataSize_not128 = 0;
int block_counter_not128 = 0;

int non_128_counter = 0;

static bool process_block(void *blk_id, size_t idSize, byte *data, size_t dataSize) {

  /*** Debug code, can't print during xmodem transfer, need to store debug stuff ***/
    if ( dataSize != 128 ) {
      dataSize_not128 = dataSize;
      block_counter_not128 = block_counter;
      non_128_counter++;
    }

    block_counter++;
    if ( first_byte_count == 0 ) {
      memset(debug_first_bytes, 0, 16);
      for ( int i = 0; i < 16; i++ ) {
        debug_first_bytes[i] = data[i];
      }
      first_byte_count = 16;
    }
    // to get last bytes, when near the end of packet, start copying data 
    if ( num_bytes_receiving <= 1024 ) {
      memset(debug_last_bytes, 0, 128);
      for ( int i = 0; i < dataSize; i++ ) {
        debug_last_bytes[i] = data[i];
      }
    }
    /*
    if ( dataSize != 128 && non_128_counter > 2 ) {
      return false;
    }
    */
    /************** BUG IN THE XMODEM ARDUINO LIBRARY
    If 0x1A is near the end of the packet, it will be dropped by library 
    This shows up like 127 bytes or some less than 128 byte data buffer
    while still expecting quite a bit of data

    This is mostly fine, but this edge case corrupted my raw binary data transfer

    Workaround implemented here 
    data always has the full data including 0x1A, so just write that directly
    ***************/

    int my_dataSize = 0;
    if ( num_bytes_receiving >= 128 ) { // I'm expecting more than 128 bytes
      my_dataSize = 128;
    } else {
      my_dataSize = num_bytes_receiving;
    }
    // First write the bytes that the library passed
    QSPI_Write_PageSafe((uint8_t *)data, flash_address, my_dataSize);

    // Update state
    flash_address += (uint32_t) my_dataSize;
    num_bytes_receiving -= (int)my_dataSize;
    if ( num_bytes_receiving > 0 ) 
    {
      return true;
    }
    return false;
}

void start_xmodem_flash(int total_bytes)
{
  XModem xmodem_qspi;
  sprintf(print_buffer, "Xmodem transfer to QSPI flash expecting %d bytes!\r\n", total_bytes);
  Serial.print(print_buffer);
  flash_address = 0; // reset this global variable
  dataSize_not128 = 0;
  block_counter_not128 = 0;
  block_counter = 0; 
  non_128_counter = 0;
  num_bytes_receiving = total_bytes;
  Serial.println("Erasing flash!");
  QSPI_FullChipErase(); // erase whole flash
  Serial.println("Ready to start xmodem!");
  xmodem_qspi.begin(Serial, XModem::ProtocolType::XMODEM);
  xmodem_qspi.setRecieveBlockHandler(process_block);
  while ( xmodem_qspi.receive() ) {}

  Serial.println("Done with Qspi Xmodem flash!"); 
  sprintf(print_buffer,"Data size not 128 = %d, block = %d\r\n", dataSize_not128, block_counter_not128); 
  Serial.print(print_buffer);
  sprintf(print_buffer,"Flash address at end = 0x%08X\r\n", flash_address);
  Serial.print(print_buffer);
  Serial.println("First bytes:");
  for ( int i = 0; i < 16; i++ ) {
    sprintf(print_buffer, "%02X ", debug_first_bytes[i]);
    Serial.print(print_buffer);
  }
  Serial.println("");
  Serial.println("Last bytes:");
  for ( int i = 0; i < 128; i++ ) {
    sprintf(print_buffer, "%02X ", debug_last_bytes[i]);
    Serial.print(print_buffer);
    if ((i + 1) % 16 == 0) {
      Serial.println("");
    }    
  }
  Serial.println("");


}



void onQspiXmodemFlash(EmbeddedCli *cli, char *args, void *context)
{
  int total_bytes = 0;
  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("Qspi fast flash no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 1 ) {
    Serial.println("Qspi fast flash needs 1 argument!");
    return;
  } 

  total_bytes = atoi( embeddedCliGetToken(args, 1) );
  if ( total_bytes == 0 ) {
    Serial.println("Xmodem flash didn't receive number of bytes!");
    return;
  }
  start_xmodem_flash(total_bytes);

}


uint8_t write_buffer[256];
int parse_arguments(const char *args, uint32_t *address, uint8_t write_buffer[256]) {
  int value_count = 0;  // Number of 8-bit values parsed
  const char *p = args;  // Pointer to traverse the input string

  // Parse the first value as a 32-bit address
  while (isspace(*p)) p++;  // Skip leading whitespace
  if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
      p += 2;  // Skip the "0x" prefix

      char *end_ptr;
      unsigned long addr = strtoul(p, &end_ptr, 16);
      if (end_ptr > p) {
          *address = (uint32_t)addr;  // Store the parsed address
      } else {
          return -1;  // Failed to parse address
      }
      p = end_ptr;  // Move the pointer past the address
  } else {
      return -1;  // Address not in the correct format
  }
  // Parse the subsequent values as 8-bit values
  while (*p != '\0') {
      while (isspace(*p)) p++;  // Skip whitespace

      if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
          p += 2;  // Skip the "0x" prefix

          char *end_ptr;
          unsigned long value = strtoul(p, &end_ptr, 16);

          // Ensure the value fits in a uint8_t and was parsed correctly
          if (end_ptr > p && value <= 0xFF) {
              if (value_count < 64) {
                  write_buffer[value_count++] = (uint8_t)value;
              }
          }

          p = end_ptr;  // Move the pointer past the value
      } else {
          p++;  // If not a valid "0x" prefixed value, skip it
      }
  }
  return value_count;  // Return the number of 8-bit values parsed
}


void onQspiWriteByte(EmbeddedCli *cli, char *args, void *context)
{
  uint32_t addr = 0;
  int value_count = 0;
  int retval = 0;

  value_count = parse_arguments(args, &addr, write_buffer);

  sprintf(print_buffer, "QSPI Writing %d bytes!\r\n", value_count);
  Serial.print(print_buffer);

  QSPI_Write(write_buffer, addr, value_count);
}

void onQspiReadByte(EmbeddedCli *cli, char *args, void *context)
{
  uint32_t addr = 0;
  int byte_count = 0;
  uint8_t value = 0;
  int retval = 0;

  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("Qspi Read byte no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 1 && embeddedCliGetTokenCount(args) != 2  ) {
    Serial.println("Qspi Read byte needs 1 or 2 argument!");
    return;
  } 

  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 1), &addr ) ) {
    Serial.println("Failed to parse first argument for uint32_t");
    return;
  }
  if ( embeddedCliGetTokenCount(args) == 2 ) {
    // multiple dump
    byte_count = atoi( embeddedCliGetToken(args, 2) );
    for ( int i = 0; i < byte_count; i++ ) {
      if ( i % 16 == 0 ) {
        sprintf(print_buffer,"\r\n%08X:  ", addr + i);
        Serial.print(print_buffer);
      }
      QSPI_Read(&value, addr+i, 1);
      sprintf(print_buffer, "%02X ", value);
      Serial.print(print_buffer);      
    }
    Serial.println("");
    
    
  } else {
    Serial.println("QSPI Read byte start!");
    QSPI_Read(&value, addr, 1);

    sprintf(print_buffer, "Qspi Read Byte, 0x%lx = 0x%x, retval = 0x%x\r\n", addr, value, retval);
    Serial.print(print_buffer);
  }

}

void onQspiDisable(EmbeddedCli *cli, char *args, void *context)
{
  Serial.println("Disabling QSPI interface!");
  QSPI_Disable();
}

void onQspiEnable(EmbeddedCli *cli, char *args, void *context)
{
  Serial.println("Reinitializing QSPI interface!");
  BSP_QSPI_Init();
}

static Node qspi_readID_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "IDRead",
    "Read QSPI flash ID",
    true,
    nullptr,
    onQspiReadID
  }
};

static Node qspi_erase_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "erase",
    "Erase a sector",
    true,
    nullptr,
    onQspiErase
  }
};

static Node qspi_write_byte_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "byteWrite",
    "Write a byte",
    false,
    nullptr,
    onQspiWriteByte
  }
};

static Node qspi_read_byte_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "byteRead",
    "Read a byte at an address, or read X number of bytes starting at an address",
    true,
    nullptr,
    onQspiReadByte
  }
};

static Node qspi_chip_erase_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "chipErase",
    "Erase the entire SPI flash",
    true,
    nullptr,
    onQspiChipErase
  }
};

static Node qspi_xmodem_flash_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "xmodem_flash",
    "Use xmodem to flash a file to QSPI flash, requires file size in bytes as argument",
    true,
    nullptr,
    onQspiXmodemFlash
  }
};

static Node qspi_disable_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "disable",
    "Disable the QSPI interface",
    true,
    nullptr,
    onQspiDisable
  }
};

static Node qspi_enable_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "enable",
    "Re-initialize and enable the QSPI interface",
    true,
    nullptr,
    onQspiEnable
  }
};

/***************** Boiler plate CLI stuff ************/



void my_qspi_dir_operation(EmbeddedCli *cli, char *args, void *context); // forward declaration


static Node * my_qspi_files[] = { &qspi_erase_node, &qspi_write_byte_node, &qspi_read_byte_node,
  &qspi_readID_node, &qspi_xmodem_flash_node, &qspi_disable_node, &qspi_enable_node, &qspi_chip_erase_node };

static Node my_qspi_dir = {
    .name = "qspi",
    .type = MY_DIRECTORY,
    .cliBinding = {"qspi",
          "qspi mode",
          true,
          nullptr,
          my_qspi_dir_operation},
    .parent = 0,
    .children = my_qspi_files,
    .num_children = sizeof(my_qspi_files) / sizeof(my_qspi_files[0])
};

void my_qspi_dir_operation(EmbeddedCli *cli, char *args, void *context) {
  change_to_node(&my_qspi_dir);
}

// Initialize function to set the parent pointers if needed
void my_qspi_fs_init() {
  for (int i = 0; i < my_qspi_dir.num_children; i++) {
    my_qspi_files[i]->parent = &my_qspi_dir;
  }
  add_root_filesystem(&my_qspi_dir);
}




void init_my_qspi_cli()
{
  Serial.println("my_qspi Init start!");
  BSP_QSPI_Init();

  Serial.println("my_qspi add cli");

  my_qspi_fs_init();
}

