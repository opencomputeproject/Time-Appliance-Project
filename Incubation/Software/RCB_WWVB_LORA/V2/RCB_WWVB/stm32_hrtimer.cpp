#include "stm32_hrtimer.h"


void debug_print_hrtimer_common_registers() 
{
  sprintf(print_buffer,"HRTimer Common registers:\r\n");
  sprintf(print_buffer,"%s CR1=0x%x\r\n", print_buffer, HRTIM1->sCommonRegs.CR1);
  sprintf(print_buffer,"%s CR2=0x%x\r\n", print_buffer, HRTIM1->sCommonRegs.CR2);
  sprintf(print_buffer,"%s OENR=0x%x\r\n", print_buffer, HRTIM1->sCommonRegs.OENR);
  sprintf(print_buffer,"%s ODISR=0x%x\r\n", print_buffer, HRTIM1->sCommonRegs.ODISR);
  sprintf(print_buffer,"%s EECR1=0x%x\r\n", print_buffer, HRTIM1->sCommonRegs.EECR1);
  sprintf(print_buffer,"%s EECR2=0x%x\r\n", print_buffer, HRTIM1->sCommonRegs.EECR2);
  sprintf(print_buffer,"%s EECR3=0x%x\r\n", print_buffer, HRTIM1->sCommonRegs.EECR3);
  Serial.print(print_buffer);  
  Serial.flush();
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
  DMA_HandleTypeDef hdma_hrtim_ppsout;
  DMA_HandleTypeDef hdma_hrtim_ppsin;
  HRTIM_HandleTypeDef hhrtim;  
}

void HRTIM_EEV1_PPSInput_Config(void)
{

  // input capture settings for PPS input, for now just EEV1 on PC10
  //HAL_HRTIM_SimpleCaptureChannelConfig
  //    -> HRTIM_EventConfig
  //    -> HRTIM_CaptureUnitConfig

  // HRTIM_EventConfig code
  HRTIM_EventCfgTypeDef EventCfg;
  EventCfg.FastMode = HRTIM_EVENTFASTMODE_DISABLE; // because using toggle output, can't use fast mode, introduces 5-6 cycle latency (10.4 - 12.5 nanoseconds)
  EventCfg.Filter = (HRTIM_EVENTFILTER_NONE);
  EventCfg.Polarity = (HRTIM_EVENTPOLARITY_HIGH);
  EventCfg.Sensitivity = ( HRTIM_EVENTSENSITIVITY_RISINGEDGE | HRTIM_EVENTSENSITIVITY_FALLINGEDGE);
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
  HAL_HRTIM_TimeBaseConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_B, &pTimeBaseCfg);



  
  /*
  HRTIM_CompareCfgTypeDef pCompareCfg = {0};
  pCompareCfg.CompareValue = 1000;
  HAL_HRTIM_WaveformCompareConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_C, HRTIM_COMPAREUNIT_1, &pCompareCfg);

  pCompareCfg.CompareValue = 1000;
  HAL_HRTIM_WaveformCompareConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_C, HRTIM_COMPAREUNIT_2, &pCompareCfg);
  */

  HRTIM_SimpleOCChannelCfgTypeDef ocCfg = {0};
  ocCfg.Mode = HRTIM_BASICOCMODE_TOGGLE;
  ocCfg.Pulse = 0xffdf; 
  ocCfg.Polarity = HRTIM_OUTPUTPOLARITY_HIGH;
  ocCfg.IdleLevel = HRTIM_OUTPUTIDLELEVEL_INACTIVE;
  HAL_HRTIM_SimpleOCChannelConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_C,
    HRTIM_OUTPUT_TC2, &ocCfg);
  HAL_HRTIM_SimpleOCChannelConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_C,
    HRTIM_OUTPUT_TC1, &ocCfg);

  HAL_HRTIM_SimpleOCChannelConfig(&hhrtim, HRTIM_TIMERINDEX_TIMER_B,
    HRTIM_OUTPUT_TC1, &ocCfg);

  





  HRTIM_EEV1_PPSInput_Config();

  HAL_HRTIM_SimpleBaseStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_C); // start Timer C as well!
  HAL_HRTIM_SimpleBaseStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_B); // start Timer B as well!

  
  // output start, this starts the timer!
  //HAL_HRTIM_SimpleOCStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_C, HRTIM_OUTPUT_TC2);
  //HAL_HRTIM_SimpleOCStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_C, HRTIM_OUTPUT_TC1);
  //HAL_HRTIM_SimpleOCStart(&hhrtim, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TC1);
  
  
  // need to check input pin status and possibly invert these
  // basically configure it to toggle every edge of input
  // but to get polarity correct, need to check what input level is when starting
  // Default behavior, EEV1 (RCB PPS Input) -> CHC2 / CHC1 / CHB1 
  // simple routing through HRTimer
  // Step 1. Configure those outputs to set on EEV1 set
  while ( wwvb_digital_read(RCB_PPS_IN) )
  {
    Serial.println("PPS Input was high when initializing hrtimer pps output!");
    delay(100);
  }
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].SETx2R = HRTIM_OUTPUTSET_EEV_1;
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].SETx1R = HRTIM_OUTPUTSET_EEV_1;
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].SETx1R = HRTIM_OUTPUTSET_EEV_1;
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].RSTx2R = HRTIM_OUTPUTRESET_EEV_1; 
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_C].RSTx1R = HRTIM_OUTPUTRESET_EEV_1; 
  hhrtim.Instance->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].RSTx1R = HRTIM_OUTPUTRESET_EEV_1;



  // enable output
  HRTIM1->sCommonRegs.OENR = HRTIM_OUTPUT_TC1 | HRTIM_OUTPUT_TC2 | HRTIM_OUTPUT_TB1;
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
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Alternate = GPIO_AF2_HRTIM1; // Alternate function for HRTIM_EEV1  
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


  // CHC1 , PA9, RCB header PPS out, same PPS out as CHC2
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF2_HRTIM1;
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



void stm32_hrtimer_init()
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

  HRTIM_Init(); // Initialize HRTIMER

  GPIO_Init(); // Initialize GPIO PA10 as HRTIM_CHC2

  HRTIM_ChannelC_Config(); // Configure HRTIMER C for output compare mode


  Serial.println("******HRTIM Timer C registers********");
  debug_print_hrtimer_registers(HRTIM_TIMERINDEX_TIMER_C);

  Serial.println("******HRTIM Timer A registers********");
  debug_print_hrtimer_registers(HRTIM_TIMERINDEX_TIMER_A);

  Serial.println("*******HRTIM common registers*******");
  debug_print_hrtimer_common_registers();

}


