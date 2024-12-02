#include "my_qspi.h"



/*************** QSPI STM32 APIs and functions ********/

// QSPI handle (global variable)
QSPI_HandleTypeDef hqspi;



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



void QSPI_Write(uint8_t *pData, uint32_t address, uint32_t size) {

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
    return;
  }

  // Transmit the data
  if (HAL_QSPI_Transmit(&hqspi, pData, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    Serial.println("QSPI write failed on transmit!");
    return;
  }
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
  hqspi.Init.ClockPrescaler     = 10;        // Set prescaler for QSPI clock
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

}

/*************** CLI functions **************/
void onQspiReadID(EmbeddedCli *cli, char *args, void *context)
{
  QSPI_ReadChipID();
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


/****** Xmodem compatible flash ******/
// Command requires total number of bytes to flash
// then uses Xmodem


#define SOH 0x01  // Start of 128-byte block
#define STX 0x02  // Start of 1024-byte block
#define EOT 0x04  // End of Transmission
#define ACK 0x06  // Acknowledge
#define NAK 0x15  // Negative Acknowledge
#define C   0x43  // Request CRC mode


uint8_t block_buffer[1024];  // Buffer for the largest possible block


// Constants for QSPI flash memory
#define PAGE_SIZE 256        // Maximum bytes per QSPI page write
#define SECTOR_SIZE 4096     // QSPI sector size for erases

// Current state for QSPI operations
uint32_t flash_address = 0;  // Current write address in QSPI flash

void processBlock(const uint8_t *data, uint16_t valid_bytes) {
    // Erase the sector if the current address is at a sector boundary
    if (flash_address % SECTOR_SIZE == 0) {
        Serial.print("Erasing sector at address: 0x");
        Serial.println(flash_address, HEX);
        QSPI_Erase(flash_address);
    }

    // Split the data into chunks of 256 bytes for QSPI_Write
    uint32_t bytes_to_write = valid_bytes;
    const uint8_t *current_data = data;

    while (bytes_to_write > 0) {
        // Write up to PAGE_SIZE (256 bytes) at a time
        uint32_t writable_bytes = (bytes_to_write > PAGE_SIZE) ? PAGE_SIZE : bytes_to_write;

        Serial.print("Writing ");
        Serial.print(writable_bytes);
        Serial.print(" bytes to address: 0x");
        Serial.println(flash_address, HEX);

        QSPI_Write((uint8_t *)current_data, flash_address, writable_bytes);

        // Update state
        flash_address += writable_bytes;
        current_data += writable_bytes;
        bytes_to_write -= writable_bytes;
    }
}

// Compute CRC-16 for XMODEM
uint16_t computeCRC(const uint8_t *data, uint16_t length) {
  uint16_t crc = 0;
  for (uint16_t i = 0; i < length; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021; // Polynomial for XMODEM CRC
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}


// Function to read a byte with a timeout
int readByteWithTimeout(unsigned long timeout_ms) {
  unsigned long start_time = millis();
  while (Serial.available() == 0) {
    if (millis() - start_time >= timeout_ms) {
        return -1;  // Timeout occurred
    }
  }
  return Serial.read();
}


// XMODEM receiver function
void receiveXmodem(uint32_t total_bytes) {
    uint32_t bytes_received = 0;  // Total bytes received
    unsigned long start_time = millis();
    int header = -1;  // Store the first valid header

    // Send initial 'C' for CRC mode and wait for the first header
    while (true) {
        if (millis() - start_time > 30000) {
            Serial.println("Timeout waiting for sender.");
            return;
        }

        Serial.write(C); // Request CRC mode

        // Check if the sender has started sending
        if (Serial.available() > 0) {
            header = Serial.read();
            if (header == SOH || header == STX || header == EOT) {
                Serial.println("Sender responded, starting transfer...");
                break;  // Exit the readiness loop with a valid header
            }
        }

        delay(100);
    }

    // Main transfer loop
    while (bytes_received < total_bytes) {
        // Use the first valid header from the initial loop, or read a new one
        if (header == -1) {
            header = readByteWithTimeout(1000);
            if (header == -1) {
                Serial.println("Timeout waiting for header");
                continue;
            }
        }

        int block_size = 0;
        if (header == SOH) {
            block_size = 128;
        } else if (header == STX) {
            block_size = 1024;
        } else if (header == EOT) {
            // End of transmission
            Serial.write(ACK);
            Serial.println("File transfer complete.");
            break;
        } else {
            // Unexpected header, send NAK
            Serial.println("Unexpected header");
            Serial.write(NAK);
            header = -1;  // Reset header for the next loop
            continue;
        }

        // Read block number and its complement
        int block_num = readByteWithTimeout(1000);
        int block_num_complement = readByteWithTimeout(1000);
        if (block_num == -1 || block_num_complement == -1 || (block_num + block_num_complement) != 0xFF) {
            Serial.println("Invalid block number");
            Serial.write(NAK);
            header = -1;  // Reset header for the next loop
            continue;
        }

        // Read the data block
        for (int i = 0; i < block_size; i++) {
            int byte = readByteWithTimeout(1000);
            if (byte == -1) {
                Serial.println("Timeout waiting for block data");
                Serial.write(NAK);
                header = -1;  // Reset header for the next loop
                break;
            }
            block_buffer[i] = (uint8_t)byte;
        }

        // Read the CRC (2 bytes)
        int crc_high = readByteWithTimeout(1000);
        int crc_low = readByteWithTimeout(1000);
        if (crc_high == -1 || crc_low == -1) {
            Serial.println("Timeout waiting for CRC");
            Serial.write(NAK);
            header = -1;  // Reset header for the next loop
            continue;
        }
        uint16_t received_crc = (crc_high << 8) | crc_low;

        // Compute CRC for the received block
        uint16_t computed_crc = computeCRC(block_buffer, block_size);

        if (computed_crc == received_crc) {
            // Data is valid, process the block
            uint16_t valid_bytes = block_size;

            // Adjust for the last block
            if (bytes_received + valid_bytes > total_bytes) {
                valid_bytes = total_bytes - bytes_received;
            }

            processBlock(block_buffer, valid_bytes);
            bytes_received += valid_bytes;

            Serial.write(ACK);  // Acknowledge successful block
        } else {
            Serial.println("CRC mismatch");
            Serial.write(NAK);  // Request retransmission
        }

        // Reset header for the next loop
        header = -1;
    }
}


void onQspiXmodemFlash(EmbeddedCli *cli, char *args, void *context)
{
  int total_bytes = 0;
  int bytes_so_far = 0;
  int flash_addr = 0;
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
  sprintf(print_buffer, "Xmodem transfer to QSPI flash expecting %d bytes!r\n", total_bytes);
  flash_address = 0; // reset this global variable
  receiveXmodem(total_bytes);

  Serial.println("Done with Qspi Xmodem flash!");  

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
  uint8_t value = 0;
  int retval = 0;

  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("Qspi Read byte no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 1 ) {
    Serial.println("Qspi Read byte needs 1 argument!");
    return;
  } 

  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 1), &addr ) ) {
    Serial.println("Failed to parse first argument for uint32_t");
    return;
  }
  Serial.println("QSPI Read byte start!");
  QSPI_Read(&value, addr, 1);

  sprintf(print_buffer, "Qspi Read Byte, 0x%lx = 0x%x, retval = 0x%x\r\n", addr, value, retval);
  Serial.print(print_buffer);
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
    "Read a byte",
    true,
    nullptr,
    onQspiReadByte
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

/***************** Boiler plate CLI stuff ************/



void my_qspi_dir_operation(EmbeddedCli *cli, char *args, void *context); // forward declaration


static Node * my_qspi_files[] = { &qspi_erase_node, &qspi_write_byte_node, &qspi_read_byte_node,
  &qspi_readID_node };

static Node my_qspi_dir = {
    .name = "my_qspi",
    .type = MY_DIRECTORY,
    .cliBinding = {"my_qspi",
          "my_qspi mode",
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

