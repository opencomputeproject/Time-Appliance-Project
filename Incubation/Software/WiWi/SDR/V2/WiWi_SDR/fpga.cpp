#include "fpga.h"

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


void QSPI_WriteEnable(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef sCommand = {0};

  // Configure the Write Enable command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = WRITE_ENABLE_CMD;
  sCommand.AddressMode       = QSPI_ADDRESS_NONE;
  sCommand.DataMode          = QSPI_DATA_NONE;

  // Send the command
  if (HAL_QSPI_Command(hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      // Command error
      //Error_Handler();
  }
}


void QSPI_AutoPollingMemReady(QSPI_HandleTypeDef *hqspi)
{
  QSPI_CommandTypeDef sCommand = {0};
  QSPI_AutoPollingTypeDef sConfig = {0};

  // Configure automatic polling mode to wait for memory ready
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = READ_STATUS_REG_CMD;
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.DummyCycles       = 0;

  sConfig.Match           = 0;
  sConfig.Mask            = 0x01;  // Wait for WIP (Write In Progress) bit to be cleared
  sConfig.MatchMode       = QSPI_MATCH_MODE_AND;
  sConfig.StatusBytesSize = 1;
  sConfig.Interval        = 0x10;
  sConfig.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_QSPI_AutoPolling(hqspi, &sCommand, &sConfig, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
      // Polling error
      //Error_Handler();
  }
}


bool FpgaQspiReadByte(uint32_t addr, uint8_t * val)
{
  QSPI_CommandTypeDef sCommand;
  uint8_t data;

  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = READ_CMD; // Standard JEDEC read command (0x03)
  sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = addr;  // Address of the byte to read
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.NbData            = 1;        // Reading 1 byte
  sCommand.DummyCycles       = 0;

  if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) == HAL_OK) {
    HAL_QSPI_Receive(&hqspi, val, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
    return 1;
  }
  return 0;
}

bool FpgaQspiWriteByte(uint32_t addr, uint8_t val)
{
  QSPI_CommandTypeDef sCommand;
  // Enable write operation
  QSPI_WriteEnable(&hqspi);

  // Configure the command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = PAGE_PROG_CMD; // Standard JEDEC write command (0x02)
  sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = addr;  // Address to write to
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.NbData            = 1;        // Writing 1 byte

  if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) == HAL_OK) {
    HAL_QSPI_Transmit(&hqspi, &val, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  } else {
    return 0;
  }
  // Wait until memory is ready
  QSPI_AutoPollingMemReady(&hqspi);
  return 1;
}

// 4KB sector, so address of 0x0 / 0x1000 / 0x2000 / 0x3000 etc etc
bool FpgaQspiEraseSector(uint32_t addr)
{
  QSPI_CommandTypeDef sCommand;
  // Enable write operation
  QSPI_WriteEnable(&hqspi);

  // Configure the command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = SECTOR_ERASE_CMD; // Standard sector erase command (0x20)
  sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = addr;  // Address of the sector to erase
  sCommand.DataMode          = QSPI_DATA_NONE;
  sCommand.DummyCycles       = 0;

  if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) == HAL_OK) {
      // Wait until memory is ready
      QSPI_AutoPollingMemReady(&hqspi);
  }
  return 1;
}

bool FpgaQspiWriteMultiple(uint32_t addr, uint8_t * data, int count)
{
  QSPI_CommandTypeDef sCommand;

  // Enable write operation
  QSPI_WriteEnable(&hqspi);

  // Configure the command
  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = PAGE_PROG_CMD; // JEDEC page program command (0x02)
  sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = addr;  // Address to write to
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.NbData            = count;  // Writing one page

  if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) == HAL_OK) {
    HAL_QSPI_Transmit(&hqspi, data, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  }

  // Wait until memory is ready
  QSPI_AutoPollingMemReady(&hqspi);
  return 1;
}

bool FpgaQspiReadMultiple(uint32_t addr, uint8_t * data, int count)
{
  QSPI_CommandTypeDef sCommand;

  sCommand.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
  sCommand.Instruction       = READ_CMD; // Standard JEDEC read command (0x03)
  sCommand.AddressMode       = QSPI_ADDRESS_1_LINE;
  sCommand.AddressSize       = QSPI_ADDRESS_24_BITS;
  sCommand.Address           = addr;  // Address to start reading from
  sCommand.DataMode          = QSPI_DATA_1_LINE;
  sCommand.NbData            = count;  // Reading one page
  sCommand.DummyCycles       = 0;

  if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) == HAL_OK) {
      HAL_QSPI_Receive(&hqspi, data, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
  }
  return 1;
}

void init_fpga_qspi()
{
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
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
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

  // Initialize the QSPI interface
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler     = 10;        // Set prescaler for QSPI clock
  hqspi.Init.FifoThreshold      = 4;
  hqspi.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi.Init.FlashSize          = 23;       // Size of the flash memory (e.g., 128MB -> 23)
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
  hqspi.Init.ClockMode          = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID            = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;

  if (HAL_QSPI_Init(&hqspi) != HAL_OK) {
      Serial.println("QSPI Init failed!");
  }

  Serial.println("FPGA QSPI Init done!");
}







void init_fpga_cli()
{

  init_fpga_qspi();
  
  /*
  // expose FPGA QSPI CLI
  
  embeddedCliAddBinding(cli, {
          "fpga-qspi-read-single",
          "Read a single byte from FPGA QSPI flash, pass base address (16-bit) / offset (16-bit) / value (8-bit) ex: dpll-write-reg 0xc03c 0x101 0x0",
          true,
          nullptr,
          onDpllWrite
  });
  */


}