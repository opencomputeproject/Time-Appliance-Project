
#include "stm32_pps.h"



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




// STM HAL API variables
extern "C" {
  DMA_HandleTypeDef hdma_hrtim;
  HRTIM_HandleTypeDef hhrtim;
}

// PPS Input variables
// just defining a set of global variables
// assume PPS input is slow enough, don't need any fifo stuff
extern "C" {
  uint64_t phc_sec_ts = 0;
  float phc_ns_ts = 0;
  uint16_t capture_ts = 0;
  uint16_t dma_remaining_ts = 0;
  uint16_t typical_count_ts = 0;
  uint64_t millis_ts = 0;
  bool valid_ts = 0;
}


// PPS output variables
extern "C" {
  // top level PHC basically, seconds and nanoseconds
  uint64_t phc_seconds = 0; // TOD basically
  float phc_nanoseconds = 0; // nanoseconds
  const float nano_per_tick = ((1.0/480.0e6)*1e9);

  uint64_t dma_complete_counter = 0;
  uint64_t timc_interrupt_counter = 0;

  bool set_next_output = 1; // has to start out as 1! output starts at reset
  // if you don't do this , it will mess up
}

extern "C" {


  // X is DMA roll over counter, Y is the single fine roll over
  // utility calculation function
  void adjust_timing(float adjustment_ns, uint16_t *X, uint16_t *Y) {
    // Constants
    const float clock_frequency_hz = 480000000.0; // 480 MHz
    const float clock_period_ns = 1e9 / clock_frequency_hz; // Clock period in nanoseconds

    if ( adjustment_ns > 400e6 ) {
      adjustment_ns = 400e6;
    } else if ( adjustment_ns < -400e6 ) {
      adjustment_ns -= 400e6;
    }
    // compute how many counter ticks to adjust by
    int64_t tick_adjust = (int64_t) (adjustment_ns / clock_period_ns);

    int64_t cur_ticks = (*X * DMA_PPS_COUNT) + *Y;

    cur_ticks += tick_adjust;

    uint32_t start_dma_hrtimer_count = 0;
    start_dma_hrtimer_count = (uint32_t) (cur_ticks - 0xffdf) / DMA_PPS_COUNT ;
    uint32_t remainder = 0;
    remainder = cur_ticks - (start_dma_hrtimer_count * DMA_PPS_COUNT);
    sprintf(print_buffer,"adjust_timing, start_dma_count=%u, remainder=%u\r\n", start_dma_hrtimer_count, remainder);
    Serial.println(print_buffer);

    if ( remainder > 0xffdf ) {
      remainder -= DMA_PPS_COUNT;
      start_dma_hrtimer_count++;
    }
    // remainder is new fine value, Y
    // start_dma_hrtimer_count is new DMA value, X
    *Y = remainder;
    *X = start_dma_hrtimer_count;
    return;
  } 

  // PPS is generated with a two step process
  // 1. First step, uses DMA for the bulk of the time
  //      DMA counts DMA_PPS_COUNT roll overs of HRTIMER
  //      HRTIMER is roll over point is set by typical_count
  // 2. Second step, on DMA completing DMA_PPS_COUNT roll overs
  //      next hrtimer is set to toggle the output and DMA is disabled
  //      this single rollover point is set by fine_count
  //      when that toggle occurs, rollover point set back to typical_count and dma enabled again
  // fine_count should be as large as possible to prevent race conditions
  uint16_t typical_count = 34277;
  uint16_t fine_count = 61000;
  uint16_t next_typical_count = 34277;
  uint16_t next_fine_count = 61000;

  // In order to generate a step in the PPS output
  // need to do a one time adjustment of typical_count and fine_count
  // make sure to keep fine_count as large as possible
  // keep the original, adjust it once, after next edge set them back
  bool doing_typical_step = 0;
  bool doing_fine_step = 0;
  uint16_t orig_typical_count = 34277;
  uint16_t orig_fine_count = 61000;

  // utility function to keep PHC aligned with running operation
  float nanoseconds_temp = 0;
  uint64_t seconds_temp = 0;
  void add_to_phc(uint64_t hrtimer_ticks) 
  {
    // compute this in nanoseconds
    nanoseconds_temp = (float) ( ((float)hrtimer_ticks) * nano_per_tick);

    // correct if overflowed nanosecond value
    while ( nanoseconds_temp > 1e9 ) {
      nanoseconds_temp -= 1e9;
      phc_seconds += 1;
    }

    phc_nanoseconds += nanoseconds_temp;

    if ( phc_nanoseconds > 1.0e9 ) {
      phc_nanoseconds -= 1.0e9;
      phc_seconds += 1;
    }
  }

  // top level DMA IRQ handler for HRTIMER 
  void HRTIMER_DMA_STREAM_HANDLER(void) { 
    HAL_DMA_IRQHandler(&hdma_hrtim);
  }
  void __attribute__((weak)) hrtimer_DMAHalfReceiveCplt(DMA_HandleTypeDef *hdma) {
  }
  void __attribute__((weak)) hrtimer_DMAReceiveCplt(DMA_HandleTypeDef *hdma) {
    if (hdma == &hdma_hrtim)
    {
      dma_complete_counter++;
      // enable set every other time on next compare 2
      if ( set_next_output ) {
        // rising edge basically on next COMPARE 2 trigger
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].RSTx2R = HRTIM_OUTPUTRESET_NONE;
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].SETx2R = HRTIM_OUTPUTSET_TIMCMP2;
      } else {
        // falling edge basically on next COMPARE 2 trigger
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].RSTx2R = HRTIM_OUTPUTRESET_TIMCMP2;
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].SETx2R = HRTIM_OUTPUTSET_NONE;
      }

      // enable TIMER C interrupt for this next time so I can disable rst / set outputs
      // disable DMA request for this next time to prevent off by one cycle error
      HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxDIER = HRTIM_TIM_FLAG_CMP2 | HRTIM_TIM_FLAG_CPT1;   

      set_next_output = !set_next_output; // toggle this

      if ( doing_fine_step ) {
        fine_count = next_fine_count;
        doing_fine_step = 0;
      } else {
        fine_count = orig_fine_count; // doing single step, next edge go back to normal
      }

      // change the compare2 value to get the 2ns resolution, hard coding for now
      HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR = fine_count;
      add_to_phc(typical_count * DMA_PPS_COUNT); // dma saw this many ticks of hrtimer
    }
  }
  void __attribute__((weak)) hrtimer_DMAError(DMA_HandleTypeDef *hdma) {    
  }

  
  // compare 2 used for pps output
  // this is only used for fine step 
  void HAL_HRTIM_Compare2EventCallback(HRTIM_HandleTypeDef * hhrtim, uint32_t TimerIdx)
  {
    // Check if the compare value matched and the output toggle occurred
    timc_interrupt_counter++;

    // disable timc interrupt
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxDIER = HRTIM_TIM_DMA_CMP2 | HRTIM_TIM_FLAG_CPT1;

    if ( doing_typical_step ) {
      typical_count = next_typical_count;
      doing_typical_step = 0;
    } else {
      typical_count = orig_typical_count; // doing single step, next edge go back to normal
    }

    // set next compare value   
    HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR = typical_count;
    add_to_phc(fine_count); // just one cycle of hrtimer, PHC saw this many ticks
  }
  
  // capture 1 used for PPS input
  void HAL_HRTIM_Capture1EventCallback(HRTIM_HandleTypeDef * hhrtim, uint32_t TimerIdx)
  {
    // PPS input capture
    // grab things most likely to change faster
    dma_remaining_ts = __HAL_DMA_GET_COUNTER(&hdma_hrtim);
    capture_ts = HAL_HRTIM_GetCapturedValue(hhrtim, HRTIM_TIMERINDEX_TIMER_C, HRTIM_CAPTUREUNIT_1);
    phc_ns_ts = phc_nanoseconds;
    phc_sec_ts = phc_seconds;
    typical_count_ts = typical_count;
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








uint64_t last_millis = 0;

void init_stm_pps()
{


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

  sprintf(print_buffer, "PPS parameters, typical_count = %u, fine_count = %u\r\n",
    typical_count, fine_count);
  Serial.print(print_buffer);

  last_millis = millis();

}



// can only do +/- 400 millisecond adjustment!
// keeps the logic simpler
void pps_step(int64_t step_val_ns) {
  orig_typical_count = typical_count;
  orig_fine_count = fine_count;
  uint16_t temp_typical, temp_fine;
  temp_typical = typical_count;
  temp_fine = fine_count;

  adjust_timing( (float) step_val_ns, &temp_typical, &temp_fine);

  next_typical_count = temp_typical;
  next_fine_count = temp_fine;
  doing_typical_step = 1;
  doing_fine_step = 1;
}

// in order to generate a frequency adjustment in pps output
// need to do an adjustment of typical_count and fine_count
// and not adjust them back
void pps_freq_adjust(int64_t ns_freq_adj) 
{
  uint16_t temp_typical, temp_fine;
  temp_typical = typical_count;
  temp_fine = fine_count;
  adjust_timing( (float) ns_freq_adj, &temp_typical, &temp_fine);

  orig_typical_count = temp_typical;
  orig_fine_count = temp_fine;
}




uint64_t last_dma_counter = 0;
uint64_t last_timc_counter = 0;

bool test_jump_forward = 1;
bool test_speed_up = 1;


uint32_t last_capture_val = 0xa5a5;
bool last_eev_pin_val = 0;


void loop_stm_pps_out()
{
  static uint64_t last_out_millis = 0;
  if ( millis() - last_out_millis > 5000 ) {
    last_out_millis = millis();
    /*
    // test jumping
    if ( test_jump_forward ) {
      pps_step(200*1000*1000);
      Serial.println("PPS Test jump forward!");
    } else {
      pps_step(-200*1000*1000);
      Serial.println("PPS Test jump backward!");
    }
    test_jump_forward = !test_jump_forward;
    */
    // test frequency adjustment
    /*
    if ( test_speed_up ) {
      pps_freq_adjust(100*1000*1000); 
      Serial.println("PPS Test speed up!");
    } else {
      pps_freq_adjust(-100*1000*1000);
      Serial.println("PPS Test slow down!");
    }
    test_speed_up = !test_speed_up;
    */

  }
  if ( last_dma_counter != dma_complete_counter) {
    last_dma_counter = dma_complete_counter;
    sprintf(print_buffer, "Loop stm pps dma counter increment to %d\r\n", 
      dma_complete_counter);
    Serial.print(print_buffer);
  }
  if ( last_timc_counter != timc_interrupt_counter  )
  {
    last_timc_counter = timc_interrupt_counter;
    sprintf(print_buffer, "Loop stm pps timc interrupt counter increment to %d\r\n", timc_interrupt_counter);
    Serial.print(print_buffer);
  }
  
  

}

void loop_stm_pps_in()
{
  static uint64_t last_in_millis = 0;
  if ( millis() - last_in_millis > 5000 ) {
    last_in_millis = millis();
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_9); // debug toggle this pin , pin 6 on RCB header
    Serial.println("Toggling PA9!");
    delay(10);
    //debug_print_hrtimer_registers(HRTIM_TIMERINDEX_TIMER_C);
    //debug_print_hrtimer_common_registers();
  }
  if ( valid_ts ) {
    sprintf(print_buffer, "Got PPS Input! Timestamp info:\r\n");
    sprintf(print_buffer, "%s   PHC_SEC_TS=%" PRIu64 "\r\n", print_buffer, phc_sec_ts);
    sprintf(print_buffer, "%s   PHC_NS_TS=%f\r\n", print_buffer, phc_ns_ts);
    sprintf(print_buffer, "%s   Capture_TS=%" PRIu16 "\r\n", print_buffer, capture_ts);
    sprintf(print_buffer, "%s   DMA_Remaining_TS=%" PRIu16 "\r\n", print_buffer, dma_remaining_ts);
    sprintf(print_buffer, "%s   Typical_Count_TS=%" PRIu16 "\r\n", print_buffer, typical_count_ts);
    sprintf(print_buffer, "%s   Millis_TS=%" PRIu64 "\r\n", print_buffer, millis_ts);
    Serial.println(print_buffer);
    valid_ts = 0;

    // need to post-process this data captured in the interrupt to get a "real" timestamp
    // phc_sec_ts / phc_ns_ts are old, they're only updated by other interrupt handlers after events have happened
    // so that's the starting point
    // capture_ts is the fine value, how many hrtimer ticks since last reset

    uint64_t timestamp_seconds = 0;
    float temp_ns = 0;
    uint64_t timestamp_nanoseconds = 0;
    if ( dma_remaining_ts != DMA_PPS_COUNT )
    {
      // this is most common case, somewhere between toggles
      // math is a bit simpler
      // PHC value + (capture_ts * ns_per_tick) + (DMA_PPS_COUNT - dma_remaining_ts) * typical_count_ts * ns_per_tick
      // PHC value is old looking, but its the time of the last event basically
      // capture_ts is fine resolution, just add it
      // then account for how many DMA transactions have already occurred in this cycle
      timestamp_seconds = phc_sec_ts;
      temp_ns = phc_ns_ts;
      temp_ns += ((float) capture_ts) * nano_per_tick;
      temp_ns += ((float) (DMA_PPS_COUNT - dma_remaining_ts) ) * ((float)typical_count_ts) * nano_per_tick;
      while ( temp_ns >= 1.0e9 ) {
        temp_ns -= 1.0e9;
        timestamp_seconds += 1;
      }
      timestamp_nanoseconds = (uint64_t) temp_ns;

      
    } else 
    {
      Serial.println("*******SPECIAL TIMESTAMP CASE, NEED TO IMPLEMENT*******");
      // this is a special case, this has two sub cases
      // 1. This is right after a toggle , need to check what interrupt flags are set
      // 2. This is right before a toggle, also need to check interrupt flags set
    }
    sprintf(print_buffer,
      "High level timestamp: %" PRIu64 " seconds, %" PRIu64 " nanoseconds\r\n", timestamp_seconds, timestamp_nanoseconds);
    Serial.print(print_buffer);
  }
  
}

void loop_stm_pps()
{
  if ( last_millis == 0 ) {
    return;
  }
  loop_stm_pps_out();
  loop_stm_pps_in();


}