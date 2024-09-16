

#include "stm32_sdr.h"


static uint8_t cur_lna_gain, cur_base_gain;

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
  uint8_t lna_gain_at_dio0, base_gain_at_dio0;

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
		lna_gain_at_dio0 = cur_lna_gain;
		base_gain_at_dio0 = cur_base_gain;
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
    sprintf(print_buffer,"Index=%08d, I=0x%x, Q=0x%x\r\n", 
		start+i, sram1_data->I_data[i_index] & 0xffff, 
		sram2_data->Q_data[q_index] & 0xffff );
    Serial.print(print_buffer);
  }
  Serial.flush();
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
  sprintf(print_buffer, "SX1257 gain settings, LNA=0x%x, baseband=0x%x, at interrupt LNA=0x%x base=0x%x\r\n",
	cur_lna_gain, cur_base_gain, lna_gain_at_dio0, base_gain_at_dio0);
  Serial.print(print_buffer); Serial.flush();
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





inline float compute_phase_from_index_start_earliest(int index) 
{
  int last_I, last_Q;
  get_lora_iq_pointers(&last_I, &last_Q);
  float I_f32 = (float)  ((int16_t)(sram1_data->I_data[ (last_I + 1 + index) % BUFFER_SIZE ] & 0xffff));
  float Q_f32 = (float) ((int16_t)(sram2_data->Q_data[ (last_Q + 1 + index) % BUFFER_SIZE ] & 0xffff));
  /*
  sprintf(print_buffer,"Compute phase from index, index=%d, int_I=0x%x I_f32=%f , int_Q=0x%x, Q_f32=%f\r\n",
    index, 
    (sram1_data->I_data[ (last_I + 1 + index) % BUFFER_SIZE ] & 0xffff), I_f32,
    (sram2_data->Q_data[ (last_Q + 1 + index) % BUFFER_SIZE ] & 0xffff), Q_f32
  );
  Serial.print(print_buffer);
  */
  
  float return_val = 0;
  arm_atan2_f32(Q_f32, I_f32, &return_val);
  /*
  sprintf(print_buffer,"Index %d, Computed phase %f\r\n", index, return_val);
  Serial.print(print_buffer);
  */
  
  return return_val;
}




float compute_end_of_packet_amplitude() 
{
  float sum = 0;
  int end_of_packet_count = 5;
  for ( int i = 0; i < end_of_packet_count; i++ ) {
    sum += compute_amplitude_from_index_start_earliest(BUFFER_SIZE - i - 1);
  }
  sum = sum / ((float)end_of_packet_count);
  return sum;
}

float compute_mid_packet_amplitude()
{
  float sum = 0;
  int end_of_packet_count = 5;
  int mid_packet_offset = 3000;
  for ( int i = 0; i < end_of_packet_count; i++ ) {
    sum += compute_amplitude_from_index_start_earliest(BUFFER_SIZE - mid_packet_offset - i - 1);
  }
  sum = sum / ((float)end_of_packet_count);
  return sum;
}


// returns the end index (when packet is actually over) with respect to beginning of the buffer (earliest sample)
// Function to find the end of the packet based on amplitude drop
// Function to find the end of the packet based on amplitude drop, working backwards
int find_index_end_of_packet() {
    float initial_amplitude, current_amplitude, amplitude_threshold;

    // Get the initial amplitude (at the oldest sample, index 0)
    initial_amplitude = compute_amplitude_from_index_start_earliest(0);
    //sprintf(print_buffer, "Initial amplitude: %f\r\n", initial_amplitude);
    //Serial.print(print_buffer);

    // Set the threshold to one-third of the initial amplitude
    amplitude_threshold = initial_amplitude / 3.0f;
    //sprintf(print_buffer, "Amplitude threshold (1/3 of initial): %f\r\n", amplitude_threshold);
    //Serial.print(print_buffer);

    // Start from the most recent sample and work backwards
    for (int i = BUFFER_SIZE - 1; i >= 0; i--) {
        current_amplitude = compute_amplitude_from_index_start_earliest(i);
        //sprintf(print_buffer, "Amplitude at index=%d: %f\r\n", i, current_amplitude);
        //Serial.print(print_buffer);

        // Check if the current amplitude is less than the threshold
        if (current_amplitude >= amplitude_threshold) {
			//sprintf(print_buffer, "Packet end detected at index %d\r\n", i);
			//Serial.print(print_buffer);
			return i;
        }
    }

    // If no drop is detected, return -1 (indicating no packet end found)
    //sprintf(print_buffer, "No packet end detected\r\n");
    //Serial.print(print_buffer);
    return -1;
}



#define TWO_PI (2.0 * M_PI)


// Function to wrap a phase value to [-M_PI, M_PI]
float wrap_phase(float phase) {
    while (phase > M_PI) {
        phase -= 2 * M_PI;
    }
    while (phase < -M_PI) {
        phase += 2 * M_PI;
    }
    return phase;
}


// Function to unwrap phase consistently
float unwrap_phase(float current_phase, float *cumulative_phase_correction, float previous_phase) {
    float delta_phase = current_phase - previous_phase;

    // Adjust the cumulative phase correction based on the wraparound
    if (delta_phase > M_PI) {
        *cumulative_phase_correction += 2 * M_PI;  // Positive wrap
    } else if (delta_phase < -M_PI) {
        *cumulative_phase_correction -= 2 * M_PI;  // Negative wrap
    }

    // Return the unwrapped phase
    return current_phase + *cumulative_phase_correction;
}




#define PHASE_BUFFER_SIZE 1500
#define SLOPE_WINDOW_SIZE 15
#define WINDOW_SIZE 100
static float phase_buffer[PHASE_BUFFER_SIZE];  // Buffer to store unwrapped phases

// Function to compute unwrapped phases and detect where the slope changes sign
int find_phase_flattening(int end_index, float * calc_phase) {
    int buffer_idx = 0;  // Index for storing in phase_buffer
    float current_phase, previous_phase, unwrapped_phase;
    float cumulative_phase_correction = 0.0f;  // To track the total phase correction over time
    float slope = 0.0, previous_slope = 0.0;
    int slope_idx_start = 0;

    //sprintf(print_buffer, "\r\n========== Start of Phase Unwrapping and Slope Detection ==========\r\n");
    //Serial.print(print_buffer);

    // Step 1: Compute the phase at the end_index (starting point)
    previous_phase = compute_phase_from_index_start_earliest(end_index);
    unwrapped_phase = previous_phase;

    // Store the first unwrapped phase in the buffer
    phase_buffer[buffer_idx++] = unwrapped_phase;
    //sprintf(print_buffer, "Index=%d, Initial Phase=%f, Unwrapped Phase=%f\r\n", end_index, previous_phase, unwrapped_phase);
    //Serial.print(print_buffer);

    // Step 2: Walk backwards through the data from the packet end
    for (int i = end_index - 1; i >= 0 && buffer_idx < PHASE_BUFFER_SIZE; i--) {
        // Get the current phase at index i
        current_phase = compute_phase_from_index_start_earliest(i);

        // Step 3: Unwrap the phase by detecting phase jumps
        float delta_phase = current_phase - previous_phase;
        if (delta_phase > M_PI) {
            cumulative_phase_correction -= 2 * M_PI;  // Phase wrapped forward
        } else if (delta_phase < -M_PI) {
            cumulative_phase_correction += 2 * M_PI;  // Phase wrapped backward
        }

        // Compute the unwrapped phase
        unwrapped_phase = current_phase + cumulative_phase_correction;
        phase_buffer[buffer_idx++] = unwrapped_phase;

        // Debug print for unwrapped phase
        //sprintf(print_buffer, "Index=%d, Phase=%f, Delta Phase=%f, Unwrapped Phase=%f\r\n", i, current_phase, delta_phase, unwrapped_phase);
        //Serial.print(print_buffer);

        // Step 4: Check if we have enough points to calculate the slope (at least 5 points)
        if (buffer_idx >= SLOPE_WINDOW_SIZE) {
            float slope_sum = 0.0;
            for (int j = 0; j < SLOPE_WINDOW_SIZE - 1; j++) {
                slope_sum += phase_buffer[buffer_idx - j - 1] - phase_buffer[buffer_idx - j - 2];
            }
            slope = slope_sum / SLOPE_WINDOW_SIZE;  // Compute the average slope over 5 points

            //sprintf(print_buffer, "Index=%d, Slope over %d points: %f\r\n", i, SLOPE_WINDOW_SIZE, slope);
            //Serial.print(print_buffer);

            // Step 5: Detect slope change (either positive-to-negative or negative-to-positive)
            if (previous_slope != 0 && ((previous_slope > 0 && slope < 0) || (previous_slope < 0 && slope > 0))) {
                // Slope changed direction, indicating possible flattening or trend shift
                //sprintf(print_buffer, "**** Slope change detected at index=%d ****\r\n", i);
                //Serial.print(print_buffer);
				*calc_phase = wrap_phase(unwrapped_phase);
				//sprintf(print_buffer, "**** Phase at slope change index, unwrapped=%f, wrapped=%f\r\n", 
				//	unwrapped_phase, *calc_phase);
				//Serial.print(print_buffer);
				
				return i;
                break;
            }

            // Update previous slope for the next iteration
            previous_slope = slope;
        }

        // Update previous phase for the next iteration
        previous_phase = current_phase;
    }

    //sprintf(print_buffer, "========== End of Slope Detection ==========\r\n\r\n");
    //Serial.print(print_buffer);
}





// Function to compute unwrapped phases and detect local maxima
int find_local_maximum(int end_index, float *calc_phase) {
    int buffer_idx = 0;  // Index for storing in phase_buffer
    float current_phase, previous_phase, unwrapped_phase;
    float cumulative_phase_correction = 0.0f;  // To track the total phase correction over time
    int half_window = WINDOW_SIZE / 2;
    int local_max_index = -1;
    float local_max_phase = -1.0f;
    float best_value = -1e9;  // Small number for comparison

    //sprintf(print_buffer, "\r\n========== Start of Phase Unwrapping and Local Maximum Detection ==========\r\n");
    //Serial.print(print_buffer);

    // Step 1: Compute the phase at the end_index (starting point)
    previous_phase = compute_phase_from_index_start_earliest(end_index);
    unwrapped_phase = previous_phase;

    // Store the first unwrapped phase in the buffer
    phase_buffer[buffer_idx++] = unwrapped_phase;
    //sprintf(print_buffer, "Index=%d, Initial Phase=%f, Unwrapped Phase=%f\r\n", end_index, previous_phase, unwrapped_phase);
    //Serial.print(print_buffer);

    // Step 2: Walk backwards through the data from the packet end
    for (int i = end_index - 1; i >= 0 && buffer_idx < PHASE_BUFFER_SIZE; i--) {
        // Get the current phase at index i
        current_phase = compute_phase_from_index_start_earliest(i);

        // Step 3: Unwrap the phase by detecting phase jumps
        float delta_phase = current_phase - previous_phase;
        if (delta_phase > M_PI) {
            cumulative_phase_correction -= 2 * M_PI;  // Phase wrapped forward
        } else if (delta_phase < -M_PI) {
            cumulative_phase_correction += 2 * M_PI;  // Phase wrapped backward
        }

        // Compute the unwrapped phase
        unwrapped_phase = current_phase + cumulative_phase_correction;
        phase_buffer[buffer_idx++] = unwrapped_phase;

        // Debug print for unwrapped phase
        //sprintf(print_buffer, "Index=%d, buffer_idx=%d, Phase=%f, Delta Phase=%f, Unwrapped Phase=%f\r\n", i, buffer_idx,
		//	current_phase, delta_phase, unwrapped_phase);
        //Serial.print(print_buffer);

        // Ensure at least half_window samples are unwrapped before checking for local maxima
        if (buffer_idx < WINDOW_SIZE) {
            previous_phase = current_phase;
            continue;  // Skip local maximum check until enough samples are unwrapped
        }

        // Step 4: Now use the buffer_idx for phase_buffer to compare local maxima
        // Compare within the half_window range
        int start = (buffer_idx - WINDOW_SIZE > 0) ? buffer_idx - WINDOW_SIZE : 0;
        int end = (buffer_idx < PHASE_BUFFER_SIZE) ? buffer_idx  : PHASE_BUFFER_SIZE - 1;
		int test_index = end - half_window; 

        // Debug print for the comparison range
        //sprintf(print_buffer, "  Checking range [%d, %d] in real data for index %d\r\n", start, end, test_index);
        //Serial.print(print_buffer);

        // Step 5: Check if current sample in phase_buffer is the local maximum
        bool is_local_maximum = true;
        for (int j = start; j <= end; j++) {
            if (phase_buffer[j] > phase_buffer[test_index] && j != test_index) {
                is_local_maximum = false;
                break;
            }
        }

        // If local maximum, update best value
        if (is_local_maximum && phase_buffer[test_index] > best_value) {
            best_value = phase_buffer[test_index];
            local_max_index = i + half_window;  // Use the actual data index `i` here
			 // + half_window because i is looping backwards 
            local_max_phase = phase_buffer[test_index];

            // Debug print for new local maximum
            //sprintf(print_buffer, "  Found new local maximum at index %d, Phase=%f\r\n", local_max_index,
			//	local_max_phase);
            //Serial.print(print_buffer);
			break;
        }

        // Update previous phase for next iteration
        previous_phase = current_phase;
    }

    // Step 6: If a local maximum was found, compute the instant phase using compute_phase_from_index_start_earliest
    if (local_max_index != -1) {
        *calc_phase = compute_phase_from_index_start_earliest(local_max_index);  // Compute phase at local maximum

        // Debug print for returning the local maximum
        //sprintf(print_buffer, "**** Returning Local Maximum Phase=%f, Index=%d ****\r\n", *calc_phase, local_max_index);
        //Serial.print(print_buffer);

        return local_max_index;
    }

    // Debug print for no local maximum found
    sprintf(print_buffer, "**** No Local Maximum Found ****\r\n");
    Serial.print(print_buffer);
	*calc_phase = 0;

    // No local maximum found
    return -1;
}









bool compute_phase_with_end_of_packet_index(float * val, bool rx)
{
  int end_index = 0;
  if ( val == 0 ) {
    return 0;
  }
  if ( rx ) {
	//dump_lora_iq_from_oldest();
  }
  end_index = find_index_end_of_packet();
  if ( end_index == -1 )
  {
    Serial.println("Find index end of packet failed!");
    return 0; // bad return, couldn't compute
  }
  
  int last_I, last_Q;
  get_lora_iq_pointers(&last_I, &last_Q);
  
  sprintf(print_buffer,"End index starting for phase calculation: %d, last I index=%d\r\n", 
	end_index, last_I);
  Serial.print(print_buffer);

  
  
  
  
	int phase_point = 0;
	float calc_phase = 0;
	//phase_flat_point = find_phase_flattening(end_index, &calc_phase);
	phase_point = find_local_maximum(end_index, &calc_phase);
	sprintf(print_buffer, "Calculating phase, end_index = %d, phase_point=%d, calc_phase=%f\r\n",
		end_index, phase_point, calc_phase);
	Serial.print(print_buffer);
	*val = calc_phase;
	return 1;

	/*
  for ( int i = 0; i < SAMPLE_LENGTH; i++ ) {
    phase_buffer[i] = compute_phase_from_index_start_earliest(phase_flat_point + i);
  }
  // do unwrap

  float32_t sum = 0;
  float32_t count = 0;
  int uwcount, uwcount_prev = 0;

  for ( int i = 1; i < SAMPLE_LENGTH; i++ ) {
    if ( (phase_buffer[i] - phase_buffer[i-1]) > M_PI ) {
      uwcount = uwcount_prev -1;
    } else if (  ( phase_buffer[i] - phase_buffer[i-1]  ) < -M_PI ) {
      uwcount = uwcount_prev+1;
    } else {
      uwcount = uwcount_prev;
    }
    phase_buffer_unwrapped[i] = phase_buffer[i] + ((float32_t)uwcount) * 2.0 * M_PI;
    uwcount_prev = uwcount;
  }

  //Serial.print("Unwrapped phase values:");
  sum = 0;
  count = 0;
  for ( int i = 0; i < SAMPLE_LENGTH; i++ ) {
    sum += phase_buffer_unwrapped[i];
    count++;
  }

  // need to implement euclidean modulo
  sum = sum / count; // raw average
  // a = sum
  // n = 2*M_PI
  // floor in C is non ideal , handle the sign myself
  if ( sum < 0 ) {
    count = sum - (-2*M_PI)*ceil(fabs(sum/(2*M_PI)));
  } else {
    count = sum - (2*M_PI)*floor(sum/(2*M_PI));
  }

  sprintf(print_buffer,"Average of unwrapped phase: %f\r\n", count);
  Serial.print(print_buffer);
  *val = count;
  */
  return 1; // good
} 


void compute_phase_from_lora_iq(phaseUnion * phi, bool rx)
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
  //dump_lora_iq_from_oldest_start_count(BUFFER_SIZE-3000-1, 3000);
  compute_phase_with_end_of_packet_index(&phi->value, rx);

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
  //Serial.println("\r\nReceived one packet!");
  *pktlen = SX1276_Lora.parsePacket();

  //Serial.println("IQ Data:");
  //dump_lora_iq_from_oldest();
  //dump_lora_iq_from_oldest_count(500);
  if ( phase != 0 ) {
    compute_phase_from_lora_iq(phase,1);
  } else {
    compute_phase_from_lora_iq(&dummyVal,1);
  }
  
  // should always happen
  rssi = SX1276_Lora.packetRssi();
  snr = SX1276_Lora.packetSnr();
  sprintf(print_buffer, "Lora packet rssi=%d , snr=%f\r\n", rssi, snr);
  //Serial.print(print_buffer);
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
  SX1257_SDR.set_rx_parameters(0x6, 0x2, 0x7, 0x1, 0x1);
  cur_lna_gain = 0x6;
  cur_base_gain = 0x2;

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
    compute_phase_from_lora_iq(&dummyVal,0);
  } else {
    compute_phase_from_lora_iq(phase,0);
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
  SX1257_SDR.set_rx_parameters(0x6, 0x9, 0x7, 0x1, 0x1); // WIRELESS CONFIG WITH GOOD ANTENNA CLOSE TOGETHER ON DESK
  
  cur_lna_gain = 0x6;
  cur_base_gain = 0x9;

  clear_lora_done(); // clear the interrupt flag

  ice40_start_stream(); // make sure this is started

  // put SX chip back into continous receive
  SX1276_Lora.receive();
}




void sdr_iq_drop_gain()
{
	//ice40_stop_stream(); // make sure stream is stopped
	if ( cur_lna_gain != 0x6 ) { // 0x6 is minimum gain for lna
		cur_lna_gain += 1;
	} else if ( cur_base_gain != 0x0 ) { // 0x0 is minimum gain for baseband
		cur_base_gain -= 1;		
	}
	SX1257_SDR.set_rx_gain(cur_lna_gain, cur_base_gain);	
	//ice40_start_stream();
}
void sdr_iq_raise_gain()
{
	//ice40_stop_stream(); // make sure stream is stopped
	if ( cur_lna_gain != 0x1 ) { // 0x1 is maximum gain for lna
		cur_lna_gain -= 1;
	} else if ( cur_base_gain != 0xF ) { // 0xF is max gain for baseband
		cur_base_gain += 1;
	}
	SX1257_SDR.set_rx_gain(cur_lna_gain, cur_base_gain);	
	//ice40_start_stream();
}

void sdr_iq_agc_run()
{
	static int last_I, last_Q, i_index, q_index;
	static int16_t i_val, q_val;
	static unsigned long last_agc_time = 0;
	static int both_zero_count;
	// simple AGC, a few cases
	// check previous few samples 
	// 1. if any of them are zero, drop the gain
	// this seems to be behavior of the sx1257
	// 2. if the "amplitude" (|I| + |Q|) is higher than 3500, lower it 
	// 3. if the "amplitude" (|I| + |Q|) is lower than 1000 , raise it
	
	// limit how often AGC can run 
	if ( micros() - last_agc_time < 10 ) {
		return;
	}

	get_lora_iq_pointers(&last_I, &last_Q);
	both_zero_count = 0;
	
	for ( int i = 0; i < 5; i++ ) 
	{
		q_index = (last_Q - i + BUFFER_SIZE) % BUFFER_SIZE; 		
		i_index = (last_I - i + BUFFER_SIZE) % BUFFER_SIZE;		
		i_val = sram1_data->I_data[i_index];
		q_val = sram2_data->Q_data[q_index];
		
		if ( i_val == 0 && q_val == 0 ) {
			both_zero_count++;
			if ( both_zero_count >= 2 ) {
				// inband method of signaling gain too high????
				sdr_iq_drop_gain();
				break;
			}
			//continue;
		} 		
		if ( i_val < 0 ) {
			i_val *= -1;
		}
		if ( q_val < 0 ) {
			q_val *= -1; // lazy absolute value
		}
		if (  ((q_val + i_val) < 1000) && (q_val != 0) && (i_val != 0) ) {
			sdr_iq_raise_gain();
			break;
		}		
		if ( (q_val + i_val) >= 3500 ) {
			sdr_iq_drop_gain();
			break;
		}
		
	}	
	last_agc_time = micros();	
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