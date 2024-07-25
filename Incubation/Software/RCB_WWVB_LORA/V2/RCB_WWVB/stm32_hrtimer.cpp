#include "stm32_hrtimer.h"


void debug_print_hrtimer_common_registers() 
{
  sprintf(print_buffer,"HRTimer Common registers:\r\n");
  sprintf(print_buffer,"%s EECR1=0x%x\r\n", print_buffer, HRTIM1->sCommonRegs.EECR1);
  sprintf(print_buffer,"%s EECR2=0x%x\r\n", print_buffer, HRTIM1->sCommonRegs.EECR2);
  sprintf(print_buffer,"%s EECR3=0x%x\r\n", print_buffer, HRTIM1->sCommonRegs.EECR3);
  Serial.print(print_buffer);  
}

void debug_print_hrtimer_registers(int TimerIdx)
{
  sprintf(print_buffer,"Timer %d registers:\r\n", TimerIdx);
  sprintf(print_buffer,"%s TIMxCR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].TIMxCR);
  sprintf(print_buffer,"%s TIMxISR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].TIMxISR);
  sprintf(print_buffer,"%s TIMxICR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].TIMxICR);
  sprintf(print_buffer,"%s TIMxDIER=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].TIMxDIER);
  sprintf(print_buffer,"%s CNTxR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].CNTxR);
  sprintf(print_buffer,"%s PERxR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].PERxR);
  sprintf(print_buffer,"%s REPxR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].REPxR);
  sprintf(print_buffer,"%s CMP1xR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].CMP1xR);
  sprintf(print_buffer,"%s CMP1CxR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].CMP1CxR);
  sprintf(print_buffer,"%s CMP2xR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].CMP2xR);
  sprintf(print_buffer,"%s CMP3xR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].CMP3xR);
  sprintf(print_buffer,"%s CMP4xR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].CMP4xR);
  sprintf(print_buffer,"%s CPT1xR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].CPT1xR);
  sprintf(print_buffer,"%s CPT2xR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].CPT2xR);
  sprintf(print_buffer,"%s DTxR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].DTxR);
  sprintf(print_buffer,"%s SETx1R=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].SETx1R);
  sprintf(print_buffer,"%s RSTx1R=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].RSTx1R);
  

  sprintf(print_buffer,"%s SETx2R=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].SETx2R);
  sprintf(print_buffer,"%s RSTx2R=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].RSTx2R);
  sprintf(print_buffer,"%s EEFxR1=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].EEFxR1);
  sprintf(print_buffer,"%s EEFxR2=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].EEFxR2);
  sprintf(print_buffer,"%s RSTxR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].RSTxR);
  sprintf(print_buffer,"%s CHPxR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].CHPxR);
  sprintf(print_buffer,"%s CPT1xCR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].CPT1xCR);
  sprintf(print_buffer,"%s CPT2xCR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].CPT2xCR);
  sprintf(print_buffer,"%s OUTxR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].OUTxR);
  sprintf(print_buffer,"%s FLTxR=0x%x\r\n", print_buffer, HRTIM1->sTimerxRegs[TimerIdx].FLTxR);

  Serial.print(print_buffer);  

}



/******* PPS Input variables **********/
// just defining a set of global variables
// assume PPS input is slow enough, don't need any fifo stuff
// all of these values are with respect to ART, not PHC
uint64_t art_val_ts = 0;
uint16_t capture_ts = 0;
uint16_t dma_remaining_ts = 0;
uint16_t typical_count_ts = 0;
uint64_t millis_ts = 0;
bool valid_ts = 0;


// STM HAL API variables
extern "C" {
  DMA_HandleTypeDef hdma_hrtim;
  HRTIM_HandleTypeDef hhrtim;  


  /********* HRTimer management variables ***********/

  uint64_t dma_complete_counter = 0;
  uint64_t timc_interrupt_counter = 0;
  uint64_t rising_edge_counter = 0;
  uint64_t falling_edge_counter = 0;


  uint16_t next_lowlevel_typical_count = 34277;
  uint16_t next_lowlevel_fine_count = 61000;
  uint16_t next_highlevel_typical_count = 34277;
  uint16_t next_highlevel_fine_count = 61000;

  bool output_level = 0;
  bool set_next_output = 1; // has to start out as 1! output starts at reset
  // if you don't do this , it will mess up

  /********* ART counters ***********/
  uint64_t art_ticks = 0; // just total number of ticks of hrtimer
  const double nano_per_tick = ((1.0/480.0e6)*1e9);

  /********* Timestamp value **********/
  // I don't need to actually timestamp PPS Output
  // just keep track of last time I generated a rising edge
  // compare that to PPS input and that's the error signal more or less
  uint64_t art_time_last_rising = 0;
}

uint64_t get_last_rising_edge_timestamp() {
  return art_time_last_rising;
}
// X is DMA roll over counter, Y is the single fine roll over
// utility calculation function
void compute_timing_adjust_ns(double adjustment_ns, uint16_t *X, uint16_t *Y) {
  // Constants
  const double clock_frequency_hz = 480000000.0; // 480 MHz
  const double clock_period_ns = 1e9 / clock_frequency_hz; // Clock period in nanoseconds
  if ( adjustment_ns > 400e6 ) {
    Serial.println("compute_timing_adjust_ns: Adjustment_ns too large, capping high");
    adjustment_ns = 400e6;
  } else if ( adjustment_ns < -400e6 ) {
    Serial.println("compute_timing_adjust_ns: Adjustment_ns too negative, capping low");
    adjustment_ns = -400e6;
  }
  // compute how many counter ticks to adjust by
  int64_t tick_adjust = (int64_t) (adjustment_ns / clock_period_ns);
  int64_t cur_ticks = (*X * DMA_PPS_COUNT) + *Y;
  cur_ticks += tick_adjust;
  uint32_t start_dma_hrtimer_count = 0;
  start_dma_hrtimer_count = (uint32_t) (cur_ticks - 0xffdf) / DMA_PPS_COUNT ;
  uint32_t remainder = 0;
  remainder = cur_ticks - (start_dma_hrtimer_count * DMA_PPS_COUNT);
  sprintf(print_buffer,"compute_timing_adjust_ns, adjustment_ns=%f, start_dma_count=%u, remainder=%u\r\n", 
    adjustment_ns, start_dma_hrtimer_count, remainder);
  Serial.println(print_buffer);
  if ( remainder > 0xffdf ) {
    remainder -= DMA_PPS_COUNT;
    start_dma_hrtimer_count++;
  }
  // remainder is new fine value, Y
  // start_dma_hrtimer_count is new DMA value, X
  sprintf(print_buffer,"Adjust timing, final typical value = %u, fine value = %u\r\n", 
    start_dma_hrtimer_count, remainder);
  Serial.print(print_buffer);
  *Y = remainder;
  *X = start_dma_hrtimer_count;
  return;
} 

void compute_level_ticks(uint32_t art_ticks, uint16_t * dma_count, uint16_t * fine_count)
{
  uint32_t start_dma_hrtimer_count = 0;
  start_dma_hrtimer_count = (uint32_t) (art_ticks - 0xffdf) / DMA_PPS_COUNT ;
  uint32_t remainder = 0;
  remainder = art_ticks - (start_dma_hrtimer_count * DMA_PPS_COUNT);
  sprintf(print_buffer,"compute_level_ticks, art_ticks=%"PRIu32 ", start_dma_count=%"PRIu32", remainder=%"PRIu32"\r\n", 
    art_ticks, start_dma_hrtimer_count, remainder);
  Serial.println(print_buffer);
  if ( remainder > 0xffdf ) {
    remainder -= DMA_PPS_COUNT;
    start_dma_hrtimer_count++;
  }
  // remainder is new fine value, Y
  // start_dma_hrtimer_count is new DMA value, X
  sprintf(print_buffer,"compute_level_ticks, final typical (DMA) value = %"PRIu32", fine value = %"PRIu32"\r\n", 
    start_dma_hrtimer_count, remainder);
  Serial.print(print_buffer);
  *fine_count = (uint16_t)remainder;
  *dma_count = (uint16_t)start_dma_hrtimer_count;
  return;
}

void set_next_level_duration(uint64_t art_ticks, bool high_level)
{
  sprintf(print_buffer, "Set next level duration, art_ticks=%"PRIu64 ", highlevel=%d\r\n",
    art_ticks, high_level);
  Serial.print(print_buffer);
  if ( high_level ) {
    compute_level_ticks( (uint32_t)art_ticks, &next_highlevel_typical_count, &next_highlevel_fine_count);
  } else {
    compute_level_ticks( (uint32_t)art_ticks, &next_lowlevel_typical_count, &next_lowlevel_fine_count);
  }
}

void get_current_art_time(uint64_t * art_ticks_now)
{  
	*art_ticks_now = (uint64_t) HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CNTxR; // current value of HRTIMer
  *art_ticks_now += (DMA_PPS_COUNT - __HAL_DMA_GET_COUNTER(&hdma_hrtim) ) * HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR; // DMA ticks
  *art_ticks_now += art_ticks; // ticks so far
}
void get_art_time_change(uint64_t * new_art_ticks)
{
  static uint64_t last_art_time = 0;
  static uint64_t this_art_time = 0;
  if ( last_art_time == 0 ) {
    get_current_art_time(&this_art_time);
    *new_art_ticks = this_art_time;
    last_art_time = this_art_time;
  } else {
    get_current_art_time(&this_art_time);
    *new_art_ticks = this_art_time - last_art_time;
    last_art_time = this_art_time;
  }
}


extern "C" {

  void add_to_art(uint64_t ticks){
    art_ticks += ticks;
  }
  // top level DMA IRQ handler for HRTIMER 
  void HRTIMER_DMA_STREAM_HANDLER(void) { 
    HAL_DMA_IRQHandler(&hdma_hrtim);
  }
  void __attribute__((weak)) hrtimer_DMAHalfReceiveCplt(DMA_HandleTypeDef *hdma) {
  }
    void __attribute__((weak)) hrtimer_DMAError(DMA_HandleTypeDef *hdma) {    
  }  
  void __attribute__((weak)) hrtimer_DMAReceiveCplt(DMA_HandleTypeDef *hdma) {
    if (hdma == &hdma_hrtim)
    {
      dma_complete_counter++;
      uint16_t cur_cmp = HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR;
      // enable set every other time on next compare 2
      // enable TIMER C interrupt for this next time so I can disable rst / set outputs
      // disable DMA request for this next time to prevent off by one cycle error
      HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxDIER = HRTIM_TIM_FLAG_CMP2 | HRTIM_TIM_FLAG_CPT1;  
      if ( set_next_output ) {
        // rising edge basically on next COMPARE 2 trigger
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].RSTx2R = HRTIM_OUTPUTRESET_NONE;
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].SETx2R = HRTIM_OUTPUTSET_TIMCMP2;
        // change the compare2 value to get the 2ns resolution
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR = next_highlevel_fine_count;
      } else {
        // falling edge basically on next COMPARE 2 trigger
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].RSTx2R = HRTIM_OUTPUTRESET_TIMCMP2;
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].SETx2R = HRTIM_OUTPUTSET_NONE;
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR = next_lowlevel_fine_count;
      }
      add_to_art( ((uint64_t)cur_cmp) * DMA_PPS_COUNT); // dma saw this many ticks of hrtimer
    }
  }

  // compare 2 used for pps output
  // this is only used for fine step 
  void HAL_HRTIM_Compare2EventCallback(HRTIM_HandleTypeDef * hhrtim, uint32_t TimerIdx)
  {
    // Check if the compare value matched and the output toggle occurred
    timc_interrupt_counter++;
    uint16_t cur_cmp = HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR;

    // disable timc interrupt, enable DMA
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxDIER = HRTIM_TIM_DMA_CMP2 | HRTIM_TIM_FLAG_CPT1;

    // set_next_output = 1 here, means just generated rising edge and output is now high
    // set_next_output = 0 here, means just generated falling edge and output is now low
    output_level = set_next_output; 
    set_next_output = !set_next_output; // toggle this

    // set_next_output = 1 here, means just generated falling edge and output is now low
    // set_next_output = 0 here, means just generated rising edge and output is now high
    if ( set_next_output ) { // setting up next level 
      HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR = next_lowlevel_typical_count;
	    falling_edge_counter += 1;
    } else {
      HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR = next_highlevel_typical_count;
	    rising_edge_counter += 1;	  
    }
    add_to_art( (uint64_t)cur_cmp ); // just one cycle of hrtimer, PHC saw this many ticks
    art_time_last_rising = art_ticks;
  }
  
  // capture 1 used for PPS input
  void HAL_HRTIM_Capture1EventCallback(HRTIM_HandleTypeDef * hhrtim, uint32_t TimerIdx)
  {
    // PPS input capture
    // grab things most likely to change faster
    dma_remaining_ts = __HAL_DMA_GET_COUNTER(&hdma_hrtim);
    capture_ts = HAL_HRTIM_GetCapturedValue(hhrtim, HRTIM_TIMERINDEX_TIMER_C, HRTIM_CAPTUREUNIT_1);
    art_val_ts = art_ticks;
    // if you need typical_count , then the current compare register is the correct value
    typical_count_ts = HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR;
    millis_ts = millis();
    valid_ts = 1;
  }

  // Interrupt handler for HRTIMER TIMC
  void HRTIM1_TIMC_IRQHandler(void)
  {
    //timc_interrupt_counter++;
    HAL_HRTIM_IRQHandler(&hhrtim, HRTIM_TIMERINDEX_TIMER_C);    
  }  
}

bool get_out_level()
{
  return output_level;
}
bool has_timestamp()
{
  return valid_ts;
}
void clear_has_timestamp()
{
  valid_ts = 0;
}

uint64_t get_pps_in_timestamp_art()
{
  uint64_t temp = 0;
  temp = art_val_ts; // how many ART ticks so far
  temp += capture_ts; // this capture point
  if ( dma_remaining_ts == DMA_PPS_COUNT) {
    // either right before fine edge or right after fine edge, don't need anything
  } else {
    // in the middle of typical / coarse timeframe
    temp += (DMA_PPS_COUNT - dma_remaining_ts) * typical_count_ts;
  }
  sprintf(print_buffer,"Get pps in timestamp art: %"PRIu64"\r\n", temp);
  Serial.print(print_buffer);
  return temp;
}




















/******************************* INIT CODE BELOW, don't mess with it if you dont have to ****************/


// GPIO initialization for PA10 as HRTIM_CHC2
void GPIO_Init(void)
{
  // enable HRTIMER CH C outputs and EEV inputs
  // WWVB LoRA V2 board, uses 
  // CHC2 (PA10) -> UFL P4
  // CHC1 (PA9) -> RCB header output
  // EEV7 (PB5) -> Ice40 connection
  // CHB1 (PC8) -> Ice40 connection
  // EEV1 (PC10) -> RCB header input
  // EEV2 (PC12) -> From PLL, 10MHz
  // EEV3 (PD5) -> From PLL, not determined frequency
  // CHE2 (PG7) -> Ice40 connection
  // for now, only implemented CHC2 because its easy to measure

  // CHC2 , PA10 , UFL P4
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF2_HRTIM1; // Alternate function for HRTIM_CHC2
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  Serial.println("Enabling PA10 for HRTIM1 CHC2 output");

  // EEV1, RCB Header input, PC10, put pulldown for now
  // pin is pin 12 on WWVB RCB Board
  // hack code for input only
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  // GPIO test mode
  //GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  //GPIO_InitStruct.Pull = GPIO_NOPULL;
  
  // HRTIMER EEV1 pin mode
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Alternate = GPIO_AF2_HRTIM1; // Alternate function for HRTIM_EEV1
  
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


  // CHC1 , PA9, using as a debug random toggle
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_9);

}

// Initialize HRTIMER
void HRTIM_Init(void)
{
    __HAL_RCC_HRTIM1_CLK_ENABLE();

    hhrtim.Instance = HRTIM1;
    hhrtim.Init.HRTIMInterruptResquests = HRTIM_IT_NONE;
    hhrtim.Init.SyncOptions = HRTIM_SYNCOPTION_NONE;
    hhrtim.Init.SyncInputSource = HRTIM_SYNCINPUTSOURCE_NONE;
    hhrtim.Init.SyncOutputSource = HRTIM_SYNCOUTPUTSOURCE_MASTER_START;
    hhrtim.Init.SyncOutputPolarity = HRTIM_SYNCOUTPUTPOLARITY_NONE;
    HAL_HRTIM_Init(&hhrtim);
    Serial.println("Enabling HRTIM subsystem");
}

void HRTIM_EEV1_PPSInput_Config(void)
{

  // input capture settings for PPS input, for now just EEV1 on PC10
  //HAL_HRTIM_SimpleCaptureChannelConfig
  //    -> HRTIM_EventConfig
  //    -> HRTIM_CaptureUnitConfig

  // HRTIM_EventConfig code
  HRTIM_EventCfgTypeDef EventCfg;
  EventCfg.FastMode = HRTIM_EVENTFASTMODE_DISABLE;
  EventCfg.Filter = (HRTIM_EVENTFILTER_NONE);
  EventCfg.Polarity = (HRTIM_EVENTPOLARITY_HIGH);
  EventCfg.Sensitivity = ( HRTIM_EVENTSENSITIVITY_RISINGEDGE);
  EventCfg.Source = HRTIM_EVENTSRC_1;

  uint32_t hrtim_eecr1;
  uint32_t hrtim_eecr2;
  uint32_t hrtim_eecr3;

  /* Configure external event channel */
  hrtim_eecr1 = hhrtim.Instance->sCommonRegs.EECR1;
  hrtim_eecr2 = hhrtim.Instance->sCommonRegs.EECR2;
  hrtim_eecr3 = hhrtim.Instance->sCommonRegs.EECR3;

  hrtim_eecr1 &= ~(HRTIM_EECR1_EE1SRC | HRTIM_EECR1_EE1POL | HRTIM_EECR1_EE1SNS | HRTIM_EECR1_EE1FAST);
  hrtim_eecr1 |= (EventCfg.Source & HRTIM_EECR1_EE1SRC);
  hrtim_eecr1 |= (EventCfg.Polarity & HRTIM_EECR1_EE1POL);
  hrtim_eecr1 |= (EventCfg.Sensitivity & HRTIM_EECR1_EE1SNS);
  /* Update the HRTIM registers (all bitfields but EE1FAST bit) */
  hhrtim.Instance->sCommonRegs.EECR1 = hrtim_eecr1;
  /* Update the HRTIM registers (EE1FAST bit) */
  hrtim_eecr1 |= (EventCfg.FastMode  & HRTIM_EECR1_EE1FAST);
  hhrtim.Instance->sCommonRegs.EECR1 = hrtim_eecr1;

  // HRTIM_CaptureUnitConfig code for capture unit 1
  uint32_t CaptureTrigger = 0xFFFFFFFFU;
  CaptureTrigger = HRTIM_CAPTURETRIGGER_EEV_1 ;
  hhrtim.TimerParam[HRTIM_TIMERINDEX_TIMER_C].CaptureTrigger1 = CaptureTrigger;
}

// Configure HRTIMER CH C for toggling output on rollover
void HRTIM_ChannelC_Config(void)
{
  HRTIM_TimeBaseCfgTypeDef pTimeBaseCfg = {0};
  pTimeBaseCfg.Period = 0xffdf; 
  pTimeBaseCfg.RepetitionCounter = 0;
  pTimeBaseCfg.PrescalerRatio = HRTIM_PRESCALERRATIO_DIV1;
  pTimeBaseCfg.Mode = HRTIM_MODE_CONTINUOUS;
  HAL_HRTIM_TimeBaseConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_C, &pTimeBaseCfg);

  HRTIM_CompareCfgTypeDef pCompareCfg = {0};
  pCompareCfg.CompareValue = 1000;
  HAL_HRTIM_WaveformCompareConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_C, HRTIM_COMPAREUNIT_1, &pCompareCfg);

  pCompareCfg.CompareValue = 1000;
  HAL_HRTIM_WaveformCompareConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_C, HRTIM_COMPAREUNIT_2, &pCompareCfg);

  HRTIM_SimpleOCChannelCfgTypeDef ocCfg = {0};
  ocCfg.Mode = HRTIM_BASICOCMODE_TOGGLE;
  ocCfg.Pulse = 0xffdf; 
  ocCfg.Polarity = HRTIM_OUTPUTPOLARITY_HIGH;
  ocCfg.IdleLevel = HRTIM_OUTPUTIDLELEVEL_INACTIVE;
  HAL_HRTIM_SimpleOCChannelConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_C,
    HRTIM_OUTPUT_TC2, &ocCfg);

  HRTIM_EEV1_PPSInput_Config();

  // enable capture unit 1 for 1PPS in , API is HAL_HRTIM_SimpleCaptureStart
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CPT1xCR = hhrtim.TimerParam[HRTIM_TIMERINDEX_TIMER_C].CaptureTrigger1;


  // output start, this starts the timer!
  HAL_HRTIM_SimpleOCStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_C, HRTIM_OUTPUT_TC2);

  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].RSTxR |= (1<<2); // make counter reset based on compare unit 2 
  // this makes the output respect the ocCfg, otherwise it doesn't

  // main configs to make PPS work
  // Compare unit 2 -> Setup by SimpleOCStart more or less

  // Output set triggers (make output high) -> Set to nothing for now
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].SETx2R = 0x0;
  // Output reset triggers (make output low) -> Set to nothing for now
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].RSTx2R = 0x0;
  // Interrupt / DMA enable flags -> Set to nothing for now
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxDIER = 0x0;




  debug_print_hrtimer_registers(HRTIM_TIMERINDEX_TIMER_C);
  debug_print_hrtimer_common_registers();
}

void DMA_Init(void)
{
    hdma_hrtim.Instance = HRTIMER_DMA_STREAM; // Example DMA stream
    hdma_hrtim.Init.Request = DMA_REQUEST_HRTIM_TIMER_C;
    hdma_hrtim.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_hrtim.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_hrtim.Init.MemInc = DMA_MINC_DISABLE;
    hdma_hrtim.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_hrtim.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_hrtim.Init.Mode = DMA_CIRCULAR; // is normal or continuous easier???
    hdma_hrtim.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_hrtim.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    hdma_hrtim.XferCpltCallback = hrtimer_DMAReceiveCplt;
    hdma_hrtim.XferHalfCpltCallback = hrtimer_DMAHalfReceiveCplt;
    HAL_DMA_Init(&hdma_hrtim);

    __HAL_LINKDMA(&hhrtim, hdmaTimerC, hdma_hrtim);
    Serial.println("DMA init for HRTIMER C done!");
}

void Start_PPS_DMA(void) 
{
  // DMA is setup (not running) 
  // HRTIMER Channel C is setup and running with max period and no output set or reset config

  // NVIC initialization for HRTIMER and DMA interrupts
  HAL_NVIC_SetPriority(HRTIMER_DMA_STREAM_IRQ, 9, 0);
  HAL_NVIC_EnableIRQ(HRTIMER_DMA_STREAM_IRQ);

  // enable interrupt on hrtimer , but the hrtimer interrupt enable is not enabled yet
  HAL_NVIC_SetPriority(HRTIM1_TIMC_IRQn, 9, 0);
  HAL_NVIC_EnableIRQ(HRTIM1_TIMC_IRQn);

  // enable DMA request from hrtimer for compare unit 2
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxDIER = HRTIM_TIM_DMA_CMP2 | HRTIM_TIM_FLAG_CPT1;

  // start the DMA
  HAL_DMA_Start_IT(&hdma_hrtim,(uint32_t)&hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].SETx2R,
    (uint32_t)&sram3_data->dummy_hrtimer_dma_val,DMA_PPS_COUNT); 



  Serial.println("Start PPS DMA done!");

}

void stm32_hrtimer_init()
{
  art_val_ts = 0;
  capture_ts = 0;
  dma_remaining_ts = 0;
  typical_count_ts = 0;
  millis_ts = 0;
  valid_ts = 0;
  // arduino set hrtim1 source initially as RCC_HRTIM1CLK_TIMCLK, need to change it

  __HAL_RCC_HRTIM1_CLK_DISABLE();
  __HAL_RCC_C1_HRTIM1_CLK_DISABLE();
  __HAL_RCC_C2_HRTIM1_CLK_DISABLE();

  __HAL_RCC_HRTIM1_CONFIG(RCC_HRTIM1CLK_CPUCLK);

  uint32_t temp = 0;
  temp = __HAL_RCC_GET_HRTIM1_SOURCE();
  sprintf(print_buffer,"HRTIMER Source = 0x%lx\r\n", temp);
  Serial.print(print_buffer);

  __HAL_RCC_HRTIM1_CLK_ENABLE();
  __HAL_RCC_C1_HRTIM1_CLK_ENABLE();
  __HAL_RCC_C2_HRTIM1_CLK_ENABLE();

  GPIO_Init(); // Initialize GPIO PA10 as HRTIM_CHC2

  HRTIM_Init(); // Initialize HRTIMER
  HRTIM_ChannelC_Config(); // Configure HRTIMER C for output compare mode
  DMA_Init(); // Initialize DMA
  Start_PPS_DMA();

}


