#include "WWVB_Arduino.h"



sram1_data_struct * sram1_data = (sram1_data_struct *)0x30000000;
sram2_data_struct * sram2_data = (sram2_data_struct *)0x30020000;
//sram3_data_struct * sram3_data = (sram3_data_struct *)0x30040000;

volatile uint8_t spi1_rx_data = 0;
volatile uint8_t spi2_rx_data = 0;
uint8_t SDR_lookup_table[256];


char print_buffer[10000];


WWVB_Pin WWVB_Pins[] = {
  // Clockmatrix PLL
	PIN_STRUCT(PLL_SDA),
	PIN_STRUCT(PLL_SCL),
	PIN_STRUCT(PLL_RST),

  // SX1276
  PIN_STRUCT(SX1276_DIO0),
  PIN_STRUCT(SX1276_SCK),
  PIN_STRUCT(SX1276_MISO),
  PIN_STRUCT(SX1276_MOSI),
  PIN_STRUCT(SX1276_NSS),
  PIN_STRUCT(SX1276_RST),

  // Antenna select
  PIN_STRUCT(LORA_LF_TXRX_SEL),
  PIN_STRUCT(LORA_HF_TXRX_SEL),
  PIN_STRUCT(LORA_LF_HF_SEL),

  PIN_STRUCT(AT86_SPI_SEL),
  PIN_STRUCT(AT86_IRQ),
  PIN_STRUCT(AT86_RST),
  PIN_STRUCT(SDR_WIFI_ATTEN_SEL),
  PIN_STRUCT(SDR_SUBG_ATTEN_SEL),

  PIN_STRUCT(ESP32_RST),

  PIN_STRUCT(QSPI_FPGA_SCLK),
  PIN_STRUCT(QSPI_FPGA_MOSI),
  PIN_STRUCT(QSPI_FPGA_WP),
  PIN_STRUCT(QSPI_FPGA_MISO),
  PIN_STRUCT(QSPI_FPGA_RESET),
  PIN_STRUCT(QSPI_FPGA_CS),
  PIN_STRUCT(FPGA_PROGRAMN),
  PIN_STRUCT(FPGA_INITN),
  PIN_STRUCT(FPGA_DONE),
  PIN_STRUCT(STM_FPGA_SPARE1),
  PIN_STRUCT(STM_FPGA_SPARE2),
  PIN_STRUCT(STM_FPGA_SPARE3),
  PIN_STRUCT(STM_FPGA_SPARE4),
  PIN_STRUCT(STM_FPGA_SPARE5),
  PIN_STRUCT(STM_FPGA_SPARE6),
  PIN_STRUCT(FPGA_SDA),
  PIN_STRUCT(FPGA_SCL),
  PIN_STRUCT(SPI1_SCK),
  PIN_STRUCT(SPI1_MOSI),
  PIN_STRUCT(SPI1_MISO),
  PIN_STRUCT(SPI2_SCK),
  PIN_STRUCT(SPI2_MOSI),
  PIN_STRUCT(SPI2_MISO),
  PIN_STRUCT(SPI3_SCK),
  PIN_STRUCT(SPI3_MOSI),
  PIN_STRUCT(SPI3_MISO),
  PIN_STRUCT(SPI4_SCK),
  PIN_STRUCT(SPI4_MOSI),
  PIN_STRUCT(SPI4_MISO),



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

void wwvb_gpio_pinmode_pullup(int pin, int dir) {
	wwvb_gpio_init.Pin = WWVB_Pins[pin].GPIO_Pin;
	if ( dir == OUTPUT ) {
		wwvb_gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	} else {
		wwvb_gpio_init.Mode = GPIO_MODE_INPUT;
	}
	wwvb_gpio_init.Pull = GPIO_PULLUP;
	wwvb_gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	wwvb_gpio_init.Alternate = 0x0;
	HAL_GPIO_Init(WWVB_Pins[pin].GPIO_Group , &wwvb_gpio_init);	
}


void init_sram2_nocache() {
  Serial.println("Disabling data caching!");
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

  /**** Proper API 
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
  ****/

  /**** Hack API, should work??? ****/
  // Disable SPI DMA TX request
  ATOMIC_CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN); 
  ATOMIC_CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN); 
  // disable SPI interrupts
  __HAL_SPI_DISABLE_IT(hspi, (SPI_IT_OVR | SPI_IT_FRE | SPI_IT_MODF));

  // hspi->State = HAL_SPI_STATE_READY; Should I change the state???? For now no

  return HAL_OK;
}
HAL_StatusTypeDef HAL_SPI_DMAResume_Fix(SPI_HandleTypeDef *hspi)
{ // Not sure about this
  HAL_SPI_StateTypeDef State = hspi->State;
  /*
  if ( State == HAL_SPI_STATE_BUSY_TX_RX || State == HAL_SPI_STATE_BUSY_TX) {
    // Enable SPI DMA TX request
    ATOMIC_SET_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN);    
  }
  if ( State == HAL_SPI_STATE_BUSY_TX_RX || State == HAL_SPI_STATE_BUSY_RX) {
    // Enable SPI DMA RX request
    ATOMIC_SET_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN);    
  }
  */
  ATOMIC_SET_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN); 
  ATOMIC_SET_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN);
  // enable SPI interrupts
  //__HAL_SPI_ENABLE_IT(hspi, (SPI_IT_OVR | SPI_IT_FRE | SPI_IT_MODF));
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

  //Serial.println("Enabling DMA TC interrupt for MDMA");
  //((DMA_Stream_TypeDef   *)hspi->hdmarx->Instance)->CR |= DMA_IT_TC;

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


// Function to convert a 32-bit integer from host to network byte order
uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0x000000FF) << 24) |
           ((hostlong & 0x0000FF00) << 8)  |
           ((hostlong & 0x00FF0000) >> 8)  |
           ((hostlong & 0xFF000000) >> 24);
}






