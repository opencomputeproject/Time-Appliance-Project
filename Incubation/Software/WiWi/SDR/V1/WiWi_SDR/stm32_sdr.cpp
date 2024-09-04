

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





/*
void stop_spi_dma() {
  Serial.println("Stopping SPI DMA");
  __HAL_DMA_DISABLE(&SX1257_SDR.hdma_spi2_rx);
  __HAL_DMA_DISABLE(&SX1257_SDR.hdma_spi1_rx);
}
*/

extern "C" {
  bool got_dio0_interrupt = 0;   
  SPI_HandleTypeDef * i_spi = 0;
  SPI_HandleTypeDef * q_spi = 0;

  void enable_dio0_interrupt() { //TXDONE / RXDONE
    // Enable DIO0 from SX1276 as interrupt, TXDONE and RXDONE
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SX1276_DIO0_N; // Replace 'x' with your GPIO pin number
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING; // For rising edge interrupt
    GPIO_InitStruct.Pull = GPIO_PULLDOWN; // Choose pull-up, pull-down, or no-pull
    HAL_GPIO_Init(SX1276_DIO0_G, &GPIO_InitStruct); 
    // Enable and set EXTI line 15_10 Interrupt to the lowest priority
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
  }
  /*
  void disable_dio0_interrupt() {
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SX1276_DIO0_N; // Replace 'x' with your GPIO pin number
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // just make it input
    GPIO_InitStruct.Pull = GPIO_PULLDOWN; // Choose pull-up, pull-down, or no-pull
    HAL_GPIO_Init(SX1276_DIO0_G, &GPIO_InitStruct);
  }
  */
  /*
  void enable_dio3_interrupt() { //Validheader
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // Enable DIO3 from SX1276 as interrupt, ValidHeader
    GPIO_InitStruct.Pin = SX1276_DIO3_N; // Replace 'x' with your GPIO pin number
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING; // For rising edge interrupt
    GPIO_InitStruct.Pull = GPIO_PULLDOWN; // Choose pull-up, pull-down, or no-pull
    HAL_GPIO_Init(SX1276_DIO3_G, &GPIO_InitStruct); 
  }
  void disable_dio3_interrupt() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SX1276_DIO3_N; // Replace 'x' with your GPIO pin number
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // just make it input
    GPIO_InitStruct.Pull = GPIO_PULLDOWN; // Choose pull-up, pull-down, or no-pull
    HAL_GPIO_Init(SX1276_DIO3_G, &GPIO_InitStruct);
  }
  */
  void EXTI15_10_IRQHandler(void)
  {
    //__HAL_GPIO_EXTI_CLEAR_IT(SX1276_DIO0_N); // Clear the interrupt pending bit for PB9
    HAL_GPIO_EXTI_IRQHandler(SX1276_DIO0_N); // Call the EXTI line callback function
  }
  /*
  void disable_iq_spi() {
    if ( i_spi != 0 ) {
      //__HAL_SPI_DISABLE(i_spi);
      CLEAR_BIT(i_spi->Instance->CR1, SPI_CR1_SSI);
    }
    if ( q_spi != 0 ) {
      //__HAL_SPI_DISABLE(q_spi);
      CLEAR_BIT(q_spi->Instance->CR1, SPI_CR1_SSI);
    }     
  }
  void enable_iq_spi() {
    if ( i_spi != 0 ) {
      SET_BIT(i_spi->Instance->CR1, SPI_CR1_SSI);
    }
    if ( q_spi != 0 ) {
      SET_BIT(q_spi->Instance->CR1, SPI_CR1_SSI);
    }   
  }
  void disable_iq_spi_dma() {
    if ( i_spi != 0 ) {
      ATOMIC_CLEAR_BIT(i_spi->Instance->CFG1, SPI_CFG1_RXDMAEN);
      ATOMIC_CLEAR_BIT(i_spi->Instance->CFG1, SPI_CFG1_TXDMAEN);
    } 
    if ( q_spi != 0 ) {
      ATOMIC_CLEAR_BIT(q_spi->Instance->CFG1, SPI_CFG1_RXDMAEN);
      ATOMIC_CLEAR_BIT(q_spi->Instance->CFG1, SPI_CFG1_TXDMAEN);
    }
  }
  void enable_iq_spi_dma() {
    if ( i_spi != 0 ) {
      ATOMIC_SET_BIT(i_spi->Instance->CFG1, SPI_CFG1_RXDMAEN);
      ATOMIC_SET_BIT(i_spi->Instance->CFG1, SPI_CFG1_TXDMAEN);
    } 
    if ( q_spi != 0 ) {
      ATOMIC_SET_BIT(q_spi->Instance->CFG1, SPI_CFG1_RXDMAEN);
      ATOMIC_SET_BIT(q_spi->Instance->CFG1, SPI_CFG1_TXDMAEN);
    }
  }
  */
  void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
  {
      if(GPIO_Pin == SX1276_DIO0_N)
      {
        //disable_iq_spi_dma();
        HAL_GPIO_WritePin(ICE_SPARE5_G, ICE_SPARE5_N, GPIO_PIN_RESET); // ICE40 stream disable
        got_dio0_interrupt = 1;
      }
  }
}



void sdr_iq_init() 
{
  HAL_StatusTypeDef stat_val = HAL_OK;
  __HAL_SPI_DISABLE(&SX1257_SDR._spi_I_Data); // make sure disable it again
  __HAL_SPI_DISABLE(&SX1257_SDR._spi_Q_Data); // make sure disable it again
  //SPI1 = I data
  //Serial.println("Enabling SPI1 I data Receive DMA");
  stat_val = HAL_SPI_Receive_DMA_NoStart_NoInterrupt(&SX1257_SDR._spi_I_Data, (uint8_t*) &sram1_data->I_data[0], BUFFER_SIZE);
  if ( stat_val != HAL_OK ) {
    Serial.print("Failed to start SPI1 RX DMA! 0x");
    Serial.print(stat_val, HEX);
    Serial.print(" error code 0x");
    Serial.println(SX1257_SDR._spi_I_Data.ErrorCode, HEX);
  }

  //Serial.println("Enabling SPI2 Q data Receive DMA");
  stat_val = HAL_SPI_Receive_DMA_NoStart_NoInterrupt(&SX1257_SDR._spi_Q_Data, (uint8_t*) &sram2_data->Q_data[0], BUFFER_SIZE);
  if ( stat_val != HAL_OK ) {
    Serial.print("Failed to start SPI2 RX DMA! 0x");
    Serial.print(stat_val, HEX);
    Serial.print(" error code 0x");
    Serial.println(SX1257_SDR._spi_Q_Data.ErrorCode, HEX);
  }
  i_spi = &SX1257_SDR._spi_I_Data;
  q_spi = &SX1257_SDR._spi_Q_Data;

  //enable_iq_spi();
  __HAL_SPI_ENABLE(&SX1257_SDR._spi_I_Data); 
  __HAL_SPI_ENABLE(&SX1257_SDR._spi_Q_Data);   

  ice40_start_stream();
  enable_dio0_interrupt();


}






/***** Top level APIs ****/
void zero_iq_buffers()
{
    // setup the IQ buffers
  for ( int i = 0; i < BUFFER_SIZE; i++ ) {
    sram1_data->I_data[i] = 0;
    sram2_data->Q_data[i] = 0;
  }
}

bool check_lora_done()
{
  if ( got_dio0_interrupt ) {
    //Serial.println("******Check LoRA Done, DIO0 Interrupt seen!*****");
    return 1;
  }
  return 0;
}
void clear_lora_done()
{
  //Serial.println("********Clear lora done, dio0 interrupt*******");
  got_dio0_interrupt = 0;
}


// gives the array index of the last item written to by DMA
// in incrementing fashion, if 2 items transfered, returns 1
inline void get_lora_iq_pointers(int * last_I, int * last_Q)
{
  // Note about these counters
  // It is the number of remaining data items to be transmitted
  // so if buffer is 10000 and this counter is 9000, it means 1000 items have been transferred
  // it won't be zero in my case where I never disable DMA and it runs in circular mode
  // so keep this in mind about going from earliest to latest
  // if counter is 9999, then last index is 0

  // to make this make sense, meaning the index of the last item written
  // its BUFFER_SIZE - counter - 1
  // example: BUFFER_SIZE = 10000
  // counter = 9995
  // means 9995 items left to transfer, so 5 have been transferred
  // last_I = 4, BUFFER_SIZE - counter - 1
  // handle the case where counter == BUFFER_SIZE

  

  if ( __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi1_rx) == BUFFER_SIZE ) {
    *last_I = 0; // max number of items left to transfer, either non transferred or right at the edge of buffer
  } else {
    *last_I = BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi1_rx) - 1;
  }
  //sprintf(print_buffer,"Last I, counter=%d, last_I=%d\r\n", __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi1_rx), *last_I);
  //Serial.print(print_buffer);


  if ( __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi2_rx) == BUFFER_SIZE ) {
    *last_Q = 0; // max number of items left to transfer, either non transferred or right at the edge of buffer
  } else {
    *last_Q = BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi2_rx) - 1;
  }

  //sprintf(print_buffer,"Last Q, counter=%d, last_Q=%d\r\n", __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi2_rx), *last_Q);
  //Serial.print(print_buffer);
}



void dump_lora_iq_from_oldest_start_count(int start, int count) {
  int last_I;
  int last_Q;
  int i_index;
  int q_index;
  get_lora_iq_pointers(&last_I, &last_Q);
  sprintf(print_buffer, "Dump IQ, last_I=%d, last_Q=%d\r\n", last_I, last_Q);
  Serial.print(print_buffer);
  sprintf(print_buffer, "*******Printing IQ from oldest, count %d*******\r\n", count);
  Serial.print(print_buffer);
  for ( int i = 0; i < count; i++ ) {
    i_index = (last_I + 1 + i + start) % BUFFER_SIZE;
    q_index = (last_Q + 1 + i + start) % BUFFER_SIZE;
    sprintf(print_buffer,"I=%08d, I=0x%x, Q=0x%x\r\n", i, sram1_data->I_data[i_index] & 0xffff, sram2_data->Q_data[q_index] & 0xffff );
    Serial.print(print_buffer);
  }
}

void dump_lora_iq_from_oldest_count(int count) 
{
  int last_I;
  int last_Q;
  int i_index;
  int q_index;
  get_lora_iq_pointers(&last_I, &last_Q);
  sprintf(print_buffer, "Dump IQ, last_I=%d, last_Q=%d\r\n", last_I, last_Q);
  Serial.print(print_buffer);
  sprintf(print_buffer, "*******Printing IQ from oldest, count %d*******\r\n", count);
  Serial.print(print_buffer);
  for ( int i = 0; i < count; i++ ) {
    i_index = (last_I + 1 + i) % BUFFER_SIZE;
    q_index = (last_Q + 1 + i) % BUFFER_SIZE;
    sprintf(print_buffer,"I=%08d, I=0x%x, Q=0x%x\r\n", i, sram1_data->I_data[i_index] & 0xffff, sram2_data->Q_data[q_index] & 0xffff );
    Serial.print(print_buffer);
  }
}

void dump_iq_buffer_straight()
{
  Serial.println("*******Printing IQ in order *******");
  for ( int i = 0; i < BUFFER_SIZE; i++ ) {
    sprintf(print_buffer,"I=%08d, I=0x%x, Q=0x%x\r\n", i, sram1_data->I_data[i] & 0xffff, sram2_data->Q_data[i] & 0xffff );
    Serial.print(print_buffer);
  }
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




// Function to compute the phase for a single sample
inline float compute_phase(int16_t I, int16_t Q) {
  float I_f32 = (float)I;
  float Q_f32 = (float)Q;
  float return_val = 0;
  arm_atan2_f32(Q_f32, I_f32, &return_val);
  return return_val;
}

inline float compute_amplitude_fast(int16_t I, int16_t Q) {

  float I_f32 = (float)I;
  float Q_f32 = (float)Q;
  float return_val = 0;
  arm_sqrt_f32( ((I_f32 * I_f32) + (Q_f32 * Q_f32)) , &return_val);
  return return_val;
}

inline float compute_amplitude_from_index_start_earliest(int index) {
  int last_I, last_Q;
  get_lora_iq_pointers(&last_I, &last_Q);
  int16_t I = ((int16_t)(sram1_data->I_data[ (last_I + 1 + index) % BUFFER_SIZE ] & 0xffff));
  int16_t Q = ((int16_t)(sram2_data->Q_data[ (last_Q + 1 + index) % BUFFER_SIZE ] & 0xffff));

  

  float return_val = compute_amplitude_fast(I,Q);

  //sprintf(print_buffer,"Compute amplitude from index, index=%d, int_I=0x%hx , int_Q=0x%hx ampl=%f\r\n",
  //  index, I, Q, return_val);
  //Serial.print(print_buffer);

  return return_val;
}

inline int16_t get_i_from_start(int index) {
  int last_I, last_Q;
  get_lora_iq_pointers(&last_I, &last_Q);
  return ((int16_t)(sram1_data->I_data[ (last_I + 1 + index) % BUFFER_SIZE ] & 0xffff));
}

inline int16_t get_q_from_start(int index) {
  int last_I, last_Q;
  get_lora_iq_pointers(&last_I, &last_Q);
  return ((int16_t)(sram2_data->Q_data[ (last_Q + 1 + index) % BUFFER_SIZE ] & 0xffff));
}



void compute_phase_from_lora_iq(phaseUnion * phi)
{
  //Serial.println("Compute phase from LoRA IQ place holder!");
  if ( SX1276_Lora.getDio0Val() ) {
    //Serial.println("DIO0 is high!");
  } else {
    //Serial.println("DIO0 is low!");
  }
  //dump_lora_iq_from_oldest();
  if ( phi == 0 ) {
    return;
  }
  phi->value = 0; // initialize it

  // not included in this implementation

  return;
}



bool is_receive_packet_available() {
  if ( SX1276_Lora.checkRxDone() ) {
    return 1;
  }
  return 0;
}

bool receive_lora_packet(uint8_t * pkt_data, int * pktlen, phaseUnion * phase)
{
  int rssi = 0;
  int snr = 0;
  phaseUnion dummyVal;
  if (!is_receive_packet_available() ) {
    return 0;
  }
  Serial.println("Received one packet!");
  *pktlen = SX1276_Lora.parsePacket();

  //Serial.println("IQ Data:");
  //dump_lora_iq_from_oldest();
  //dump_lora_iq_from_oldest_count(500);
  if ( phase != 0 ) {
    compute_phase_from_lora_iq(phase);
  } else {
    compute_phase_from_lora_iq(&dummyVal);
  }
  
  // should always happen
  rssi = SX1276_Lora.packetRssi();
  snr = SX1276_Lora.packetSnr();
  sprintf(print_buffer, "Lora packet rssi=%d , snr=%f\r\n", rssi, snr);
  Serial.print(print_buffer);
  Serial.print("Received packet data: ");
  for (int j = 0; j < *pktlen; j++) {
    if ( SX1276_Lora.available() ) {
      pkt_data[j] = (uint8_t)SX1276_Lora.read();
      sprintf(print_buffer, " 0x%x", pkt_data[j]);
      Serial.print(print_buffer);
    }
  }
  Serial.println("");

  zero_iq_buffers(); // computationally expensive, but just for debug

  // done with all this
  // SX1276 is still in continuous RX mode
  // clear IRQs (RXDONE)
  SX1276_Lora.clearIRQs();

  // reenable everything
  clear_lora_done();

  ice40_start_stream();

  //SX1276_Lora.dumpRegisters(Serial);
  return 1;

}

void send_packet(uint8_t * pkt_data, int pktlen, phaseUnion * phase, uint32_t * timestamp)
{
  uint32_t start_time = 0;
  sprintf(print_buffer, "send_packet, pktlen %d\r\n", pktlen );
  Serial.print(print_buffer);  
  for ( int i = 0; i < pktlen; i++ ) {
    sprintf(print_buffer, " 0x%x", pkt_data[i]);
    Serial.print(print_buffer);
  }
  Serial.print("\r\n");



  // SX1276 DIO0 configuration / status  
  // put the LoRA into idle
  SX1276_Lora.idle();
  // clear IRQs, RXDONE / TXDONE 
  SX1276_Lora.clearIRQs();
  // set DIO0 as TXDONE
  SX1276_Lora.setDio0_TxDone();
  // set SX1276 antenna to TX
  SX1276_Lora.setantenna(1,1,1);

  // 5. SX1257 config -> change gain for TX, basically minimal gain
  SX1257_SDR.set_rx_parameters(0x6, 0x1, 0x7, 0x1, 0x1);

  clear_lora_done(); // clear the interrupt flag
  ice40_start_stream(); // make sure this is started

  // setup packet and write it
  SX1276_Lora.disableCrc(); // explicit mode, but not CRC       
  SX1276_Lora.beginPacket(0);
  SX1276_Lora.write( pkt_data, pktlen );
  SX1276_Lora.endPacket(1); // finish packet and send it, non-blocking
  start_time = micros();
  //while ( !SX1276_Lora.getDio0Val() )
  while ( !check_lora_done() )
  {
    // wait for interrupt
    // put some upper limit in time
    if ( micros() - start_time > 500000 )
    {
      Serial.println("endPacket to DIO0 interrupt took too long!");
      break;
    }
  }



  // done with radio

  SX1276_Lora.idle();
  // clear IRQs, RXDONE / TXDONE 
  SX1276_Lora.clearIRQs();


  //dump_lora_iq_from_oldest();
  



  //phaseUnion temp;
  //compute_phase_from_lora_iq(&temp);

  //dump_lora_iq_from_oldest_count(500);
  if ( phase == 0 ) {
    phaseUnion dummyVal;
    compute_phase_from_lora_iq(&dummyVal);
  } else {
    compute_phase_from_lora_iq(phase);
  }
  if ( timestamp != 0 ) {
    *timestamp = micros();
    sprintf(print_buffer,"TX timestamp: %ld\r\n", *timestamp);
    Serial.print(print_buffer);
  }

  zero_iq_buffers(); // a lot of computation, but just for debug

  //SX1276_Lora.dumpRegisters(Serial);

  clear_lora_done(); // clear the interrupt flag

  ice40_start_stream();
  // done with TX, now put it back in continous RX mode
  switch_lora_to_rx();
}

void switch_lora_to_rx()
{
  // put it in idle
  SX1276_Lora.idle();
  // clear interrupts
  SX1276_Lora.clearIRQs();
  // setup dio0 back for RXDONE
  SX1276_Lora.setDio0_RxDone();
  // enable SDR antenna
  SX1257_SDR.set_antenna(0); // set SDR antenna to RX
  // setup antenna for RX
  SX1276_Lora.setantenna(1, 1, 0);


  // 5. SX1257 config -> change gain for RX
  //SX1257_SDR.set_rx_parameters(0x6, 0x1, 0x7, 0x1, 0x1); // CABLED CONFIG, 0x6 / 0x3
  //SX1257_SDR.set_rx_parameters(0x1, 0xF, 0x7, 0x1, 0x1); // WIRELESS CONFIG, 0x1 / 0xF , MAX
  SX1257_SDR.set_rx_parameters(0x6, 0xD, 0x7, 0x1, 0x1); // WIRELESS CONFIG WITH GOOD ANTENNA CLOSE TOGETHER ON DESK

  clear_lora_done(); // clear the interrupt flag

  ice40_start_stream(); // make sure this is started

  // put SX chip back into continous receive
  SX1276_Lora.receive();
}






/**************** TEST CASES *************/

void test_dio0_interrupt()
{
  uint32_t start_time = 0;
  // simple test
  // do a dummy transmit that I know the approx time of airtime
  // poll for dio0 assert
    // put the LoRA into idle
  SX1276_Lora.idle();
  // clear IRQs, RXDONE / TXDONE 
  SX1276_Lora.clearIRQs();
  // set DIO0 as TXDONE
  SX1276_Lora.setDio0_TxDone();
  // set SX1276 antenna to TX
  SX1276_Lora.setantenna(1,1,1);
  // clear interrupt flag
  clear_lora_done();


  // before starting, check the pin status and interrupts
  if ( check_lora_done() ) {
    Serial.println("****************Test dio0 interrupt, saw interrupt when not expected, FAIL*************");
    return;
  }
  if ( wwvb_digital_read(SX1276_DIO0) ) {
    Serial.println("***************DIO0 is high in beginning of TX test, FAIL**********************");
    return;
  }

  SX1276_Lora.disableCrc(); // explicit mode, but not CRC       
  SX1276_Lora.beginPacket(0);
  for ( int i = 0; i < 40; i++ ) {
    SX1276_Lora.write((uint8_t)i);
  }
  SX1276_Lora.endPacket(1); // run it async! I will handle checking when it's done
  
  // should take about 110 milliseconds, but allow 500 milliseconds
  start_time = micros();
  while ( 1 ) {
    if ( (micros() - start_time) > 500000 ) {
      // exceeded time
      Serial.println("************Test dio0 interrupt, TXDONE packet time exceeded 500ms, FAIL*************");
      break;
    }
    if ( wwvb_digital_read(SX1276_DIO0) ) {
      // pin is high, did I also get interrupt?
      if ( check_lora_done() ) {
        Serial.println("************Test dio0 interrupt, DIO0 went high and interrupt fired, PASS!!!!!!!!");
        break;
      } else {
        Serial.println("************Test dio0 interrupt, DIO0 went high, interrupt didn't fire, FAIL**************");
        break;
      }
    }
  }

  // make sure these are enabled
  //enable_iq_spi();
  wwvb_digital_write(ICE_SPARE5, 1);
}

void test_ice40_stream_enable_disable()
{
  // step 1. check that I/Q data is streaming right now
  int last_I, last_Q;
  int next_I, next_Q;

  get_lora_iq_pointers(&last_I, &last_Q);

  // streaming at 1Msps, so 1 sample every microsecond
  // just give it some time and check the counters are changing
  delayMicroseconds(10);
  get_lora_iq_pointers(&next_I, &next_Q);

  if ( next_I == last_I ) {
    Serial.println("**************Test ice40 stream disable, I not incrementing, FAIL************");
    return;
  }
  if ( next_Q == last_Q ) {
    Serial.println("**************Test ice40 stream disable, Q not incrementing, FAIL************");
    return;
  }
  // ok so data is streaming now, should be able to stop it with the stream control
  ice40_stop_stream();
  get_lora_iq_pointers(&last_I, &last_Q);
  delayMicroseconds(10);
  get_lora_iq_pointers(&next_I, &next_Q);
  if ( next_I != last_I ) {
    Serial.println("****************Test ice40 stream disable, I incremented, FAIL*************");
    return;
  }
  if ( next_Q != last_Q ) {
    Serial.println("****************Test ice40 stream disable, Q incremented, FAIL*************");
    return;
  }
  ice40_start_stream();
  delayMicroseconds(10);
  get_lora_iq_pointers(&last_I, &last_Q);
  if ( last_I == next_I ) {
    Serial.println("*****************Test ice40 stream disable, I didn't increment after reenable, FAIL*************");
    return;
  }
  if ( last_Q == next_Q ) {
    Serial.println("****************Test ice40 stream disable, Q didn't increment after reenable, FAIL**************");
    return;
  }

  Serial.println("****************Test ice40 stream disable, PASS!!!!!!!!!!!!!!!!");
  return;
}



void test_ice40_reset() {
  // basically same test as stream disable enable, but with the reset


    // step 1. check that I/Q data is streaming right now
  int last_I, last_Q;
  int next_I, next_Q;
  get_lora_iq_pointers(&last_I, &last_Q);

  // streaming at 1Msps, so 1 sample every microsecond
  // just give it some time and check the counters are changing
  delayMicroseconds(10);
  get_lora_iq_pointers(&next_I, &next_Q);

  if ( next_I == last_I ) {
    Serial.println("**************Test ice40 reset, I not incrementing, FAIL************");
    return;
  }
  if ( next_Q == last_Q ) {
    Serial.println("**************Test ice40 reset, Q not incrementing, FAIL************");
    return;
  }
  // ok so data is streaming now, should be able to stop it with the stream control
  hold_ice40_reset();
  get_lora_iq_pointers(&last_I, &last_Q);
  delayMicroseconds(10);
  get_lora_iq_pointers(&next_I, &next_Q);
  if ( next_I != last_I ) {
    Serial.println("****************Test ice40 reset, I incremented, FAIL*************");
    return;
  }
  if ( next_Q != last_Q ) {
    Serial.println("****************Test ice40 reset, Q incremented, FAIL*************");
    return;
  }
  release_ice40_reset();
  delayMicroseconds(10);
  get_lora_iq_pointers(&last_I, &last_Q);
  if ( last_I == next_I ) {
    Serial.println("*****************Test ice40 reset, I didn't increment after reenable, FAIL*************");
    return;
  }
  if ( last_Q == next_Q ) {
    Serial.println("****************Test ice40 reset, Q didn't increment after reenable, FAIL**************");
    return;
  }

  Serial.println("****************Test ice40 reset, PASS!!!!!!!!!!!!!!!!");
  return;
}


void test_ice40_fixed_pattern()
{
  // need special FPGA image that drives a known data pattern
  // I wrote my test FPGA to shift an increasing counter value, so the int16_t pattern should just continously increase
  int last_I, last_Q;
  int next_I, next_Q;
  int i = 0;
  

  //get_lora_iq_pointers(&last_I, &last_Q);
  last_I = 0;
  last_Q = 0;
  

  // wait a small time
  delayMicroseconds(500);
  //ice40_stop_stream();

  get_lora_iq_pointers(&next_I, &next_Q);

  // print the data in order, but wrap around
  i = last_I;
  Serial.println("*********Test ICE40 fixed pattern, data:");
  while ( i != next_I ) {
    sprintf(print_buffer,"I=0x%x, Q=0x%x\r\n", sram1_data->I_data[i] & 0xffff, 
      sram2_data->Q_data[i] & 0xffff );
    Serial.print(print_buffer);
    i++;
    if ( i == BUFFER_SIZE ) {
      i = 0;
    }    
  }

  Serial.println(" Test ice40 fixed pattern, test 1 done");
  Serial.println("");
  Serial.println("");

  // wait a small time
  delayMicroseconds(500);
  //ice40_stop_stream();

  get_lora_iq_pointers(&next_I, &next_Q);

  // print the data in order, but wrap around
  i = last_I;
  Serial.println("*********Test ICE40 fixed pattern, data:");
  while ( i != next_I ) {
    sprintf(print_buffer,"I=0x%x, Q=0x%x\r\n", sram1_data->I_data[i] & 0xffff, 
      sram2_data->Q_data[i] & 0xffff );
    Serial.print(print_buffer);
    i++;
    if ( i == BUFFER_SIZE ) {
      i = 0;
    }    
  }

}