#include "WWVB_Arduino.h"


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



  hspi->State = HAL_SPI_STATE_READY; // set state back to ready
  if ( tx_errorcode != HAL_OK ) {
    return tx_errorcode;
  }
  if ( rx_errorcode != HAL_OK ) {
    return rx_errorcode;
  }
  return HAL_OK;
}
