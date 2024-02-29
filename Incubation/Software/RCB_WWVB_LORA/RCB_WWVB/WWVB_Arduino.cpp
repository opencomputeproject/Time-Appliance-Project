#include "WWVB_Arduino.h"



sram1_data_struct * sram1_data = (sram1_data_struct *)0x30000000;

volatile uint8_t spi1_rx_data = 0;
volatile uint8_t spi2_rx_data = 0;
uint8_t SDR_lookup_table[256];





WWVB_Pin WWVB_Pins[] = {
	PIN_STRUCT(WLED_RED),
	PIN_STRUCT(WLED_GREEN),
	PIN_STRUCT(WLED_BLUE),
  
	PIN_STRUCT(PLL_SDA),
	PIN_STRUCT(PLL_SCL),
	PIN_STRUCT(PLL_RST),

  PIN_STRUCT(SIT5501_SDA),
  PIN_STRUCT(SIT5501_SCL),

  PIN_STRUCT(SX1276_MISO),
  PIN_STRUCT(SX1276_MOSI),
  PIN_STRUCT(SX1276_SCK),
  PIN_STRUCT(SX1276_NSS),
  PIN_STRUCT(SX1276_RST),
  PIN_STRUCT(SX1276_DIO0),
  PIN_STRUCT(SX1276_DIO1),
  PIN_STRUCT(SX1276_DIO2),
  PIN_STRUCT(SX1276_DIO3),
  PIN_STRUCT(SX1276_DIO4),
  PIN_STRUCT(SX1276_DIO5),

  // SX1257 management SPI
  PIN_STRUCT(SX1257_MISO),
  PIN_STRUCT(SX1257_MOSI),
  PIN_STRUCT(SX1257_SCK),
  PIN_STRUCT(SX1257_NSS),
  PIN_STRUCT(SX1257_RST),
  //SX1257 data path SPI 1 & 2
  PIN_STRUCT(SX1257_CLK_OUT_SPI1),
  PIN_STRUCT(SX1257_CLK_OUT_SPI2),
  PIN_STRUCT(SX1257_I_IN),
  PIN_STRUCT(SX1257_Q_IN),
  PIN_STRUCT(SX1257_I_OUT),
  PIN_STRUCT(SX1257_Q_OUT),

  PIN_STRUCT(WWVB_AMP1_CS),
  PIN_STRUCT(WWVB_AMP2_CS),

  PIN_STRUCT(WWVB_SMA_UFL_SEL),
  PIN_STRUCT(WWVB_AMPLIFIER_SEL0),
  PIN_STRUCT(WWVB_AMPLIFIER_SEL1),

  PIN_STRUCT(LORA_SMA_UFL_SEL),
  PIN_STRUCT(SDR_TX_RX_SEL),
  PIN_STRUCT(LORA_LF_TXRX_SEL),
  PIN_STRUCT(LORA_HF_TXRX_SEL),
  PIN_STRUCT(LORA_LF_HF_SEL),
};




static GPIO_InitTypeDef wwvb_gpio_init;

void wwvb_digital_write(int pin, bool val) {
  if ( val ) {
    HAL_GPIO_WritePin( WWVB_Pins[pin].GPIO_Group , 
      WWVB_Pins[pin].GPIO_Pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin( WWVB_Pins[pin].GPIO_Group , 
      WWVB_Pins[pin].GPIO_Pin, GPIO_PIN_RESET);
  }

}

bool wwvb_digital_read(int pin) {
	return (bool) HAL_GPIO_ReadPin( WWVB_Pins[pin].GPIO_Group ,
		WWVB_Pins[pin].GPIO_Pin );
}

void wwvb_gpio_pinmode(int pin, int dir) {
	wwvb_gpio_init.Pin = WWVB_Pins[pin].GPIO_Pin;
	if ( dir == OUTPUT ) {
		wwvb_gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	} else {
		wwvb_gpio_init.Mode = GPIO_MODE_INPUT;
	}
	wwvb_gpio_init.Pull = GPIO_NOPULL;
	wwvb_gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	wwvb_gpio_init.Alternate = 0x0;
	HAL_GPIO_Init(WWVB_Pins[pin].GPIO_Group , &wwvb_gpio_init);	
}


void init_sram2_nocache() {
  Serial.println("Disabling data caching for SRAM2!");
  SCB_CleanDCache();
  SCB_DisableDCache();

  return;

  return;
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  // Disable MPU
  HAL_MPU_Disable();

  // Configure the MPU attributes for the non-cacheable memory region
  // where your variable is located
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = D2_AHBSRAM_BASE; // D2 SRAM1 / SRAM2 , 128K + 128K
  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB; // Adjust the size as needed
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0; // Use an appropriate region number
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  //https://github.com/STMicroelectronics/STM32CubeF7/blob/master/Projects/STM32F767ZI-Nucleo/Examples/ADC/ADC_RegularConversion_DMA/Src/main.c

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  // Enable MPU
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}



void HAL_SPI_IRQHandler_CircFix(SPI_HandleTypeDef *hspi) {
  if ( !hspi->hdmarx ) {
    HAL_SPI_IRQHandler(hspi);
    return;
  }
  if ( hspi->hdmarx && hspi->hdmarx->Init.Mode == DMA_NORMAL ) {
    HAL_SPI_IRQHandler(hspi);
    return;
  }
  // Circular or double buffer mode, need to write my own handler


  uint32_t itsource = hspi->Instance->IER;
  uint32_t itflag   = hspi->Instance->SR;
  uint32_t trigger  = itsource & itflag;
  uint32_t cfg1     = hspi->Instance->CFG1;
  uint32_t handled  = 0UL;

  HAL_SPI_StateTypeDef State = hspi->State;
#if defined (__GNUC__)
  __IO uint16_t *prxdr_16bits = (__IO uint16_t *)(&(hspi->Instance->RXDR));
#endif /* __GNUC__ */

  /* SPI in SUSPEND mode  ----------------------------------------------------*/
  if (HAL_IS_BIT_SET(itflag, SPI_FLAG_SUSP) && HAL_IS_BIT_SET(itsource, SPI_FLAG_EOT))
  {
    /* Clear the Suspend flag */
    __HAL_SPI_CLEAR_SUSPFLAG(hspi);

    /* Suspend on going, Call the Suspend callback */
    // HAL_SPI_SuspendCallback(hspi); NEED TO FIGURE OUT HOW TO DEFINE THIS PROPERLY
    return;
  }

  /* SPI in mode Transmitter and Receiver ------------------------------------*/
  if (HAL_IS_BIT_CLR(trigger, SPI_FLAG_OVR) && HAL_IS_BIT_CLR(trigger, SPI_FLAG_UDR) && \
      HAL_IS_BIT_SET(trigger, SPI_FLAG_DXP))
  {
    hspi->TxISR(hspi);
    hspi->RxISR(hspi);
    handled = 1UL;
  }
  /* SPI in mode Receiver ----------------------------------------------------*/
  if (HAL_IS_BIT_CLR(trigger, SPI_FLAG_OVR) && HAL_IS_BIT_SET(trigger, SPI_FLAG_RXP) && \
      HAL_IS_BIT_CLR(trigger, SPI_FLAG_DXP))
  {
    hspi->RxISR(hspi);
    handled = 1UL;
  }
  /* SPI in mode Transmitter -------------------------------------------------*/
  if (HAL_IS_BIT_CLR(trigger, SPI_FLAG_UDR) && HAL_IS_BIT_SET(trigger, SPI_FLAG_TXP) && \
      HAL_IS_BIT_CLR(trigger, SPI_FLAG_DXP))
  {
    hspi->TxISR(hspi);
    handled = 1UL;
  }
#if defined(USE_SPI_RELOAD_TRANSFER)
  /* SPI Reload  -------------------------------------------------*/
  if (HAL_IS_BIT_SET(trigger, SPI_FLAG_TSERF))
  {
    hspi->Reload.Requested = 0UL;
    __HAL_SPI_CLEAR_TSERFFLAG(hspi);
  }
#endif /* USE_SPI_RELOAD_TRANSFER */
  if (handled != 0UL)
  {
    return;
  }

  /* SPI End Of Transfer: DMA or IT based transfer */
  if (HAL_IS_BIT_SET(trigger, SPI_FLAG_EOT))
  {
    /* Clear EOT/TXTF/SUSP flag */
    __HAL_SPI_CLEAR_EOTFLAG(hspi);
    __HAL_SPI_CLEAR_TXTFFLAG(hspi);
    __HAL_SPI_CLEAR_SUSPFLAG(hspi);

    /* Disable EOT interrupt */
    //__HAL_SPI_DISABLE_IT(hspi, SPI_IT_EOT);

    //hspi->State = HAL_SPI_STATE_BUSY_RX; don't change state
    if (hspi->ErrorCode != HAL_SPI_ERROR_NONE)
    {
      HAL_SPI_ErrorCallback(hspi);
      return;
    }


    /* Call appropriate user callback */
    if (State == HAL_SPI_STATE_BUSY_TX_RX)
    {
      HAL_SPI_TxRxCpltCallback(hspi);
    }
    else if (State == HAL_SPI_STATE_BUSY_RX)
    {
      HAL_SPI_RxCpltCallback(hspi);
    }
    else if (State == HAL_SPI_STATE_BUSY_TX)
    {
      HAL_SPI_TxCpltCallback(hspi);
    }
    else
    {
      /* End of the appropriate call */
    }

    return;
  }

  /* SPI in Error Treatment --------------------------------------------------*/
  if ((trigger & (SPI_FLAG_MODF | SPI_FLAG_OVR | SPI_FLAG_FRE | SPI_FLAG_UDR)) != 0UL)
  {
    /* SPI Overrun error interrupt occurred ----------------------------------*/
    if ((trigger & SPI_FLAG_OVR) != 0UL)
    {
      SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_OVR);
      __HAL_SPI_CLEAR_OVRFLAG(hspi);
    }

    /* SPI Mode Fault error interrupt occurred -------------------------------*/
    if ((trigger & SPI_FLAG_MODF) != 0UL)
    {
      SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_MODF);
      __HAL_SPI_CLEAR_MODFFLAG(hspi);
    }

    /* SPI Frame error interrupt occurred ------------------------------------*/
    if ((trigger & SPI_FLAG_FRE) != 0UL)
    {
      SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FRE);
      __HAL_SPI_CLEAR_FREFLAG(hspi);
    }

    /* SPI Underrun error interrupt occurred ------------------------------------*/
    if ((trigger & SPI_FLAG_UDR) != 0UL)
    {
      SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_UDR);
      __HAL_SPI_CLEAR_UDRFLAG(hspi);
    }

    if (hspi->ErrorCode != HAL_SPI_ERROR_NONE)
    {
      /* Disable SPI peripheral */
      __HAL_SPI_DISABLE(hspi);

      /* Disable all interrupts */
      __HAL_SPI_DISABLE_IT(hspi, (SPI_IT_EOT | SPI_IT_RXP | SPI_IT_TXP | SPI_IT_MODF |
                                  SPI_IT_OVR | SPI_IT_FRE | SPI_IT_UDR));

      /* Disable the SPI DMA requests if enabled */
      if (HAL_IS_BIT_SET(cfg1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN))
      {
        /* Disable the SPI DMA requests */
        CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);

        /* Abort the SPI DMA Rx channel */
        if (hspi->hdmarx != NULL)
        {
          /* Set the SPI DMA Abort callback :
          will lead to call HAL_SPI_ErrorCallback() at end of DMA abort procedure */
          //hspi->hdmarx->XferAbortCallback = SPI_DMAAbortOnError; -> STM didn't implement, NEED TO ADD
          if (HAL_OK != HAL_DMA_Abort_IT(hspi->hdmarx))
          {
            SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_ABORT);
          }
        }
        /* Abort the SPI DMA Tx channel */
        if (hspi->hdmatx != NULL)
        {
          /* Set the SPI DMA Abort callback :
          will lead to call HAL_SPI_ErrorCallback() at end of DMA abort procedure */
          //hspi->hdmatx->XferAbortCallback = SPI_DMAAbortOnError; -> STM didnt implement, NEED TO ADD
          if (HAL_OK != HAL_DMA_Abort_IT(hspi->hdmatx))
          {
            SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_ABORT);
          }
        }
      }
      else
      {
        /* Restore hspi->State to Ready */
        hspi->State = HAL_SPI_STATE_READY;

        HAL_SPI_ErrorCallback(hspi);
      }
    }
    return;
  }
}

// kinda copying UART DMA functions
// Not putting locks so they can be called from interrupt context
HAL_StatusTypeDef HAL_SPI_DMAPause_Fix(SPI_HandleTypeDef *hspi) 
{
  HAL_SPI_StateTypeDef State = hspi->State;
  if ( HAL_IS_BIT_SET(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN) && 
      ( State == HAL_SPI_STATE_BUSY_TX_RX || State == HAL_SPI_STATE_BUSY_TX ) )
  {
    // Disable SPI DMA TX request
    ATOMIC_CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN);    
  }
  if ( HAL_IS_BIT_SET(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN) && 
      ( State == HAL_SPI_STATE_BUSY_TX_RX || State == HAL_SPI_STATE_BUSY_RX ) ) 
  {
    ATOMIC_CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN);
  }
  // disable SPI interrupts
  __HAL_SPI_DISABLE_IT(hspi, (SPI_IT_OVR | SPI_IT_FRE | SPI_IT_MODF));

  // hspi->State = HAL_SPI_STATE_READY; Should I change the state???? For now no

  return HAL_OK;
}
HAL_StatusTypeDef HAL_SPI_DMAResume_Fix(SPI_HandleTypeDef *hspi)
{ // Not sure about this
  HAL_SPI_StateTypeDef State = hspi->State;

  if ( State == HAL_SPI_STATE_BUSY_TX_RX || State == HAL_SPI_STATE_BUSY_TX) {
    // Enable SPI DMA TX request
    ATOMIC_SET_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN);    
  }
  if ( State == HAL_SPI_STATE_BUSY_TX_RX || State == HAL_SPI_STATE_BUSY_RX) {
    // Enable SPI DMA RX request
    ATOMIC_SET_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN);    
  }
  // enable SPI interrupts
  __HAL_SPI_ENABLE_IT(hspi, (SPI_IT_OVR | SPI_IT_FRE | SPI_IT_MODF));
  // should I clear any interrupts? letting higher level handle 

  return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_DMAStop_Fix(SPI_HandleTypeDef *hspi) 
{
  HAL_SPI_StateTypeDef State = hspi->State;
  HAL_StatusTypeDef rx_errorcode = HAL_OK;
  HAL_StatusTypeDef tx_errorcode = HAL_OK;

  // disable peripheral
  __HAL_SPI_DISABLE(hspi);

  // Disable SPI interrupts
  __HAL_SPI_DISABLE_IT(hspi, (SPI_IT_OVR | SPI_IT_FRE | SPI_IT_MODF));

  // Clear any interrupts
  __HAL_SPI_CLEAR_EOTFLAG(hspi);
  __HAL_SPI_CLEAR_TXTFFLAG(hspi);
  __HAL_SPI_CLEAR_SUSPFLAG(hspi);
  __HAL_SPI_CLEAR_TSERFFLAG(hspi);
  __HAL_SPI_CLEAR_OVRFLAG(hspi);
  __HAL_SPI_CLEAR_MODFFLAG(hspi);
  __HAL_SPI_CLEAR_FREFLAG(hspi);
  __HAL_SPI_CLEAR_UDRFLAG(hspi);



  // Stop DMA TX request if ongoing
  if ( State == HAL_SPI_STATE_BUSY_TX_RX || State == HAL_SPI_STATE_BUSY_TX) {
    // Disable SPI DMA TX request
    ATOMIC_CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN);  
      
    if ( hspi->hdmatx ) {
      if ( HAL_DMA_Abort(hspi->hdmatx) != HAL_OK ) {
        if ( HAL_DMA_GetError(hspi->hdmatx) != HAL_DMA_ERROR_NONE ) {
          hspi->ErrorCode = HAL_SPI_ERROR_DMA;
          tx_errorcode = HAL_TIMEOUT; // also want to disable RX if ongoing
        }
      }
    }
  }
  // Stop DMA RX request if ongoing
  if ( State == HAL_SPI_STATE_BUSY_TX_RX || State == HAL_SPI_STATE_BUSY_RX) {
    // Disable SPI DMA RX request
    ATOMIC_CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN);    
    if ( hspi->hdmarx ) {
      if ( HAL_DMA_Abort(hspi->hdmarx) != HAL_OK ) {
        if ( HAL_DMA_GetError(hspi->hdmarx) != HAL_DMA_ERROR_NONE ) {
          hspi->ErrorCode = HAL_SPI_ERROR_DMA;
          rx_errorcode = HAL_TIMEOUT; // also want to disable RX if ongoing
        } 
      }
    }
  }

  // clear DMA flags as well
  __HAL_DMA_CLEAR_FLAG(hspi->hdmarx, __HAL_DMA_GET_TC_FLAG_INDEX(hspi->hdmarx) );
  __HAL_DMA_CLEAR_FLAG(hspi->hdmarx, __HAL_DMA_GET_HT_FLAG_INDEX(hspi->hdmarx) );
  __HAL_DMA_CLEAR_FLAG(hspi->hdmarx, __HAL_DMA_GET_TE_FLAG_INDEX(hspi->hdmarx) );
  __HAL_DMA_CLEAR_FLAG(hspi->hdmarx, __HAL_DMA_GET_FE_FLAG_INDEX(hspi->hdmarx) );

  hspi->State = HAL_SPI_STATE_READY; // set state back to ready
  hspi->hdmarx->State = HAL_DMA_STATE_READY; 
  if ( tx_errorcode != HAL_OK ) {
    return tx_errorcode;
  }
  if ( rx_errorcode != HAL_OK ) {
    return rx_errorcode;
  }
  return HAL_OK;
}

extern "C" {
  void __attribute__((weak)) SPI_DMAHalfReceiveCplt(DMA_HandleTypeDef *hdma) {

  }
  void __attribute__((weak)) SPI_DMAReceiveCplt(DMA_HandleTypeDef *hdma) {

  }
  void __attribute__((weak)) SPI_DMAError(DMA_HandleTypeDef *hdma) {
    
  }
}

HAL_StatusTypeDef HAL_SPI_Receive_DMA_NoStart_HackWriteMDMA(SPI_HandleTypeDef *hspi, uint32_t * mdma_reg, uint32_t * mdma_val_to_write)
{
  HAL_StatusTypeDef errorcode = HAL_OK;

  /* Check Direction parameter */
  assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE_2LINES_RXONLY(hspi->Init.Direction));

  /* Lock the process */
  __HAL_LOCK(hspi);

  if (hspi->State != HAL_SPI_STATE_READY)
  {
    errorcode = HAL_BUSY;
    __HAL_UNLOCK(hspi);
    return errorcode;
  }

  if ((mdma_reg == NULL) || (mdma_val_to_write == NULL))
  {
    errorcode = HAL_ERROR;
    __HAL_UNLOCK(hspi);
    return errorcode;
  }

  /* Set the transaction information */
  hspi->State       = HAL_SPI_STATE_BUSY_RX;
  hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  hspi->pRxBuffPtr  = NULL;
  hspi->RxXferSize  = 1;
  hspi->RxXferCount = 1;

  /*Init field not used in handle to zero */
  hspi->RxISR       = NULL;
  hspi->TxISR       = NULL;
  hspi->TxXferSize  = (uint16_t) 0UL;
  hspi->TxXferCount = (uint16_t) 0UL;

  /* Configure communication direction : 1Line */
  if (hspi->Init.Direction == SPI_DIRECTION_1LINE)
  {
    SPI_1LINE_RX(hspi);
  }
  else
  {
    SPI_2LINES_RX(hspi);
  }

  /* Packing mode management is enabled by the DMA settings */
  if (((hspi->Init.DataSize > SPI_DATASIZE_16BIT) && (hspi->hdmarx->Init.MemDataAlignment != DMA_MDATAALIGN_WORD))    || \
      ((hspi->Init.DataSize > SPI_DATASIZE_8BIT) && ((hspi->hdmarx->Init.MemDataAlignment != DMA_MDATAALIGN_HALFWORD) && \
                                                     (hspi->hdmarx->Init.MemDataAlignment != DMA_MDATAALIGN_WORD))))
  {
    /* Restriction the DMA data received is not allowed in this mode */
    errorcode = HAL_ERROR;
    __HAL_UNLOCK(hspi);
    return errorcode;
  }

  /* Clear RXDMAEN bit */
  CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN);

  /* Adjust XferCount according to DMA alignment / Data size */
  if (hspi->Init.DataSize <= SPI_DATASIZE_8BIT)
  {
    if (hspi->hdmarx->Init.MemDataAlignment == DMA_MDATAALIGN_HALFWORD)
    {
      hspi->RxXferCount = (hspi->RxXferCount + (uint16_t) 1UL) >> 1UL;
    }
    if (hspi->hdmarx->Init.MemDataAlignment == DMA_MDATAALIGN_WORD)
    {
      hspi->RxXferCount = (hspi->RxXferCount + (uint16_t) 3UL) >> 2UL;
    }
  }
  else if (hspi->Init.DataSize <= SPI_DATASIZE_16BIT)
  {
    if (hspi->hdmarx->Init.MemDataAlignment == DMA_MDATAALIGN_WORD)
    {
      hspi->RxXferCount = (hspi->RxXferCount + (uint16_t) 1UL) >> 1UL;
    }
  }
  else
  {
    /* Adjustment done */
  }

  /* Set the SPI RxDMA Half transfer complete callback */
  hspi->hdmarx->XferHalfCpltCallback = SPI_DMAHalfReceiveCplt;

  /* Set the SPI Rx DMA transfer complete callback */
  hspi->hdmarx->XferCpltCallback = SPI_DMAReceiveCplt;

  /* Set the DMA error callback */
  hspi->hdmarx->XferErrorCallback = SPI_DMAError;

  /* Set the DMA AbortCpltCallback */
  hspi->hdmarx->XferAbortCallback = NULL;

  __HAL_SPI_DISABLE(hspi);
  CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN);
  

  /* Enable the Rx DMA Stream/Channel  */
  if (HAL_OK != HAL_DMA_Start(hspi->hdmarx, (uint32_t)mdma_val_to_write, (uint32_t)mdma_reg,
                                 hspi->RxXferCount))
  {
    /* Update SPI error code */
    SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_DMA);

    /* Unlock the process */
    __HAL_UNLOCK(hspi);

    hspi->State = HAL_SPI_STATE_READY;
    errorcode = HAL_ERROR;
    return errorcode;
  }

  /* Set the number of data at current transfer */
  if (hspi->hdmarx->Init.Mode == DMA_CIRCULAR)
  {
    MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, 0UL);
  }
  else
  {
    MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, 1);
  }

  /* Enable Rx DMA Request */
  SET_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN);

  /* Enable the SPI Error Interrupt Bit */
  //__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_OVR | SPI_IT_FRE | SPI_IT_MODF));

  /* Enable SPI peripheral */
  //__HAL_SPI_ENABLE(hspi);

  if (hspi->Init.Mode == SPI_MODE_MASTER)
  {
    /* Master transfer start */
    SET_BIT(hspi->Instance->CR1, SPI_CR1_CSTART);
  }

  /* Unlock the process */
  __HAL_UNLOCK(hspi);
  return errorcode;  
}

HAL_StatusTypeDef HAL_SPI_Receive_DMA_NoStart_NoInterrupt(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
  HAL_StatusTypeDef errorcode = HAL_OK;

  /* Check Direction parameter */
  assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE_2LINES_RXONLY(hspi->Init.Direction));

  /* Lock the process */
  __HAL_LOCK(hspi);

  if (hspi->State != HAL_SPI_STATE_READY)
  {
    errorcode = HAL_BUSY;
    __HAL_UNLOCK(hspi);
    return errorcode;
  }

  if ((pData == NULL) || (Size == 0UL))
  {
    errorcode = HAL_ERROR;
    __HAL_UNLOCK(hspi);
    return errorcode;
  }

  /* Set the transaction information */
  hspi->State       = HAL_SPI_STATE_BUSY_RX;
  hspi->ErrorCode   = HAL_SPI_ERROR_NONE;
  hspi->pRxBuffPtr  = (uint8_t *)pData;
  hspi->RxXferSize  = Size;
  hspi->RxXferCount = Size;

  /*Init field not used in handle to zero */
  hspi->RxISR       = NULL;
  hspi->TxISR       = NULL;
  hspi->TxXferSize  = (uint16_t) 0UL;
  hspi->TxXferCount = (uint16_t) 0UL;

  /* Configure communication direction : 1Line */
  if (hspi->Init.Direction == SPI_DIRECTION_1LINE)
  {
    SPI_1LINE_RX(hspi);
  }
  else
  {
    SPI_2LINES_RX(hspi);
  }

  /* Packing mode management is enabled by the DMA settings */
  if (((hspi->Init.DataSize > SPI_DATASIZE_16BIT) && (hspi->hdmarx->Init.MemDataAlignment != DMA_MDATAALIGN_WORD))    || \
      ((hspi->Init.DataSize > SPI_DATASIZE_8BIT) && ((hspi->hdmarx->Init.MemDataAlignment != DMA_MDATAALIGN_HALFWORD) && \
                                                     (hspi->hdmarx->Init.MemDataAlignment != DMA_MDATAALIGN_WORD))))
  {
    /* Restriction the DMA data received is not allowed in this mode */
    errorcode = HAL_ERROR;
    __HAL_UNLOCK(hspi);
    return errorcode;
  }

  /* Clear RXDMAEN bit */
  CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN);

  /* Adjust XferCount according to DMA alignment / Data size */
  if (hspi->Init.DataSize <= SPI_DATASIZE_8BIT)
  {
    if (hspi->hdmarx->Init.MemDataAlignment == DMA_MDATAALIGN_HALFWORD)
    {
      hspi->RxXferCount = (hspi->RxXferCount + (uint16_t) 1UL) >> 1UL;
    }
    if (hspi->hdmarx->Init.MemDataAlignment == DMA_MDATAALIGN_WORD)
    {
      hspi->RxXferCount = (hspi->RxXferCount + (uint16_t) 3UL) >> 2UL;
    }
  }
  else if (hspi->Init.DataSize <= SPI_DATASIZE_16BIT)
  {
    if (hspi->hdmarx->Init.MemDataAlignment == DMA_MDATAALIGN_WORD)
    {
      hspi->RxXferCount = (hspi->RxXferCount + (uint16_t) 1UL) >> 1UL;
    }
  }
  else
  {
    /* Adjustment done */
  }

  /* Set the SPI RxDMA Half transfer complete callback */
  hspi->hdmarx->XferHalfCpltCallback = SPI_DMAHalfReceiveCplt;

  /* Set the SPI Rx DMA transfer complete callback */
  hspi->hdmarx->XferCpltCallback = SPI_DMAReceiveCplt;

  /* Set the DMA error callback */
  hspi->hdmarx->XferErrorCallback = SPI_DMAError;

  /* Set the DMA AbortCpltCallback */
  hspi->hdmarx->XferAbortCallback = NULL;

  Serial.println("Enabling DMA TC interrupt for MDMA");
  ((DMA_Stream_TypeDef   *)hspi->hdmarx->Instance)->CR |= DMA_IT_TC;

  /* Enable the Rx DMA Stream/Channel  */
  if (HAL_OK != HAL_DMA_Start(hspi->hdmarx, (uint32_t)&hspi->Instance->RXDR, (uint32_t)hspi->pRxBuffPtr,
                                 hspi->RxXferCount))
  {
    /* Update SPI error code */
    SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_DMA);

    /* Unlock the process */
    __HAL_UNLOCK(hspi);

    hspi->State = HAL_SPI_STATE_READY;
    errorcode = HAL_ERROR;
    return errorcode;
  }

  /* Set the number of data at current transfer */
  if (hspi->hdmarx->Init.Mode == DMA_CIRCULAR)
  {
    MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, 0UL);
  }
  else
  {
    MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, Size);
  }

  /* Enable Rx DMA Request */
  SET_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN);

  /* Enable the SPI Error Interrupt Bit */
  //__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_OVR | SPI_IT_FRE | SPI_IT_MODF));

  /* Enable SPI peripheral */
  //__HAL_SPI_ENABLE(hspi);

  if (hspi->Init.Mode == SPI_MODE_MASTER)
  {
    /* Master transfer start */
    SET_BIT(hspi->Instance->CR1, SPI_CR1_CSTART);
  }

  /* Unlock the process */
  __HAL_UNLOCK(hspi);
  return errorcode;
}

// Function to count the number of set bits in an 8-bit value
unsigned int countSetBits(unsigned char n) {
    unsigned int count = 0;
    while (n) {
        count += n & 1;
        n >>= 1;
    }
    return count;
}

// Function to return the difference between the number of bits set and not set
int bitsDifference(uint8_t value) {
    int setBits = countSetBits(value);
    // Since a uint8_t has 8 bits, the number of unset bits is 8 - setBits
    // The difference is setBits - (8 - setBits) = 2 * setBits - 8
    return 2 * setBits - 8;
}

void init_sram1_data() {
  // fill in the lookup table
  // Sigma delta output is a signed value between -8 and 8
  // but to make it work with DFSDM, keep it positive shift it to 0 to 16 -> NAH
  for ( int i = 0; i < 256; i++ ) {
    //sram1_data->SDR_lookup_table[i] = (uint8_t)(bitsDifference((uint8_t)i) + 8);
    sram1_data->SDR_lookup_table[i] = (int8_t)(bitsDifference( (uint8_t)i) );
  }  
  
}



int32_t extend_sign_24bit(uint32_t value) {
    // Assume 'value' contains the 24-bit number in the least significant bits.
    // Check if the 24-bit number is negative.
    if (value & 0x800000) {
        // If it's negative, extend the sign by setting the upper 8 bits.
        value |= 0xFF000000;
    } else {
        // If it's positive, clear the upper 8 bits to ensure it's treated as a positive 32-bit integer.
        value &= 0x00FFFFFF;
    }
    return value;
}



// Function to count the number of 1 bits in a uint32_t
int countOneBits(uint32_t n) {
    int count = 0;
    while (n) {
        n &= (n - 1); // Clear the least significant bit set
        count++;
    }
    return count;
}

// Function to calculate the difference in the number of 1 bits and the number of 0 bits
int bitDifference(uint32_t value) {
    int ones = countOneBits(value);
    int zeros = 32 - ones; // Since uint32_t is 32 bits, the number of 0 bits is 32 minus the number of 1 bits
    return ones - zeros;
}

#define DFSDM_FLTCR1_MSB_RCH_OFFSET     8
#define DFSDM_MSB_MASK                  0xFFFF0000U
#define DFSDM_LSB_MASK                  0x0000FFFFU
#define DFSDM_CKAB_TIMEOUT              5000U
#define DFSDM1_CHANNEL_NUMBER           8U
static __IO uint32_t                v_dfsdm1ChannelCounter = 0;
static DFSDM_Channel_HandleTypeDef *a_dfsdm1ChannelHandle[DFSDM1_CHANNEL_NUMBER] = {NULL};


static uint32_t DFSDM_GetChannelFromInstance(const DFSDM_Channel_TypeDef* Instance)
{
  uint32_t channel;

  /* Get channel from instance */
  if(Instance == DFSDM1_Channel0)
  {
    channel = 0;
  }
#if defined(DFSDM2_Channel0)
  else if (Instance == DFSDM2_Channel0)
  {
    channel = 0;
  }
  else if (Instance == DFSDM2_Channel1)
  {
    channel = 1;
  }
#endif /* DFSDM2_Channel0 */
  else if(Instance == DFSDM1_Channel1)
  {
    channel = 1;
  }
  else if(Instance == DFSDM1_Channel2)
  {
    channel = 2;
  }
  else if(Instance == DFSDM1_Channel3)
  {
    channel = 3;
  }
  else if(Instance == DFSDM1_Channel4)
  {
    channel = 4;
  }
  else if(Instance == DFSDM1_Channel5)
  {
    channel = 5;
  }
  else if(Instance == DFSDM1_Channel6)
  {
    channel = 6;
  }
  else /* DFSDM1_Channel7 */
  {
    channel = 7;
  }

  return channel;
}



HAL_StatusTypeDef HAL_DFSDM_ChannelInit_Debug(DFSDM_Channel_HandleTypeDef *hdfsdm_channel)
{
  __IO uint32_t               *channelCounterPtr;
  DFSDM_Channel_HandleTypeDef **channelHandleTable;
  DFSDM_Channel_TypeDef       *channel0Instance;

  /* Check DFSDM Channel handle */
  if(hdfsdm_channel == NULL)
  {
    return HAL_ERROR;
  }

  /* Check parameters */
  assert_param(IS_DFSDM_CHANNEL_ALL_INSTANCE(hdfsdm_channel->Instance));
  assert_param(IS_FUNCTIONAL_STATE(hdfsdm_channel->Init.OutputClock.Activation));
  assert_param(IS_DFSDM_CHANNEL_INPUT(hdfsdm_channel->Init.Input.Multiplexer));
  assert_param(IS_DFSDM_CHANNEL_DATA_PACKING(hdfsdm_channel->Init.Input.DataPacking));
  assert_param(IS_DFSDM_CHANNEL_INPUT_PINS(hdfsdm_channel->Init.Input.Pins));
  assert_param(IS_DFSDM_CHANNEL_SERIAL_INTERFACE_TYPE(hdfsdm_channel->Init.SerialInterface.Type));
  assert_param(IS_DFSDM_CHANNEL_SPI_CLOCK(hdfsdm_channel->Init.SerialInterface.SpiClock));
  assert_param(IS_DFSDM_CHANNEL_FILTER_ORDER(hdfsdm_channel->Init.Awd.FilterOrder));
  assert_param(IS_DFSDM_CHANNEL_FILTER_OVS_RATIO(hdfsdm_channel->Init.Awd.Oversampling));
  assert_param(IS_DFSDM_CHANNEL_OFFSET(hdfsdm_channel->Init.Offset));
  assert_param(IS_DFSDM_CHANNEL_RIGHT_BIT_SHIFT(hdfsdm_channel->Init.RightBitShift));


  channelCounterPtr  = &v_dfsdm1ChannelCounter;
  channelHandleTable = a_dfsdm1ChannelHandle;
  channel0Instance   = DFSDM1_Channel0;


  /* Check that channel has not been already initialized */
  if (channelHandleTable[DFSDM_GetChannelFromInstance(hdfsdm_channel->Instance)] != NULL)
  {
    Serial.println("HAL_DFSDM_ChannelInit_Debug CHANNEL ALREADY INITIALIZED");
    return HAL_ERROR;
  }

#if (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1)
  /* Reset callback pointers to the weak predefined callbacks */
  hdfsdm_channel->CkabCallback = HAL_DFSDM_ChannelCkabCallback;
  hdfsdm_channel->ScdCallback  = HAL_DFSDM_ChannelScdCallback;

  /* Call MSP init function */
  if(hdfsdm_channel->MspInitCallback == NULL)
  {
    hdfsdm_channel->MspInitCallback = HAL_DFSDM_ChannelMspInit;
  }
  hdfsdm_channel->MspInitCallback(hdfsdm_channel);
#else
  /* Call MSP init function */
  HAL_DFSDM_ChannelMspInit(hdfsdm_channel);
#endif

  /* Update the channel counter */
  (*channelCounterPtr)++;

  /* Configure output serial clock and enable global DFSDM interface only for first channel */
  if(*channelCounterPtr == 1U)
  {
    Serial.println("HAL_DFSDM_ChannelInit_Debug ENABLING GLOBAL DFSDM ON FIRST CHANNEL");
    assert_param(IS_DFSDM_CHANNEL_OUTPUT_CLOCK(hdfsdm_channel->Init.OutputClock.Selection));
    /* Set the output serial clock source */
    channel0Instance->CHCFGR1 &= ~(DFSDM_CHCFGR1_CKOUTSRC);
    channel0Instance->CHCFGR1 |= hdfsdm_channel->Init.OutputClock.Selection;

    /* Reset clock divider */
    channel0Instance->CHCFGR1 &= ~(DFSDM_CHCFGR1_CKOUTDIV);
    if(hdfsdm_channel->Init.OutputClock.Activation == ENABLE)
    {
      assert_param(IS_DFSDM_CHANNEL_OUTPUT_CLOCK_DIVIDER(hdfsdm_channel->Init.OutputClock.Divider));
      /* Set the output clock divider */
      channel0Instance->CHCFGR1 |= (uint32_t)((hdfsdm_channel->Init.OutputClock.Divider - 1U) <<
                                              DFSDM_CHCFGR1_CKOUTDIV_Pos);
    }

    /* enable the DFSDM global interface */
    channel0Instance->CHCFGR1 |= DFSDM_CHCFGR1_DFSDMEN;
  }
  Serial.println("HAL_DFSDM_ChannelInit_Debug SET CHANNEL INPUT PARAMETERS");

  /* Set channel input parameters */
  hdfsdm_channel->Instance->CHCFGR1 &= ~(DFSDM_CHCFGR1_DATPACK | DFSDM_CHCFGR1_DATMPX |
                                         DFSDM_CHCFGR1_CHINSEL);
  hdfsdm_channel->Instance->CHCFGR1 |= (hdfsdm_channel->Init.Input.Multiplexer |
                                        hdfsdm_channel->Init.Input.DataPacking |
                                        hdfsdm_channel->Init.Input.Pins);

  /* Set serial interface parameters */
  hdfsdm_channel->Instance->CHCFGR1 &= ~(DFSDM_CHCFGR1_SITP | DFSDM_CHCFGR1_SPICKSEL);
  hdfsdm_channel->Instance->CHCFGR1 |= (hdfsdm_channel->Init.SerialInterface.Type |
                                        hdfsdm_channel->Init.SerialInterface.SpiClock);

  /* Set analog watchdog parameters */
  hdfsdm_channel->Instance->CHAWSCDR &= ~(DFSDM_CHAWSCDR_AWFORD | DFSDM_CHAWSCDR_AWFOSR);
  hdfsdm_channel->Instance->CHAWSCDR |= (hdfsdm_channel->Init.Awd.FilterOrder |
                                         ((hdfsdm_channel->Init.Awd.Oversampling - 1U) << DFSDM_CHAWSCDR_AWFOSR_Pos));

  /* Set channel offset and right bit shift */
  hdfsdm_channel->Instance->CHCFGR2 &= ~(DFSDM_CHCFGR2_OFFSET | DFSDM_CHCFGR2_DTRBS);
  hdfsdm_channel->Instance->CHCFGR2 |= (((uint32_t) hdfsdm_channel->Init.Offset << DFSDM_CHCFGR2_OFFSET_Pos) |
                                        (hdfsdm_channel->Init.RightBitShift << DFSDM_CHCFGR2_DTRBS_Pos));

  /* Enable DFSDM channel */
  hdfsdm_channel->Instance->CHCFGR1 |= DFSDM_CHCFGR1_CHEN;

  /* Set DFSDM Channel to ready state */
  hdfsdm_channel->State = HAL_DFSDM_CHANNEL_STATE_READY;

  /* Store channel handle in DFSDM channel handle table */
  channelHandleTable[DFSDM_GetChannelFromInstance(hdfsdm_channel->Instance)] = hdfsdm_channel;

  return HAL_OK;
}


HAL_StatusTypeDef HAL_DFSDM_FilterInit_Debug(DFSDM_Filter_HandleTypeDef *hdfsdm_filter)
{
  const DFSDM_Filter_TypeDef *filter0Instance;

  /* Check DFSDM Channel handle */
  if(hdfsdm_filter == NULL)
  {
    return HAL_ERROR;
  }

  /* Check parameters */
  assert_param(IS_DFSDM_FILTER_ALL_INSTANCE(hdfsdm_filter->Instance));
  assert_param(IS_DFSDM_FILTER_REG_TRIGGER(hdfsdm_filter->Init.RegularParam.Trigger));
  assert_param(IS_FUNCTIONAL_STATE(hdfsdm_filter->Init.RegularParam.FastMode));
  assert_param(IS_FUNCTIONAL_STATE(hdfsdm_filter->Init.RegularParam.DmaMode));
  assert_param(IS_DFSDM_FILTER_INJ_TRIGGER(hdfsdm_filter->Init.InjectedParam.Trigger));
  assert_param(IS_FUNCTIONAL_STATE(hdfsdm_filter->Init.InjectedParam.ScanMode));
  assert_param(IS_FUNCTIONAL_STATE(hdfsdm_filter->Init.InjectedParam.DmaMode));
  assert_param(IS_DFSDM_FILTER_SINC_ORDER(hdfsdm_filter->Init.FilterParam.SincOrder));
  assert_param(IS_DFSDM_FILTER_OVS_RATIO(hdfsdm_filter->Init.FilterParam.Oversampling));
  assert_param(IS_DFSDM_FILTER_INTEGRATOR_OVS_RATIO(hdfsdm_filter->Init.FilterParam.IntOversampling));


  filter0Instance = DFSDM1_Filter0;


  /* Check parameters compatibility */
  if ((hdfsdm_filter->Instance == filter0Instance) &&
      ((hdfsdm_filter->Init.RegularParam.Trigger  == DFSDM_FILTER_SYNC_TRIGGER) ||
       (hdfsdm_filter->Init.InjectedParam.Trigger == DFSDM_FILTER_SYNC_TRIGGER)))
  {
    Serial.println("HAL_DFSDM_FilterInit_Debug FILTER0 INSTANCE TRIGGER CHECK FAILED");
    return HAL_ERROR;
  }

  /* Initialize DFSDM filter variables with default values */
  hdfsdm_filter->RegularContMode     = DFSDM_CONTINUOUS_CONV_OFF;
  hdfsdm_filter->InjectedChannelsNbr = 1;
  hdfsdm_filter->InjConvRemaining    = 1;
  hdfsdm_filter->ErrorCode           = DFSDM_FILTER_ERROR_NONE;

#if (USE_HAL_DFSDM_REGISTER_CALLBACKS == 1)
  /* Reset callback pointers to the weak predefined callbacks */
  hdfsdm_filter->AwdCallback             = HAL_DFSDM_FilterAwdCallback;
  hdfsdm_filter->RegConvCpltCallback     = HAL_DFSDM_FilterRegConvCpltCallback;
  hdfsdm_filter->RegConvHalfCpltCallback = HAL_DFSDM_FilterRegConvHalfCpltCallback;
  hdfsdm_filter->InjConvCpltCallback     = HAL_DFSDM_FilterInjConvCpltCallback;
  hdfsdm_filter->InjConvHalfCpltCallback = HAL_DFSDM_FilterInjConvHalfCpltCallback;
  hdfsdm_filter->ErrorCallback           = HAL_DFSDM_FilterErrorCallback;

  /* Call MSP init function */
  if(hdfsdm_filter->MspInitCallback == NULL)
  {
    hdfsdm_filter->MspInitCallback = HAL_DFSDM_FilterMspInit;
  }
  hdfsdm_filter->MspInitCallback(hdfsdm_filter);
#else
  /* Call MSP init function */
  HAL_DFSDM_FilterMspInit(hdfsdm_filter);
#endif

  /* Set regular parameters */
  hdfsdm_filter->Instance->FLTCR1 &= ~(DFSDM_FLTCR1_RSYNC);
  if(hdfsdm_filter->Init.RegularParam.FastMode == ENABLE)
  {
    hdfsdm_filter->Instance->FLTCR1 |= DFSDM_FLTCR1_FAST;
  }
  else
  {
    hdfsdm_filter->Instance->FLTCR1 &= ~(DFSDM_FLTCR1_FAST);
  }

  if(hdfsdm_filter->Init.RegularParam.DmaMode == ENABLE)
  {
    hdfsdm_filter->Instance->FLTCR1 |= DFSDM_FLTCR1_RDMAEN;
  }
  else
  {
    hdfsdm_filter->Instance->FLTCR1 &= ~(DFSDM_FLTCR1_RDMAEN);
  }

  /* Set injected parameters */
  Serial.println("HAL_DFSDM_FilterInit_Debug Set injected parameters");
  hdfsdm_filter->Instance->FLTCR1 &= ~(DFSDM_FLTCR1_JSYNC | DFSDM_FLTCR1_JEXTEN | DFSDM_FLTCR1_JEXTSEL);
  if(hdfsdm_filter->Init.InjectedParam.Trigger == DFSDM_FILTER_EXT_TRIGGER)
  {
    assert_param(IS_DFSDM_FILTER_EXT_TRIG(hdfsdm_filter->Init.InjectedParam.ExtTrigger));
    assert_param(IS_DFSDM_FILTER_EXT_TRIG_EDGE(hdfsdm_filter->Init.InjectedParam.ExtTriggerEdge));
    hdfsdm_filter->Instance->FLTCR1 |= (hdfsdm_filter->Init.InjectedParam.ExtTrigger);
  }

  if(hdfsdm_filter->Init.InjectedParam.ScanMode == ENABLE)
  {
    hdfsdm_filter->Instance->FLTCR1 |= DFSDM_FLTCR1_JSCAN;
  }
  else
  {
    hdfsdm_filter->Instance->FLTCR1 &= ~(DFSDM_FLTCR1_JSCAN);
  }

  if(hdfsdm_filter->Init.InjectedParam.DmaMode == ENABLE)
  {
    hdfsdm_filter->Instance->FLTCR1 |= DFSDM_FLTCR1_JDMAEN;
  }
  else
  {
    hdfsdm_filter->Instance->FLTCR1 &= ~(DFSDM_FLTCR1_JDMAEN);
  }

  /* Set filter parameters */
  hdfsdm_filter->Instance->FLTFCR &= ~(DFSDM_FLTFCR_FORD | DFSDM_FLTFCR_FOSR | DFSDM_FLTFCR_IOSR);
  hdfsdm_filter->Instance->FLTFCR |= (hdfsdm_filter->Init.FilterParam.SincOrder |
                                      ((hdfsdm_filter->Init.FilterParam.Oversampling - 1U) << DFSDM_FLTFCR_FOSR_Pos) |
                                      (hdfsdm_filter->Init.FilterParam.IntOversampling - 1U));

  /* Store regular and injected triggers and injected scan mode*/
  hdfsdm_filter->RegularTrigger   = hdfsdm_filter->Init.RegularParam.Trigger;
  hdfsdm_filter->InjectedTrigger  = hdfsdm_filter->Init.InjectedParam.Trigger;
  hdfsdm_filter->ExtTriggerEdge   = hdfsdm_filter->Init.InjectedParam.ExtTriggerEdge;
  hdfsdm_filter->InjectedScanMode = hdfsdm_filter->Init.InjectedParam.ScanMode;

  /* Enable DFSDM filter */
  hdfsdm_filter->Instance->FLTCR1 |= DFSDM_FLTCR1_DFEN;

  /* Set DFSDM filter to ready state */
  hdfsdm_filter->State = HAL_DFSDM_FILTER_STATE_READY;
  Serial.println("HAL_DFSDM_FilterInit_Debug GOOD DONE");

  return HAL_OK;
}




HAL_StatusTypeDef HAL_DFSDM_FilterConfigRegChannel_Debug(DFSDM_Filter_HandleTypeDef *hdfsdm_filter,
                                                   uint32_t                    Channel,
                                                   uint32_t                    ContinuousMode)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Check parameters */
  assert_param(IS_DFSDM_FILTER_ALL_INSTANCE(hdfsdm_filter->Instance));
  assert_param(IS_DFSDM_REGULAR_CHANNEL(Channel));
  assert_param(IS_DFSDM_CONTINUOUS_MODE(ContinuousMode));

  /* Check DFSDM filter state */
  if((hdfsdm_filter->State != HAL_DFSDM_FILTER_STATE_RESET) &&
      (hdfsdm_filter->State != HAL_DFSDM_FILTER_STATE_ERROR))
  {
    Serial.println("HAL_DFSDM_FilterConfigRegChannel_Debug STATE CHECK");
    /* Configure channel and continuous mode for regular conversion */
    hdfsdm_filter->Instance->FLTCR1 &= ~(DFSDM_FLTCR1_RCH | DFSDM_FLTCR1_RCONT);
    if(ContinuousMode == DFSDM_CONTINUOUS_CONV_ON)
    {
      hdfsdm_filter->Instance->FLTCR1 |= (uint32_t) (((Channel & DFSDM_MSB_MASK) << DFSDM_FLTCR1_MSB_RCH_OFFSET) |
                                                    DFSDM_FLTCR1_RCONT);
    }
    else
    {
      hdfsdm_filter->Instance->FLTCR1 |= (uint32_t) ((Channel & DFSDM_MSB_MASK) << DFSDM_FLTCR1_MSB_RCH_OFFSET);
    }
    /* Store continuous mode information */
    hdfsdm_filter->RegularContMode = ContinuousMode;
  }
  else
  {
    Serial.println("HAL_DFSDM_FilterConfigRegChannel_Debug STATE CHECK FAIL");
    status = HAL_ERROR;
  }

  /* Return function status */
  return status;
}



HAL_StatusTypeDef HAL_DFSDM_FilterRegularStart_DMA_Debug(DFSDM_Filter_HandleTypeDef *hdfsdm_filter,
                                                   int32_t                    *pData,
                                                   uint32_t                    Length)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Check parameters */
  assert_param(IS_DFSDM_FILTER_ALL_INSTANCE(hdfsdm_filter->Instance));

  /* Check destination address and length */
  if((pData == NULL) || (Length == 0U))
  {
    Serial.println("HAL_DFSDM_FilterRegularStart_DMA_Debug length check fail!");
    status = HAL_ERROR;
  }
  /* Check that DMA is enabled for regular conversion */
  else if((hdfsdm_filter->Instance->FLTCR1 & DFSDM_FLTCR1_RDMAEN) != DFSDM_FLTCR1_RDMAEN)
  {
    Serial.println("HAL_DFSDM_FilterRegularStart_DMA_Debug DMA enable check FAIL!");
    status = HAL_ERROR;
  }
  /* Check parameters compatibility */
  /* THIS IS DEBUG USE CASE FOR SDR BOARD
  else if((hdfsdm_filter->RegularTrigger == DFSDM_FILTER_SW_TRIGGER) && \
           (hdfsdm_filter->RegularContMode == DFSDM_CONTINUOUS_CONV_OFF) && \
           (hdfsdm_filter->hdmaReg->Init.Mode == DMA_NORMAL) && \
           (Length != 1U))
  {
    Serial.println("HAL_DFSDM_FilterRegularStart_DMA_Debug COMPATIBILITY CHECK FAIL");
    status = HAL_ERROR;
  }
  */
  /* THIS IS MY USE CASE FOR SDR BOARD
  else if((hdfsdm_filter->RegularTrigger == DFSDM_FILTER_SW_TRIGGER) && \
           (hdfsdm_filter->RegularContMode == DFSDM_CONTINUOUS_CONV_OFF) && \
           (hdfsdm_filter->hdmaReg->Init.Mode == DMA_CIRCULAR))
  {
    Serial.println("HAL_DFSDM_FilterRegularStart_DMA_Debug CIRCULAR COMPAT CHECK FAIL");
    status = HAL_ERROR;
  }
  */
  /* Check DFSDM filter state */
  else if((hdfsdm_filter->State == HAL_DFSDM_FILTER_STATE_READY) || \
           (hdfsdm_filter->State == HAL_DFSDM_FILTER_STATE_INJ))
  {
    Serial.println("HAL_DFSDM_FilterRegularStart_DMA_Debug START DMA WITHOUT INTERRUPT");
    /* Set callbacks on DMA handler */
    //hdfsdm_filter->hdmaReg->XferCpltCallback = DFSDM_DMARegularConvCplt;
    //hdfsdm_filter->hdmaReg->XferErrorCallback = DFSDM_DMAError;
    //hdfsdm_filter->hdmaReg->XferHalfCpltCallback = (hdfsdm_filter->hdmaReg->Init.Mode == DMA_CIRCULAR) ?\
    //                                               DFSDM_DMARegularHalfConvCplt : NULL;

    // Start DMA in interrupt mode 
    //if(HAL_DMA_Start_IT(hdfsdm_filter->hdmaReg, (uint32_t)&hdfsdm_filter->Instance->FLTRDATAR, \
    //                     (uint32_t) pData, Length) != HAL_OK)
    // I DONT WANT INTERRUPTS FOR SDR BOARD I THINK, maybe later enable
    if(HAL_DMA_Start(hdfsdm_filter->hdmaReg, (uint32_t)&hdfsdm_filter->Instance->FLTRDATAR, \
                           (uint32_t) pData, Length) != HAL_OK)
    {
      Serial.println("HAL_DFSDM_FilterRegularStart_DMA_Debug HAL_DMA_START FAIL");
      /* Set DFSDM filter in error state */
      hdfsdm_filter->State = HAL_DFSDM_FILTER_STATE_ERROR;
      status = HAL_ERROR;
    }
    else
    {
      Serial.println("HAL_DFSDM_FilterRegularStart_DMA_Debug DMA started GOOD!");
      /* Start regular conversion once */
      //DFSDM_RegConvStart(hdfsdm_filter);
      /* Software start of regular conversion */
      hdfsdm_filter->Instance->FLTCR1 |= DFSDM_FLTCR1_RSWSTART;
      /* Update DFSDM filter state */
      hdfsdm_filter->State = (hdfsdm_filter->State == HAL_DFSDM_FILTER_STATE_READY) ? \
          HAL_DFSDM_FILTER_STATE_REG : HAL_DFSDM_FILTER_STATE_REG_INJ;
    }
  }
  else
  {
    Serial.println("HAL_DFSDM_FilterRegularStart_DMA_Debug END ERROR");
    status = HAL_ERROR;
  }
  /* Return function status */
  return status;
}                                                   
