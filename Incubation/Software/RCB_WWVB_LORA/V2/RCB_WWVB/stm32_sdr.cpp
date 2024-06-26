

#include "stm32_sdr.h"




void print_spi_registers(char * name, SPI_HandleTypeDef * hspi)
{
  char print_buf[256];
  
  sprintf(print_buf, "%s regs: CR1=0x%x CR2=0x%x CFG1=0x%x CFG2=0x%x IER=0x%x "
    "SR=0x%x RXDR=0x%x", name, hspi->Instance->CR1,
    hspi->Instance->CR2,
    hspi->Instance->CFG1,
    hspi->Instance->CFG2,
    hspi->Instance->IER,
    hspi->Instance->SR,
    hspi->Instance->RXDR );
  Serial.println(print_buf);
}

void print_dma_registers(char * name, DMA_Stream_TypeDef * hdma) 
{
  char print_buf[256];  
  sprintf(print_buf, "%s regs: LISR=0x%x HISR=0x%x CR=0x%x NDTR=0x%x PAR=0x%x M0AR=0x%x M1AR=0x%x FCR=0x%x" , 
    name, DMA2->LISR, DMA2->HISR, 
     hdma->CR,
     hdma->NDTR,
     hdma->PAR,
     hdma->M0AR,
     hdma->M1AR,
     hdma->FCR );
  Serial.println(print_buf);
}



void stm32_sdr_spi_init()
{

  HAL_StatusTypeDef stat_val = HAL_OK; 
  Serial.println("Enabling RX DMA!");
  __HAL_SPI_DISABLE(&SX1257_SDR._spi_I_Data); // make sure disable it again
  __HAL_SPI_DISABLE(&SX1257_SDR._spi_Q_Data); // make sure disable it again
  //SPI1 = I data
  Serial.println("Enabling SPI1 I data Receive DMA");
  // Dump data into DFSDM register
  stat_val = HAL_SPI_Receive_DMA_NoStart_NoInterrupt(&SX1257_SDR._spi_I_Data, (uint8_t*) &sram1_data->I_data[0], BUFFER_SIZE);
  //stat_val = HAL_SPI_Receive_DMA_NoStart_NoInterrupt(&SX1257_SDR._spi_I_Data, (uint8_t*) &SX1257_SDR.hdfsdm_I.Instance->CHDATINR, 1);
  if ( stat_val != HAL_OK ) {
    Serial.print("Failed to start SPI1 RX DMA! 0x");
    Serial.print(stat_val, HEX);
    Serial.print(" error code 0x");
    Serial.println(SX1257_SDR._spi_I_Data.ErrorCode, HEX);
  }


  Serial.println("Enabling SPI2 Q data Receive DMA");
  stat_val = HAL_SPI_Receive_DMA_NoStart_NoInterrupt(&SX1257_SDR._spi_Q_Data, (uint8_t*) &sram2_data->Q_data[0], BUFFER_SIZE);
  //stat_val = HAL_SPI_Receive_DMA_NoStart_NoInterrupt(&SX1257_SDR._spi_Q_Data, (uint8_t*) &SX1257_SDR.hdfsdm_Q.Instance->CHDATINR, 1);
  if ( stat_val != HAL_OK ) {
    Serial.print("Failed to start SPI2 RX DMA! 0x");
    Serial.print(stat_val, HEX);
    Serial.print(" error code 0x");
    Serial.println(SX1257_SDR._spi_Q_Data.ErrorCode, HEX);
  }

  Serial.println("STM32 SDR SPI DMA Init successful!");
  //Serial.println("ENABLING STM32 SDR SPI");
  //__HAL_SPI_ENABLE(&SX1257_SDR._spi_I_Data); 
  //__HAL_SPI_ENABLE(&SX1257_SDR._spi_Q_Data); 
}




void stop_spi_dma() {
  Serial.println("Stopping SPI DMA");
  __HAL_DMA_DISABLE(&SX1257_SDR.hdma_spi2_rx);
  __HAL_DMA_DISABLE(&SX1257_SDR.hdma_spi1_rx);
}




bool sdr_init_done = 0;
void stm32_sdr_init()
{
  if ( sdr_init_done == 0 ) {
    sdr_init_done = 1;

    // init in reverse order of how the data flows, basically setup the pipeline
    //stm32_sdr_dfsdm_init();
    stm32_sdr_spi_init();

    // Enable DIO0 from SX1276 as interrupt
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SX1276_DIO0_N; // Replace 'x' with your GPIO pin number
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING; // For rising edge interrupt
    GPIO_InitStruct.Pull = GPIO_PULLDOWN; // Choose pull-up, pull-down, or no-pull
    HAL_GPIO_Init(SX1276_DIO0_G, &GPIO_InitStruct); 

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 3, 0); // Replace 'x' with the EXTI line number, depends on what pin you use
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn); // Replace 'x' with the EXTI line number
    Serial.println("STM32 SDR Init done!");
  }

}

extern "C" {
  bool got_dio0_interrupt = 0;
  void EXTI15_10_IRQHandler(void)
  {

    //__HAL_GPIO_EXTI_CLEAR_IT(SX1276_DIO0_N); // Clear the interrupt pending bit for PB9
    HAL_GPIO_EXTI_IRQHandler(SX1276_DIO0_N); // Call the EXTI line callback function

  }

  void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
  {
      if(GPIO_Pin == SX1276_DIO0_N)
      {
        // STOP SPI from SDR
        __HAL_SPI_DISABLE(&SX1257_SDR._spi_Q_Data); 
        __HAL_SPI_DISABLE(&SX1257_SDR._spi_I_Data);
        got_dio0_interrupt = 1;
      }
  }
}


void stm32_lora_test() 
{
  unsigned long start_time = 0;

  got_dio0_interrupt = 0;

  // START SPI from SDR
  __HAL_SPI_ENABLE(&SX1257_SDR._spi_Q_Data); 
  __HAL_SPI_ENABLE(&SX1257_SDR._spi_I_Data);
  SX1257_SDR.set_rx_mode(1, 1); // enable RX SDR path

  SX1276_Lora.beginPacket();
  SX1276_Lora.print("Hello");
  SX1276_Lora.endPacket();

  start_time = millis();
  while ( !got_dio0_interrupt && (millis() - start_time < 1000 ) ) {
    delay(1);
  }
  if ( !got_dio0_interrupt ) {
    Serial.println("STM32 LORA TX TEST DIDNT GET DIO0 INTERRUPT, END TEST EARLY");
    return;
  } else {
    Serial.println("STM32 LORA TX TEST GOT DIO0 INTERRUPT");
  }

  // now have data in I and Q arrays in sram1_data
  // need to convert them byte by byte and feed to respective DFSDM
  // MAKE SURE TO CHECK THE DMA COUNTER, need to get the address last written by DMA basically


  
}



/***** Top level APIs ****/


bool check_lora_done()
{
  if ( got_dio0_interrupt ) {
    return 1;
  }
  return 0;
}
void clear_lora_done()
{
  got_dio0_interrupt = 0;
}


void get_lora_iq_pointers(int * last_I, int * last_Q)
{
  *last_I = __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi1_rx);
  *last_Q = __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi2_rx);
}

void check_start_lora_rx()
{

  /*
  if ( !check_lora_done() ) { // didn't get interrupt
    if ( !lora_rx_running ) {
      Serial.println("Check start LoRA RX, starting RX path again");

      // enable SDR
      SX1257_SDR.set_antenna(0); // setup SDR for RX
      SX1276_Lora.setantenna(1, 1, 0); // high frequency SMA SX1276->TX on standard transceiver
      // setup SDR RX
      SX1257_SDR.set_rx_parameters(0x6, 0xf, 0x7, 0x1, 0x1);
      SX1257_SDR.set_rx_mode(1, 1); // enable SDR RX path

      // reset FPGA, a bit rough but keeps the SPI peripheral in sync with FPGA
      // start RX 

      hold_ice40_reset();
      delayMicroseconds(2); // something small
      __HAL_SPI_ENABLE(&SX1257_SDR._spi_I_Data); 
      __HAL_SPI_ENABLE(&SX1257_SDR._spi_Q_Data); 
      release_ice40_reset();

      lora_rx_running = 1;
    }
  }
  */
}

void force_restart_lora_rx() {
  Serial.println("Forcing restart LoRA RX, starting RX path again");

  // enable SDR
  SX1257_SDR.set_antenna(0); // setup SDR for RX
  SX1276_Lora.setantenna(1, 1, 0); // high frequency SMA SX1276->TX on standard transceiver
  // setup SDR RX
  SX1257_SDR.set_rx_parameters(0x6, 0xf, 0x7, 0x1, 0x1);
  SX1257_SDR.set_rx_mode(1, 1); // enable SDR RX path

  SX1276_Lora.receive(40);

  // reset FPGA, a bit rough but keeps the SPI peripheral in sync with FPGA
  // start RX 

  hold_ice40_reset();
  __HAL_SPI_DISABLE(&SX1257_SDR._spi_I_Data); 
  __HAL_SPI_DISABLE(&SX1257_SDR._spi_Q_Data); 
  delayMicroseconds(5); // something small
  __HAL_SPI_ENABLE(&SX1257_SDR._spi_I_Data); 
  __HAL_SPI_ENABLE(&SX1257_SDR._spi_Q_Data); 
  delayMicroseconds(5);
  release_ice40_reset();
}



void dump_lora_iq_from_oldest() 
{
  int last_I;
  int last_Q;
  int i_index;
  int q_index;
  get_lora_iq_pointers(&last_I, &last_Q);
  sprintf(print_buffer, "Dump IQ, last_I=%d, last_Q=%d\r\n", last_I, last_Q);
  Serial.print(print_buffer);
  Serial.println("*******Printing IQ from oldest *******");
  for ( int i = 0; i < BUFFER_SIZE; i++ ) {
    i_index = (last_I + 1 + i) % BUFFER_SIZE;
    q_index = (last_Q + 1 + i) % BUFFER_SIZE;
    sprintf(print_buffer,"I=%08d, I=0x%x, Q=0x%x\r\n", i, sram1_data->I_data[i_index] & 0xffff, sram2_data->Q_data[q_index] & 0xffff );
    Serial.print(print_buffer);
  }
}

void compute_phase_from_lora_iq(phaseUnion * phi)
{
  Serial.println("Compute phase from LoRA IQ place holder!");
  //dump_lora_iq_from_oldest();
  if ( phi != 0 ) {
    phi->value = 0; // 0 for now
  }

}


