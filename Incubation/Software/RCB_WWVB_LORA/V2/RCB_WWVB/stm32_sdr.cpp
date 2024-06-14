

#include "stm32_sdr.h"




// convert 32 bit bitstream of 1/0 from sigma delta to signed value
inline int16_t convert_spi_to_dfsdm(uint32_t val) {
  //https://stackoverflow.com/questions/15736602/fastest-way-to-count-number-of-1s-in-a-register-arm-assembly
  // count the number of bits set in the value
  val = val - ((val >> 1) & 0x55555555);                    // reuse input as temporary
  val = (val & 0x33333333) + ((val >> 2) & 0x33333333);     // temp
  val = ((val + (val >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
  // convert that to value based on 1 being 1 and 0 being -1 for sigma delta
  return (int16_t)( (2 * val) - 32 ); 
}

inline uint32_t calc_ampl(uint32_t i_val, uint32_t q_val) {
  int32_t i_analog;
  int32_t q_analog;
  i_analog = (int32_t) convert_spi_to_dfsdm(i_val);
  q_analog = (int32_t) convert_spi_to_dfsdm(q_val);

  // not efficient, but can do later
  return (uint32_t) sqrt(i_analog * i_analog + q_analog * q_analog);
}




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


void print_dfsdm_filter_regs(char * name, DFSDM_Filter_HandleTypeDef * hfilt_in)
{
  DFSDM_Filter_TypeDef * hfilt = hfilt_in->Instance;
  char print_buf[256];
  sprintf(print_buf, "%s regs: FLTCR1=0x%x FLTCR2=0x%x FLTISR=0x%x "
    "FLTJCHGR=0x%x FLTFCR=0x%x FLTJDATAR=0x%x FLTRDATAR=0x%x "
    "FLTAWHTR=0x%x FLTAWLTR=0x%x FLTAWSR=0x%x FLTAWCFR=0x%x "
    "FLTEXMAX=0x%x FLTEXMIN=0x%x FLTCNVTIMR=0x%x",
    name, hfilt->FLTCR1, hfilt->FLTCR2, hfilt->FLTISR,
    hfilt->FLTJCHGR, hfilt->FLTFCR, hfilt->FLTJDATAR, hfilt->FLTRDATAR,
    hfilt->FLTAWHTR, hfilt->FLTAWLTR, hfilt->FLTAWSR, hfilt->FLTAWCFR,
    hfilt->FLTEXMAX, hfilt->FLTEXMIN, hfilt->FLTCNVTIMR);
  Serial.println(print_buf);
}

void print_dfsdm_regs(char * name, DFSDM_Channel_HandleTypeDef * hdfsdm_in)
{
  DFSDM_Channel_TypeDef * hdfsdm = hdfsdm_in->Instance;
  char print_buf[256];
  sprintf(print_buf, "%s regs: CHCFGR1=0x%x CHCFGR2=0x%x "
   "CHAWSCDR=0x%x CHWDATAR=0x%x CHDATINR=0x%x", name,
   hdfsdm->CHCFGR1, hdfsdm->CHCFGR2, hdfsdm->CHAWSCDR,
   hdfsdm->CHWDATAR, hdfsdm->CHDATINR);
  Serial.println(print_buf);
}

uint32_t GetDFSDMClockFreq(void) {
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;
    uint32_t pFLatency = 0;
    uint32_t dfsdmClockFreq = 0;

    // Get current clock settings
    HAL_RCC_GetOscConfig(&RCC_OscInitStruct);
    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &pFLatency);

    // Assuming DFSDM is on APB2 bus - adjust if it's on a different APB bus
    switch (RCC_ClkInitStruct.APB2CLKDivider) {
        case RCC_HCLK_DIV1: dfsdmClockFreq = HAL_RCC_GetPCLK2Freq(); break;
        case RCC_HCLK_DIV2: dfsdmClockFreq = HAL_RCC_GetPCLK2Freq() / 2; break;
        case RCC_HCLK_DIV4: dfsdmClockFreq = HAL_RCC_GetPCLK2Freq() / 4; break;
        case RCC_HCLK_DIV8: dfsdmClockFreq = HAL_RCC_GetPCLK2Freq() / 8; break;
        case RCC_HCLK_DIV16: dfsdmClockFreq = HAL_RCC_GetPCLK2Freq() / 16; break;
        default: dfsdmClockFreq = HAL_RCC_GetPCLK2Freq(); break;
    }

    return dfsdmClockFreq;
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
  stat_val = HAL_SPI_Receive_DMA_NoStart_NoInterrupt(&SX1257_SDR._spi_Q_Data, (uint8_t*) &sram1_data->Q_data[0], BUFFER_SIZE);
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

void stm32_sdr_spi_test() 
{
  /*
  // simple test , SPI should be running, just poll SPI data register
  unsigned long start_time = 0;
  uint8_t prev_val_I = 0;
  uint8_t prev_val_Q = 0;
  int spi_count = 0;
  char print_buf[256];
  start_time = millis();

  Serial.println("STM32 SDR SPI Test start");

  __HAL_SPI_ENABLE(&SX1257_SDR._spi_I_Data); // SPI not started before here
  __HAL_SPI_ENABLE(&SX1257_SDR._spi_Q_Data); // SPI not started before here


  print_spi_registers("SPI1 I before SPI test", &SX1257_SDR._spi_I_Data);
  print_spi_registers("SPI2 Q before SPI test", &SX1257_SDR._spi_Q_Data);

  print_dma_registers("SPI1 I DMA before SPI test",(DMA_Stream_TypeDef *)(SX1257_SDR.hdma_spi1_rx.Instance) );
  print_dma_registers("SPI2 Q DMA before SPI test",(DMA_Stream_TypeDef *)(SX1257_SDR.hdma_spi2_rx.Instance) );




  // run for one second, check that the SPI data reg is changing and the dma_val is correct
  // don't print to serial , just store them in local array and print at the end
  prev_val_I = spi1_rx_data;
  prev_val_Q = spi2_rx_data;
  while ( millis() - start_time < 1000 ) {
    if ( spi1_rx_data != prev_val_I  ) {
      sprintf(print_buf, "Saw I change cur_val=0x%x prev_val_I=0x%x", spi1_rx_data,prev_val_I);
      Serial.println(print_buf);
      prev_val_I = spi1_rx_data;
      spi_count++;
    }
    if ( spi2_rx_data != prev_val_Q ) {
      sprintf(print_buf, "Saw Q change cur_val=0x%x prev_val_Q=0x%x", spi2_rx_data,prev_val_Q);
      Serial.println(print_buf);
      prev_val_Q = spi2_rx_data;
      spi_count++;
    }
    if ( spi_count >= 127 ) {
      break;
    } 
  }
  if ( spi_count >= 127 ) {
    Serial.println("Saw DMA changing spi1_rx_data, TEST PASS");
  } else {
    Serial.println("Didn't see DMA changing spi1_rx_data, TEST FAIL");
  }

  Serial.println("");
  print_spi_registers("SPI1 I after SPI test", &SX1257_SDR._spi_I_Data);
  print_spi_registers("SPI2 Q after SPI test", &SX1257_SDR._spi_Q_Data);

  print_dma_registers("SPI1 I DMA after SPI test",(DMA_Stream_TypeDef *)(SX1257_SDR.hdma_spi1_rx.Instance) );
  print_dma_registers("SPI2 Q DMA after SPI test",(DMA_Stream_TypeDef *)(SX1257_SDR.hdma_spi2_rx.Instance) );

  __HAL_SPI_DISABLE(&SX1257_SDR._spi_I_Data); // disable it again
  __HAL_SPI_DISABLE(&SX1257_SDR._spi_Q_Data); // disable it again
  */
}

/**************************** DFSDM output ************************/

DMA_HandleTypeDef dma_dfsdm_I;
DMA_HandleTypeDef dma_dfsdm_Q;



void start_dfsdm_dma() {
  /*
  Serial.println("Starting DFSDM DMA");
  // start the DMA
  if ( HAL_DFSDM_FilterRegularStart_DMA_Debug(&SX1257_SDR.hdfsdm_filt_I,
    (int32_t*)&sram1_data->filtered_I_buffer[0],FILTERED_BUFFER_SIZE) != HAL_OK ) {
    Serial.println("FAILED TO START DFSDM FILTER START DMA FOR I");
  }
  if ( HAL_DFSDM_FilterRegularStart_DMA_Debug(&SX1257_SDR.hdfsdm_filt_Q,
    (int32_t*)&sram1_data->filtered_Q_buffer[0],FILTERED_BUFFER_SIZE) != HAL_OK ) {
    Serial.println("FAILED TO START DFSDM FILTER START DMA FOR Q");
  }
  */
}

void stop_dfsdm_dma() {
  Serial.println("Stopping DFSDM DMA");
  
  __HAL_DMA_DISABLE(SX1257_SDR.hdfsdm_filt_I.hdmaReg);
  __HAL_DMA_DISABLE(SX1257_SDR.hdfsdm_filt_Q.hdmaReg);
  SX1257_SDR.hdfsdm_filt_I.State = HAL_DFSDM_FILTER_STATE_READY;
  SX1257_SDR.hdfsdm_filt_Q.State = HAL_DFSDM_FILTER_STATE_READY;
}

void stop_spi_dma() {
  Serial.println("Stopping SPI DMA");
  __HAL_DMA_DISABLE(&SX1257_SDR.hdma_spi2_rx);
  __HAL_DMA_DISABLE(&SX1257_SDR.hdma_spi1_rx);
}


void stm32_sdr_dfsdm_init()
{
  // DFSDM filters are setup as part of SX1257
  // here just setup DMA of the output
  // I , RX DMA
  dma_dfsdm_I.Instance = STM32_SDR_DFSDM_I_STREAM; // Example stream, adjust as needed
  dma_dfsdm_I.Init.Request = DMA_REQUEST_DFSDM1_FLT0; // Make sure to use the correct request number
  dma_dfsdm_I.Init.Direction = DMA_PERIPH_TO_MEMORY;
  dma_dfsdm_I.Init.PeriphInc = DMA_PINC_DISABLE;
  dma_dfsdm_I.Init.MemInc = DMA_MINC_ENABLE;
  dma_dfsdm_I.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  dma_dfsdm_I.Init.MemDataAlignment = DMA_PDATAALIGN_WORD; 
  dma_dfsdm_I.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  dma_dfsdm_I.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  dma_dfsdm_I.Init.MemBurst = DMA_MBURST_SINGLE; 
  dma_dfsdm_I.Init.Mode = DMA_CIRCULAR;  // DMA_CIRCULAR for continous
  dma_dfsdm_I.Init.Priority = DMA_PRIORITY_HIGH;

  HAL_DMA_Init(&dma_dfsdm_I);
  // link to filter
  __HAL_LINKDMA(&SX1257_SDR.hdfsdm_filt_I, hdmaReg, dma_dfsdm_I);


  // Q , RX DMA
  dma_dfsdm_Q.Instance = STM32_SDR_DFSDM_Q_STREAM; // Example stream, adjust as needed
  dma_dfsdm_Q.Init.Request = DMA_REQUEST_DFSDM1_FLT1; // Make sure to use the correct request number
  dma_dfsdm_Q.Init.Direction = DMA_PERIPH_TO_MEMORY;
  dma_dfsdm_Q.Init.PeriphInc = DMA_PINC_DISABLE;
  dma_dfsdm_Q.Init.MemInc = DMA_MINC_ENABLE;
  dma_dfsdm_Q.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  dma_dfsdm_Q.Init.MemDataAlignment = DMA_PDATAALIGN_WORD; 
  dma_dfsdm_Q.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  dma_dfsdm_Q.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  dma_dfsdm_Q.Init.MemBurst = DMA_MBURST_SINGLE; 
  dma_dfsdm_Q.Init.Mode = DMA_CIRCULAR; 
  dma_dfsdm_Q.Init.Priority = DMA_PRIORITY_HIGH;

  HAL_DMA_Init(&dma_dfsdm_Q);
  // link to filter
  __HAL_LINKDMA(&SX1257_SDR.hdfsdm_filt_Q, hdmaReg, dma_dfsdm_Q);


  //print_dfsdm_regs("DFSDM Config I", &SX1257_SDR.hdfsdm_I);
  //print_dfsdm_regs("DFSDM Config Q", &SX1257_SDR.hdfsdm_Q);

  //print_dfsdm_filter_regs("DFSDM Filter I" , &SX1257_SDR.hdfsdm_filt_I);
  //print_dfsdm_filter_regs("DFSDM Filter Q" , &SX1257_SDR.hdfsdm_filt_Q);

  start_dfsdm_dma();


}



void stm32_sdr_dfsdm_test() 
{
  /*
  // check first and last value of output buffers, simple test
  char print_buf[256];
  uint32_t prev_I_out_first = 0;
  uint32_t prev_I_out_last = 0;
  uint32_t prev_Q_out_first = 0;
  uint32_t prev_Q_out_last = 0;

  Serial.println("SDR DFSDM Test start!");



  prev_I_out_first = filtered_I_buffer[0];
  prev_I_out_last = filtered_I_buffer[FILTERED_BUFFER_SIZE-1];
  prev_Q_out_first = filtered_Q_buffer[0];
  prev_Q_out_last = filtered_Q_buffer[FILTERED_BUFFER_SIZE-1];

  //__HAL_SPI_ENABLE(&SX1257_SDR._spi_Q_Data); // SPI not started before here
  __HAL_SPI_ENABLE(&SX1257_SDR._spi_I_Data); // SPI not started before here

  Serial.println("SPI Pipeline enabled, starting SDR RX");
  SX1257_SDR.set_rx_mode(1, 1); // enable this here!
  

  delay(10);

  sprintf(print_buf, "I prev_first=0x%x now_first=0x%x prev_last=0x%x now_last=0x%x",
    prev_I_out_first, filtered_I_buffer[0], prev_I_out_last, filtered_I_buffer[FILTERED_BUFFER_SIZE-1]);
  Serial.println(print_buf);

  sprintf(print_buf, "Q prev_first=0x%x now_first=0x%x prev_last=0x%x now_last=0x%x",
    prev_Q_out_first, filtered_Q_buffer[0], prev_Q_out_last, filtered_Q_buffer[FILTERED_BUFFER_SIZE-1]);
  Serial.println(print_buf);
    
  prev_I_out_first = filtered_I_buffer[0];
  prev_I_out_last = filtered_I_buffer[FILTERED_BUFFER_SIZE-1];
  prev_Q_out_first = filtered_Q_buffer[0];
  prev_Q_out_last = filtered_Q_buffer[FILTERED_BUFFER_SIZE-1];

  delay(500);
  Serial.println("After 500 ms");


  sprintf(print_buf, "I prev_first=0x%x now_first=0x%x prev_last=0x%x now_last=0x%x",
    prev_I_out_first, filtered_I_buffer[0], prev_I_out_last, filtered_I_buffer[FILTERED_BUFFER_SIZE-1]);
  Serial.println(print_buf);

  sprintf(print_buf, "Q prev_first=0x%x now_first=0x%x prev_last=0x%x now_last=0x%x",
    prev_Q_out_first, filtered_Q_buffer[0], prev_Q_out_last, filtered_Q_buffer[FILTERED_BUFFER_SIZE-1]);
  Serial.println(print_buf);
    
  __HAL_SPI_DISABLE(&SX1257_SDR._spi_Q_Data); // SPI not started before here
  __HAL_SPI_DISABLE(&SX1257_SDR._spi_I_Data); // SPI not started before here

  Serial.println("");

  for ( int i = 0; i < FILTERED_BUFFER_SIZE; i++ ) {
    if ( i % 5 == 0 ) {
      Serial.println("");
      sprintf(print_buf, "%05d: ", i);
      Serial.print(print_buf);
    }
    sprintf(print_buf, "I=0x%06x=>%02d Q=0x%06x=>%02d, ", 
      filtered_I_buffer[i] >> 8, filtered_I_buffer[i] >> 8,
      filtered_Q_buffer[i] >> 8, filtered_Q_buffer[i] >> 8);
    Serial.print(print_buf);
  }

  Serial.println("SDR DFSDM Test end!");
  Serial.println("");
  print_dfsdm_regs("DFSDM Config I AFTER TEST", &SX1257_SDR.hdfsdm_I);
  print_dfsdm_regs("DFSDM Config Q AFTER TEST", &SX1257_SDR.hdfsdm_Q);

  Serial.println("");

  print_dfsdm_filter_regs("DFSDM Filter I AFTER TEST" , &SX1257_SDR.hdfsdm_filt_I);
  print_dfsdm_filter_regs("DFSDM Filter Q AFTER TEST" , &SX1257_SDR.hdfsdm_filt_Q);

  Serial.print("DFSDM CLock freq: "); Serial.println(GetDFSDMClockFreq());

  */
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

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 3, 0); // Replace 'x' with the EXTI line number
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn); // Replace 'x' with the EXTI line number
  }

}

extern "C" {
  bool got_dio0_interrupt = 0;
  void EXTI9_5_IRQHandler(void)
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

