
#include "stm32_pps.h"





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





extern "C" {
  DMA_HandleTypeDef hdma_hrtim;
  HRTIM_HandleTypeDef hhrtim;
}


extern "C" {
  uint32_t X = 3000; // Number of times HRTIMER overflows before triggering DMA
  uint64_t dma_complete_counter = 0;
  uint64_t timc_interrupt_counter = 0;
  bool set_next_output = 0;
  void Start_DMA_Transfer(void);

  
  // Utility function, Update HRTIM compare value
  void Update_HRTIM_CompareValue(uint32_t newCompareValue)
  {
    HRTIM_CompareCfgTypeDef pCompareCfg = {0};
    pCompareCfg.CompareValue = newCompareValue;
    HAL_HRTIM_WaveformCompareConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_C, HRTIM_COMPAREUNIT_1, &pCompareCfg);
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
      // keep DMA running, may get off by one on this
      if ( set_next_output ) {
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxDIER = HRTIM_TIM_FLAG_SET2 | HRTIM_TIM_DMA_CMP2;
      } else {
        HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxDIER = HRTIM_TIM_FLAG_RST2 | HRTIM_TIM_DMA_CMP2;
      }     

      set_next_output = !set_next_output; // toggle this

      // change the compare2 value to get the 2ns resolution, hard coding for now
      HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR = 62511;
    }
  }
  void __attribute__((weak)) hrtimer_DMAError(DMA_HandleTypeDef *hdma) {    
  }


  // Interrupt handler for HRTIMER TIMC
  void HRTIM1_TIMC_IRQHandler(void)
  {
    // Check if the compare value matched and the output toggle occurred
    if (__HAL_HRTIM_TIMER_GET_FLAG(&hhrtim, HRTIM_TIMERINDEX_TIMER_C, HRTIM_TIM_FLAG_CMP2))
    {
      timc_interrupt_counter++;

      // clear set / reset on output
      //HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].RSTx2R = HRTIM_OUTPUTRESET_NONE;
      //HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].SETx2R = HRTIM_OUTPUTSET_NONE;

      // disable timc interrupt
      HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxDIER = HRTIM_TIM_DMA_CMP2;

      // make sure compare value is back to max
      HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].CMP2xR = 0xffdf;
    }
    HAL_HRTIM_IRQHandler(&hhrtim, HRTIM_TIMERINDEX_TIMER_C);
  }  
}



// GPIO initialization for PA10 as HRTIM_CHC2
void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_HRTIM1; // Alternate function for HRTIM_CHC2
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    Serial.println("Enabling PA10 for HRTIM1 CHC2 output");
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

// Configure HRTIMER CH C for toggling output on rollover

uint16_t cur_period = 1000;
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

  // start the DMA
  HAL_DMA_Start_IT(&hdma_hrtim,(uint32_t)&hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].SETx2R,
    (uint32_t)&sram3_data->dummy_hrtimer_dma_val,3663); 

  // enable DMA request from hrtimer for compare unit 2
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].TIMxDIER = HRTIM_TIM_DMA_CMP2;

  Serial.println("Start PPS DMA done!");

}








uint64_t last_millis = 0;

void init_stm_pps()
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
  //NVIC_Init(); // Initialize NVIC for interrupts

  //Start_DMA_Transfer(); // Start the DMA transfer

  last_millis = millis();

}

uint64_t last_dma_counter = 0;
uint64_t last_timc_counter = 0;


void loop_stm_pps()
{
  if ( last_millis == 0 ) {
    return;
  }
  if ( millis() - last_millis > 2000 ) {
    last_millis = millis();
    // toggle the frequency back and forth
    if ( cur_period == 1000 ) {
      cur_period = 2000;
      
    } else {
      cur_period = 1000;
    }
    //sprintf(print_buffer, "Changing HRTIMER C index to %d\r\n", cur_period);
    //Serial.print(print_buffer);
    //hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].PERxR = cur_period;

  }
  if ( last_dma_counter != dma_complete_counter) {
    last_dma_counter = dma_complete_counter;
    sprintf(print_buffer, "Loop stm pps dma counter increment to %d\r\n", dma_complete_counter);
    Serial.print(print_buffer);
  }
  if ( last_timc_counter != timc_interrupt_counter  )
  {
    last_timc_counter = timc_interrupt_counter;
    sprintf(print_buffer, "Loop stm pps timc interrupt counter increment to %d\r\n", timc_interrupt_counter);
    Serial.print(print_buffer);
  }

}