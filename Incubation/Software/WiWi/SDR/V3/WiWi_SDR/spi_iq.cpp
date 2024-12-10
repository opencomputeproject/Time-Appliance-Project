

#include "spi_iq.h"

/******************** STM32 code *******************/

/******* SPI1 = SubG I *******/
SPI_HandleTypeDef spi1_handler;
DMA_HandleTypeDef spi1_rx_dma_handler;
DMA_HandleTypeDef spi1_tx_dma_handler;

/****** SPI2 = SubG Q ********/
SPI_HandleTypeDef spi2_handler;
DMA_HandleTypeDef spi2_rx_dma_handler;
DMA_HandleTypeDef spi2_tx_dma_handler;

/****** SPI3 = WiFi I *******/
SPI_HandleTypeDef spi3_handler;
DMA_HandleTypeDef spi3_rx_dma_handler;
DMA_HandleTypeDef spi3_tx_dma_handler;

/****** SPI4 = WiFi Q ********/
SPI_HandleTypeDef spi4_handler;
DMA_HandleTypeDef spi4_rx_dma_handler;
DMA_HandleTypeDef spi4_tx_dma_handler;
extern "C" {
  void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) // called by HAL_SPI_Init
  {
    if (hspi == &spi1_handler ){ 
      // Init the STM32 SPI interface
      GPIO_InitTypeDef GPIO_InitStruct = {0};

      //SCK Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI1_SCK].GPIO_Pin;  
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;  // Alternate function for SPI1
      HAL_GPIO_Init(WWVB_Pins[SPI1_SCK].GPIO_Group, &GPIO_InitStruct);

      // MISO Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI1_MISO].GPIO_Pin;  
      HAL_GPIO_Init(WWVB_Pins[SPI1_MISO].GPIO_Group, &GPIO_InitStruct);

      // MOSI Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI1_MOSI].GPIO_Pin; 
      HAL_GPIO_Init(WWVB_Pins[SPI1_MOSI].GPIO_Group, &GPIO_InitStruct);

      // RX DMA , NOT USING AS NORMAL SPI DMA!!!!
      spi1_rx_dma_handler.Instance = SUBG_I_RX_DMA_STREAM; // Example stream, adjust as needed
      spi1_rx_dma_handler.Init.Request = DMA_REQUEST_SPI1_RX; // Make sure to use the correct request number
      spi1_rx_dma_handler.Init.Direction = DMA_PERIPH_TO_MEMORY;
      spi1_rx_dma_handler.Init.PeriphInc = DMA_PINC_DISABLE;
      spi1_rx_dma_handler.Init.MemInc = DMA_MINC_ENABLE;
      spi1_rx_dma_handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
      spi1_rx_dma_handler.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD; 
      spi1_rx_dma_handler.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
      spi1_rx_dma_handler.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
      spi1_rx_dma_handler.Init.Mode = DMA_CIRCULAR; 
      spi1_rx_dma_handler.Init.Priority = DMA_PRIORITY_HIGH;

      HAL_DMA_Init(&spi1_rx_dma_handler);

      spi1_handler.hdmarx = &spi1_rx_dma_handler; 
      spi1_rx_dma_handler.Parent = &spi1_handler; // Link DMA to SPI1 RX, NEED BOTH 

      // TX DMA not done here yet, just focus on RX only keep code simpler
      
    } 
    else if (hspi == &spi2_handler)
    {
      GPIO_InitTypeDef GPIO_InitStruct = {0};

      // SCK Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI2_SCK].GPIO_Pin;  
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;  // Alternate function for SPI
      HAL_GPIO_Init(WWVB_Pins[SPI2_SCK].GPIO_Group, &GPIO_InitStruct);

      // MISO Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI2_MISO].GPIO_Pin;  
      HAL_GPIO_Init(WWVB_Pins[SPI2_MISO].GPIO_Group, &GPIO_InitStruct);

      // MOSI Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI2_MOSI].GPIO_Pin; 
      HAL_GPIO_Init(WWVB_Pins[SPI2_MOSI].GPIO_Group, &GPIO_InitStruct);

      // RX DMA
      spi2_rx_dma_handler.Instance = SUBG_Q_RX_DMA_STREAM; // Example stream, adjust as needed
      spi2_rx_dma_handler.Init.Request = DMA_REQUEST_SPI2_RX; // Make sure to use the correct request number
      spi2_rx_dma_handler.Init.Direction = DMA_PERIPH_TO_MEMORY;
      spi2_rx_dma_handler.Init.PeriphInc = DMA_PINC_DISABLE;
      spi2_rx_dma_handler.Init.MemInc = DMA_MINC_ENABLE;
      spi2_rx_dma_handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
      spi2_rx_dma_handler.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
      spi2_rx_dma_handler.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
      spi2_rx_dma_handler.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
      spi2_rx_dma_handler.Init.Mode = DMA_CIRCULAR; // Or DMA_CIRCULAR for continuous reception
      spi2_rx_dma_handler.Init.Priority = DMA_PRIORITY_HIGH;

      HAL_DMA_Init(&spi2_rx_dma_handler);

      spi2_handler.hdmarx = &spi2_rx_dma_handler; 
      spi2_rx_dma_handler.Parent = &spi2_handler; // Link DMA to SPI RX , NEED BOTH, SAME AS __HAL_DMA_LINK
    }
    else if ( hspi == &spi3_handler ) {
      GPIO_InitTypeDef GPIO_InitStruct = {0};

      // SCK Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI3_SCK].GPIO_Pin;  
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;  // Alternate function for SPI for this pin
      HAL_GPIO_Init(WWVB_Pins[SPI3_SCK].GPIO_Group, &GPIO_InitStruct);

      // MISO Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI3_MISO].GPIO_Pin;  
      GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;  // Alternate function for SPI for this pin
      HAL_GPIO_Init(WWVB_Pins[SPI3_MISO].GPIO_Group, &GPIO_InitStruct);

      // MOSI Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI3_MOSI].GPIO_Pin; 
      GPIO_InitStruct.Alternate = GPIO_AF5_SPI3;  // Alternate function for SPI
      HAL_GPIO_Init(WWVB_Pins[SPI3_MOSI].GPIO_Group, &GPIO_InitStruct);


      // RX DMA
      spi3_rx_dma_handler.Instance = WIFI_I_RX_DMA_STREAM; // Example stream, adjust as needed
      spi3_rx_dma_handler.Init.Request = DMA_REQUEST_SPI3_RX; // Make sure to use the correct request number
      spi3_rx_dma_handler.Init.Direction = DMA_PERIPH_TO_MEMORY;
      spi3_rx_dma_handler.Init.PeriphInc = DMA_PINC_DISABLE;
      spi3_rx_dma_handler.Init.MemInc = DMA_MINC_ENABLE;
      spi3_rx_dma_handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
      spi3_rx_dma_handler.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
      spi3_rx_dma_handler.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
      spi3_rx_dma_handler.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
      spi3_rx_dma_handler.Init.Mode = DMA_CIRCULAR; // Or DMA_CIRCULAR for continuous reception
      spi3_rx_dma_handler.Init.Priority = DMA_PRIORITY_HIGH;

      HAL_DMA_Init(&spi3_rx_dma_handler);

      spi3_handler.hdmarx = &spi3_rx_dma_handler; 
      spi3_rx_dma_handler.Parent = &spi3_handler; // Link DMA to SPI RX , NEED BOTH, SAME AS __HAL_DMA_LINK

    }
    else if ( hspi == &spi4_handler ) {
      GPIO_InitTypeDef GPIO_InitStruct = {0};

      // SCK Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI4_SCK].GPIO_Pin;  
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;  // Alternate function for SPI
      HAL_GPIO_Init(WWVB_Pins[SPI4_SCK].GPIO_Group, &GPIO_InitStruct);

      // MISO Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI4_MISO].GPIO_Pin;  
      HAL_GPIO_Init(WWVB_Pins[SPI4_MISO].GPIO_Group, &GPIO_InitStruct);

      // MOSI Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SPI4_MOSI].GPIO_Pin; 
      HAL_GPIO_Init(WWVB_Pins[SPI4_MOSI].GPIO_Group, &GPIO_InitStruct);


      // RX DMA
      spi4_rx_dma_handler.Instance = WIFI_Q_RX_DMA_STREAM; // Example stream, adjust as needed
      spi4_rx_dma_handler.Init.Request = DMA_REQUEST_SPI4_RX; // Make sure to use the correct request number
      spi4_rx_dma_handler.Init.Direction = DMA_PERIPH_TO_MEMORY;
      spi4_rx_dma_handler.Init.PeriphInc = DMA_PINC_DISABLE;
      spi4_rx_dma_handler.Init.MemInc = DMA_MINC_ENABLE;
      spi4_rx_dma_handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
      spi4_rx_dma_handler.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
      spi4_rx_dma_handler.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
      spi4_rx_dma_handler.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
      spi4_rx_dma_handler.Init.Mode = DMA_CIRCULAR; // Or DMA_CIRCULAR for continuous reception
      spi4_rx_dma_handler.Init.Priority = DMA_PRIORITY_HIGH;

      HAL_DMA_Init(&spi4_rx_dma_handler);

      spi4_handler.hdmarx = &spi4_rx_dma_handler; 
      spi4_rx_dma_handler.Parent = &spi4_handler; // Link DMA to SPI RX , NEED BOTH, SAME AS __HAL_DMA_LINK
    }
  }
}


int init_subg_spi()
{

  // Init the STM32 SPI1 interface, SubG I Data
  spi1_handler.Instance = SPI1;
  spi1_handler.Init.Mode = SPI_MODE_SLAVE;  // SPI mode (Master/Slave)
  spi1_handler.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;  // different speed limits
  spi1_handler.Init.DataSize = SPI_DATASIZE_16BIT;  // data frame format, int16t from FPGA on V2 board
  spi1_handler.Init.CLKPolarity = SPI_POLARITY_LOW;  // Clock polarity CPOL
  spi1_handler.Init.CLKPhase = SPI_PHASE_2EDGE;  // Clock phase CPHA
  //spi1_handler.Init.CLKPhase = SPI_PHASE_1EDGE;  // Clock phase CPHA
  spi1_handler.Init.NSS = SPI_NSS_SOFT;  // NSS signal is managed by software
  spi1_handler.Init.NSSPolarity = SPI_NSS_POLARITY_HIGH;
  spi1_handler.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  // Baud rate prescaler
  spi1_handler.Init.FirstBit = SPI_FIRSTBIT_LSB;  //
  spi1_handler.Init.TIMode = SPI_TIMODE_DISABLE;  // Disable TI mode
  spi1_handler.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Disable CRC calculation
  spi1_handler.Init.CRCPolynomial = 7;  // Polynomial for CRC calculation
  //spi1_handler.Init.IOSwap = SPI_IO_SWAP_ENABLE; // messed up on board, swap MISO and MOSI with respect to STM32 -> V1 board, V2 doesn't need
  spi1_handler.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  spi1_handler.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
  spi1_handler.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  spi1_handler.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;

  if ( HAL_SPI_Init(&spi1_handler) != HAL_OK ) { // Calls MSP Init
    Serial.println("FAILED TO INIT SPI1 SUBG I Data SPI");
    return -1;
  }
  spi1_handler.Instance->CFG2 |= SPI_CFG2_AFCNTR; // hack, keep it as SPI mode always

  // Init the STM32 SPI1 interface, SubG I Data
  spi2_handler.Instance = SPI2;
  spi2_handler.Init.Mode = SPI_MODE_SLAVE;  // SPI mode (Master/Slave)
  spi2_handler.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;  // different speed limits
  spi2_handler.Init.DataSize = SPI_DATASIZE_16BIT;  // data frame format, int16t from FPGA on V2 board
  spi2_handler.Init.CLKPolarity = SPI_POLARITY_LOW;  // Clock polarity CPOL
  spi2_handler.Init.CLKPhase = SPI_PHASE_2EDGE;  // Clock phase CPHA
  spi2_handler.Init.NSS = SPI_NSS_SOFT;  // NSS signal is managed by software
  spi2_handler.Init.NSSPolarity = SPI_NSS_POLARITY_HIGH;
  spi2_handler.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  // Baud rate prescaler
  spi2_handler.Init.FirstBit = SPI_FIRSTBIT_LSB;  //
  spi2_handler.Init.TIMode = SPI_TIMODE_DISABLE;  // Disable TI mode
  spi2_handler.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Disable CRC calculation
  spi2_handler.Init.CRCPolynomial = 7;  // Polynomial for CRC calculation
  //spi2_handler.Init.IOSwap = SPI_IO_SWAP_ENABLE; // messed up on board, swap MISO and MOSI with respect to STM32 -> V1 board, V2 doesn't need
  spi2_handler.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  spi2_handler.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
  spi2_handler.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  spi2_handler.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;

  if ( HAL_SPI_Init(&spi2_handler) != HAL_OK ) { // Calls MSP Init
    Serial.println("FAILED TO INIT SPI2 SUB Q Data SPI");
    return -1;
  }
  spi2_handler.Instance->CFG2 |= SPI_CFG2_AFCNTR; // hack, keep it as SPI mode always

  return 0;
}

int init_wifi_spi()
{

  // Init the STM32 SPI3 interface, WiFi I Data
  spi3_handler.Instance = SPI3;
  spi3_handler.Init.Mode = SPI_MODE_SLAVE;  // SPI mode (Master/Slave)
  spi3_handler.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;  // different speed limits
  spi3_handler.Init.DataSize = SPI_DATASIZE_16BIT;  // data frame format, int16t from FPGA on V2 board
  spi3_handler.Init.CLKPolarity = SPI_POLARITY_LOW;  // Clock polarity CPOL
  spi3_handler.Init.CLKPhase = SPI_PHASE_2EDGE;  // Clock phase CPHA
  spi3_handler.Init.NSS = SPI_NSS_SOFT;  // NSS signal is managed by software
  spi3_handler.Init.NSSPolarity = SPI_NSS_POLARITY_HIGH;
  spi3_handler.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  // Baud rate prescaler
  spi3_handler.Init.FirstBit = SPI_FIRSTBIT_LSB;  //
  spi3_handler.Init.TIMode = SPI_TIMODE_DISABLE;  // Disable TI mode
  spi3_handler.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Disable CRC calculation
  spi3_handler.Init.CRCPolynomial = 7;  // Polynomial for CRC calculation
  //spi3_handler.Init.IOSwap = SPI_IO_SWAP_ENABLE; // messed up on board, swap MISO and MOSI with respect to STM32 -> V1 board, V2 doesn't need
  spi3_handler.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  spi3_handler.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
  spi3_handler.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  spi3_handler.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;

  if ( HAL_SPI_Init(&spi3_handler) != HAL_OK ) { // Calls MSP Init
    Serial.println("FAILED TO INIT SPI3 WIFI I Data SPI");
    return -1;
  }
  spi3_handler.Instance->CFG2 |= SPI_CFG2_AFCNTR; // hack, keep it as SPI mode always

  // Init the STM32 SPI4 interface, WiFi Q Data
  spi4_handler.Instance = SPI4;
  spi4_handler.Init.Mode = SPI_MODE_SLAVE;  // SPI mode (Master/Slave)
  spi4_handler.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;  // different speed limits
  spi4_handler.Init.DataSize = SPI_DATASIZE_16BIT;  // data frame format, int16t from FPGA on V2 board
  spi4_handler.Init.CLKPolarity = SPI_POLARITY_LOW;  // Clock polarity CPOL
  spi4_handler.Init.CLKPhase = SPI_PHASE_2EDGE;  // Clock phase CPHA
  spi4_handler.Init.NSS = SPI_NSS_SOFT;  // NSS signal is managed by software
  spi4_handler.Init.NSSPolarity = SPI_NSS_POLARITY_HIGH;
  spi4_handler.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  // Baud rate prescaler
  spi4_handler.Init.FirstBit = SPI_FIRSTBIT_LSB;  //
  spi4_handler.Init.TIMode = SPI_TIMODE_DISABLE;  // Disable TI mode
  spi4_handler.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Disable CRC calculation
  spi4_handler.Init.CRCPolynomial = 7;  // Polynomial for CRC calculation
  //spi4_handler.Init.IOSwap = SPI_IO_SWAP_ENABLE; // messed up on board, swap MISO and MOSI with respect to STM32 -> V1 board, V2 doesn't need
  spi4_handler.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  spi4_handler.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
  spi4_handler.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  spi4_handler.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;

  if ( HAL_SPI_Init(&spi4_handler) != HAL_OK ) { // Calls MSP Init
    Serial.println("FAILED TO INIT SPI4 WiFI Q Data SPI");
    return -1;
  }
  spi4_handler.Instance->CFG2 |= SPI_CFG2_AFCNTR; // hack, keep it as SPI mode always
  return 0;
}

void start_subg_dma()
{
  // DMA handlers are setup in MSP Init
  // actually start the DMA here
  HAL_StatusTypeDef stat_val = HAL_OK;
  __HAL_SPI_DISABLE(&spi1_handler); // make sure disable it again
  __HAL_SPI_DISABLE(&spi2_handler); // make sure disable it again
  stat_val = HAL_SPI_Receive_DMA_NoStart_NoInterrupt(&spi1_handler, (uint8_t*) &sram1_data->subg_I_data[0], BUFFER_SIZE);
  if ( stat_val != HAL_OK ) {
    Serial.print("Failed to start SPI1 RX DMA! 0x");
    Serial.print(stat_val, HEX);
    Serial.print(" error code 0x");
    Serial.println(spi1_handler.ErrorCode, HEX);
  }
  stat_val = HAL_SPI_Receive_DMA_NoStart_NoInterrupt(&spi2_handler, (uint8_t*) &sram1_data->subg_Q_data[0], BUFFER_SIZE);
  if ( stat_val != HAL_OK ) {
    Serial.print("Failed to start SPI2 RX DMA! 0x");
    Serial.print(stat_val, HEX);
    Serial.print(" error code 0x");
    Serial.println(spi2_handler.ErrorCode, HEX);
  }

  __HAL_SPI_ENABLE(&spi1_handler); 
  __HAL_SPI_ENABLE(&spi2_handler);   

}


void start_wifi_dma()
{
  // DMA handlers are setup in MSP Init
  // actually start the DMA here
  HAL_StatusTypeDef stat_val = HAL_OK;
  __HAL_SPI_DISABLE(&spi3_handler); // make sure disable it again
  __HAL_SPI_DISABLE(&spi4_handler); // make sure disable it again
  stat_val = HAL_SPI_Receive_DMA_NoStart_NoInterrupt(&spi3_handler, (uint8_t*) &sram2_data->wifi_I_data[0], BUFFER_SIZE);
  if ( stat_val != HAL_OK ) {
    Serial.print("Failed to start SPI3 RX DMA! 0x");
    Serial.print(stat_val, HEX);
    Serial.print(" error code 0x");
    Serial.println(spi1_handler.ErrorCode, HEX);
  }
  stat_val = HAL_SPI_Receive_DMA_NoStart_NoInterrupt(&spi4_handler, (uint8_t*) &sram2_data->wifi_Q_data[0], BUFFER_SIZE);
  if ( stat_val != HAL_OK ) {
    Serial.print("Failed to start SPI4 RX DMA! 0x");
    Serial.print(stat_val, HEX);
    Serial.print(" error code 0x");
    Serial.println(spi2_handler.ErrorCode, HEX);
  }

  __HAL_SPI_ENABLE(&spi3_handler); 
  __HAL_SPI_ENABLE(&spi4_handler);   
}




/************* Top level code **************/
void zero_iq_buffers()
{
    // setup the IQ buffers
  for ( int i = 0; i < BUFFER_SIZE; i++ ) {
    sram1_data->subg_I_data[i] = 0;
    sram1_data->subg_Q_data[i] = 0;
    sram2_data->wifi_I_data[i] = 0;
    sram2_data->wifi_Q_data[i] = 0;
  }
}




void init_spi_iq_interfaces()
{

  zero_iq_buffers();

  // setup SPI and DMA interfaces
  init_subg_spi();
  init_wifi_spi();
}

void init_spi_iq_dma()
{
  
  start_subg_dma();
  start_wifi_dma();
}

void init_spi_iq_data()
{
  disable_subg_shift();
  disable_wifi_shift();
  init_spi_iq_interfaces();
  init_spi_iq_dma();
  enable_subg_shift();
  enable_wifi_shift();
}




/**************** CLI functions ********************/


void onSpiIQPrintStats(EmbeddedCli *cli, char *args, void *context)
{
  Serial.println("SPI IQ interface stats!");
  uint32_t spi1_count, spi2_count, spi3_count, spi4_count;
  spi1_count = __HAL_DMA_GET_COUNTER(&spi1_rx_dma_handler);
  spi2_count = __HAL_DMA_GET_COUNTER(&spi2_rx_dma_handler);
  spi3_count = __HAL_DMA_GET_COUNTER(&spi3_rx_dma_handler);
  spi4_count = __HAL_DMA_GET_COUNTER(&spi4_rx_dma_handler);

  sprintf(print_buffer, "SPI1 count: %d\r\n", spi1_count); Serial.print(print_buffer);
  sprintf(print_buffer, "SPI2 count: %d\r\n", spi2_count); Serial.print(print_buffer);
  sprintf(print_buffer, "SPI3 count: %d\r\n", spi3_count); Serial.print(print_buffer);
  sprintf(print_buffer, "SPI4 count: %d\r\n", spi4_count); Serial.print(print_buffer);

}

void onSpiIQPrintLatestIQSamplesPerInterface(EmbeddedCli *cli, char *args, void *context)
{
  // two arguments
  // 1. Which SPI interface, 1 - 4
  // 2. How many samples to print 

  if (embeddedCliGetTokenCount(args) != 2) {
    Serial.println("SPI IQ Print latest IQ samples needs 2 arguments!");
    return;
  } 
  int spi_interface = 0;
  int samples_to_print = 0;
  uint16_t value = 0;
  spi_interface = atoi( embeddedCliGetToken(args, 1) );
  samples_to_print = atoi( embeddedCliGetToken(args, 2) );

  if ( spi_interface < 1 ) {
    spi_interface = 1;
  } else if ( spi_interface > 4 ) {
    spi_interface = 4;
  }
  if ( samples_to_print < 0 ) {
    samples_to_print = 1;
  } else if ( samples_to_print > BUFFER_SIZE) {
    samples_to_print = BUFFER_SIZE;
  }

  for ( int i = 0; i < samples_to_print; i++ ) {
    if ( spi_interface == 1 ) value = (uint16_t) sram1_data->subg_I_data[i];
    else if (spi_interface == 2 ) value = (uint16_t) sram1_data->subg_Q_data[i];
    else if ( spi_interface == 3 ) value = (uint16_t) sram2_data->wifi_I_data[i];
    else if ( spi_interface == 4 ) value = (uint16_t) sram2_data->wifi_Q_data[i];

    sprintf(print_buffer, "%06d : 0x%04X\r\n", i, value);
    Serial.print(print_buffer);
  }

}

void onSpiIQPrintLatestIQSamples(EmbeddedCli *cli, char *args, void *context)
{
  // two arguments
  // 1. Which SPI interface, 1 - 4
  // 2. How many samples to print 

  if (embeddedCliGetTokenCount(args) != 2) {
    Serial.println("SPI IQ Print latest IQ samples needs 2 arguments!");
    return;
  } 
  int channel = 0;
  int samples_to_print = 0;
  uint16_t value_i, value_q;
  channel = atoi( embeddedCliGetToken(args, 1) );
  samples_to_print = atoi( embeddedCliGetToken(args, 2) );
  value_i = 0;
  value_q = 0;

  if ( channel < 0 ) {
    channel = 0;
    Serial.println("SubG samples:");
  } else if ( channel > 1 ) {
    channel = 1;
    Serial.println("WiFi samples:");
  }
  if ( samples_to_print < 0 ) {
    samples_to_print = 1;
  } else if ( samples_to_print > BUFFER_SIZE) {
    samples_to_print = BUFFER_SIZE;
  }
  

  for ( int i = 0 ; i < samples_to_print; i++ ) {
    if ( channel == 0 ) {
      value_i = sram1_data->subg_I_data[i];
      value_q = sram1_data->subg_Q_data[i];
    } else {
      value_i = sram2_data->wifi_I_data[i];
      value_q = sram2_data->wifi_Q_data[i];
    }
    sprintf(print_buffer, "%06d : I=0x%04X Q=0x%04X\r\n", i, value_i, value_q);
    Serial.print(print_buffer);
  }

}



void onSpiIQPauseSubG(EmbeddedCli *cli, char *args, void *context)
{
  Serial.println("Pausing SubG stream (SPI1 and SPI2)");
  disable_subg_shift();
}
void onSpiIQEnableSubG(EmbeddedCli *cli, char *args, void *context)
{
  Serial.println("Enabling SubG stream (SPI1 and SPI2)");
  enable_subg_shift();
}
void onSpiIQPauseWiFi(EmbeddedCli *cli, char *args, void *context)
{
  Serial.println("Pausing WiFi stream (SPI3 and SPI4)");
  disable_wifi_shift();
}
void onSpiIQEnableWiFi(EmbeddedCli *cli, char *args, void *context)
{
  Serial.println("Enabling WiFi stream (SPI3 and SPI4)");
  enable_wifi_shift();
}

/*********** CLI boilerplate and init ***********/

static Node spi_iq_pause_subg_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "subg_pause",
    "Pause SubG streaming",
    true,
    nullptr,
    onSpiIQPauseSubG
  }
};
static Node spi_iq_enable_subg_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "subg_enable",
    "Enable SubG streaming",
    true,
    nullptr,
    onSpiIQEnableSubG
  }
};
static Node spi_iq_pause_wifi_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "wifi_pause",
    "Pause WiFi streaming",
    true,
    nullptr,
    onSpiIQPauseWiFi
  }
};
static Node spi_iq_enable_wifi_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "wifi_enable",
    "Enable WiFi streaming",
    true,
    nullptr,
    onSpiIQEnableWiFi
  }
};






static Node spi_iq_print_stats_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "print_stats",
    "Print current DMA transfer counters, for debug",
    true,
    nullptr,
    onSpiIQPrintStats
  }
};

static Node spi_iq_print_samples_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "channel_print_samples",
    "Print hex samples from a channel, pass channel (0 for SubGHz, 1 for WiFi) and number of samples up to BUFFER_SIZE",
    true,
    nullptr,
    onSpiIQPrintLatestIQSamples
  }
};

static Node spi_iq_print_samples_interface_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "interface_print_samples",
    "Print hex samples from SPI interface, pass spi interface (1-4) and number of samples up to BUFFER_SIZE",
    true,
    nullptr,
    onSpiIQPrintLatestIQSamplesPerInterface
  }
};

void spi_iq_dir_operation(EmbeddedCli *cli, char *args, void *context);

static Node * spi_iq_files[] = { &spi_iq_print_stats_node, &spi_iq_print_samples_node,
  &spi_iq_print_samples_interface_node,
  &spi_iq_pause_subg_node, &spi_iq_enable_subg_node,
  &spi_iq_pause_wifi_node, &spi_iq_enable_wifi_node };

static Node spi_iq_dir = {
    .name = "spi_iq",
    .type = MY_DIRECTORY,
    .cliBinding = {"spi_iq",
          "spi_iq mode",
          true,
          nullptr,
          spi_iq_dir_operation},
    .parent = 0,
    .children = spi_iq_files,
    .num_children = sizeof(spi_iq_files) / sizeof(spi_iq_files[0])
};

void spi_iq_dir_operation(EmbeddedCli *cli, char *args, void *context) {
  change_to_node(&spi_iq_dir);
}

// Initialize function to set the parent pointers if needed
void spi_iq_fs_init() {
  for (int i = 0; i < spi_iq_dir.num_children; i++) {
    spi_iq_files[i]->parent = &spi_iq_dir;
  }
  add_root_filesystem(&spi_iq_dir);
}



void init_spi_iq()
{
  Serial.println("Init SPI IQ datastreams");

  init_spi_iq_data();

    // expose spi_iq CLI
  spi_iq_fs_init();
  Serial.println("SPI IQ initialized!");
}