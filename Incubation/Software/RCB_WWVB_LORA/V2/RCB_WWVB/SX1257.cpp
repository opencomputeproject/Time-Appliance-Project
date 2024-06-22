#include "SX1257.h"


#define REGMODE 0x0
#define TX_PA_DRIVER_ENABLE (1<<3)
#define TX_FRONTEND_PLL_ENABLE (1<<2)
#define RX_FRONTEND_PLL_ENABLE (1<<1)
#define RX_STANDBY_ENABLE (1)

#define REG_FRF_RX_MSB 0x1
#define REG_FRF_RX_MID 0x2
#define REG_FRF_RX_LSB 0x3
#define REG_FRF_TX_MSB 0x4
#define REG_FRF_TX_MID 0x5
#define REG_FRF_TX_LSB 0x6

#define REG_TX_GAIN 0x8
#define TX_GAIN_NEG9DB 0x0
#define TX_GAIN_NEG6DB 0x1
#define TX_GAIN_NEG3DB 0x2
#define TX_GAIN_MAX 0x3
#define TX_GAIN_SHIFT 4
#define TX_MIXER_GAIN_MASK 0xF

#define REG_TX_BW 0xA
#define TX_PLL_BW_75K 0x0
#define TX_PLL_BW_150K 0x1
#define TX_PLL_BW_225K 0x2
#define TX_PLL_BW_300K 0x3
#define TX_PLL_BW_SHIFT 5
#define TX_ANAFILT_BW_MASK 0x1f // check datasheet for formula

#define REG_TX_DAC_BW 0xB
#define TX_DAC_FIR_TAP_CNT_MASK 0x7

#define REG_RX_ANA_GAIN 0xC
#define RX_LNA_GAIN_SHIFT 5
#define RX_LNA_GAIN_MASK 0x7
#define RX_LNA_BASEBAND_GAIN_SHIFT 2
#define RX_LNA_BASEBAND_GAIN_MASK 0x7
#define RX_LNA_IMPEDANCE_50 (0<<0)

#define REG_RX_BW 0xD
#define RX_ADC_BW_100K_200K 0x2
#define RX_ADC_BW_200K_400K 0x5
#define RX_ADC_BW_GT400K 0x7
#define RX_ADC_BW_SHIFT 5
#define RX_ADC_BW_MASK 0x7
#define RX_ADC_TRIM_32M 0x6
#define RX_ADC_TRIM_36M 0x5
#define RX_ADC_TRIM_SHIFT 2
#define RX_ADC_TRIM_MASK 0x7
#define RX_ANALOG_FILT_BW_750K 0x0
#define RX_ANALOG_FILT_BW_500K 0x1
#define RX_ANALOG_FILT_BW_375K 0x2
#define RX_ANALOG_FILT_BW_250K 0x3


#define REG_RX_PLL_BW 0xE
#define RX_PLL_BW_75K 0x0
#define RX_PLL_BW_150K 0x1
#define RX_PLL_BW_225K 0x2
#define RX_PLL_BW_300K 0x3
#define RX_PLL_BW_SHIFT 1
#define RX_PLL_BW_MASK 0x3
#define RX_ADC_TEMP_EN 1

#define REG_DIO_MAPPING 0xF

#define REG_CLK_SELECT 0x10
#define DIG_LOOPBACK (1<<3)
#define RF_LOOPBACK (1<<2)
#define CLK_OUT_ENABLE (1<<1)
#define TX_DAC_CLK_SEL_XTAL (0<<0)
#define TX_DAC_CLK_SEL_CLK_IN (1<<0)

#define REG_MODE_STATUS 0x11
#define RX_PLL_LOCK (1<<1)
#define TX_PLL_LOCK (1<<0)

#define REG_LOW_BAT_THRESH 0x1A // useless for me





// C++ -> C world pointers, annoying but HAL being in C needs it
// DONT USE ANY C++ THINGS IN THESE EXTERN C BLOCKS
// make bridge pointers / structures as needed
SDR_stats * my_stats = &SX1257_SDR.sx1257_stats;
SPI_HandleTypeDef * spi1_handler = &SX1257_SDR._spi_I_Data;
DMA_HandleTypeDef * spi1_rx_dma_handler = &SX1257_SDR.hdma_spi1_rx;
DMA_HandleTypeDef * spi1_tx_dma_handler = &SX1257_SDR.hdma_spi1_tx;

SPI_HandleTypeDef * spi2_handler = &SX1257_SDR._spi_Q_Data;
DMA_HandleTypeDef * spi2_rx_dma_handler = &SX1257_SDR.hdma_spi2_rx;
DMA_HandleTypeDef * spi2_tx_dma_handler = &SX1257_SDR.hdma_spi2_tx;
extern "C" {

  // mandatory IRQ handlers to clear interrupts and keep things running
  void SPI1_IRQHandler(void) {
    // normal HAL_SPI_IRQHandler does not work for circular mode!
    my_stats->spi_I_irq_counter++;
    //HAL_SPI_IRQHandler_CircFix(spi1_handler); 
    HAL_SPI_IRQHandler(spi1_handler);
  }
  void SPI2_IRQHandler(void) {
    // normal HAL_SPI_IRQHandler does not work for circular mode!
    my_stats->spi_Q_irq_counter++;
    //HAL_SPI_IRQHandler_CircFix(spi2_handler); 
    HAL_SPI_IRQHandler(spi2_handler);
  }
  void SX1257_I_RX_DMA_STREAM_HANDLER(void) { 
    my_stats->spi_I_RX_DMA_IRQHandler_counter++;   
    HAL_DMA_IRQHandler(spi1_rx_dma_handler);
  }
  void SX1257_Q_RX_DMA_STREAM_HANDLER(void) {
    HAL_DMA_IRQHandler(spi2_rx_dma_handler);
  }
  void SX1257_I_TX_DMA_STREAM_HANDLER(void) {
    my_stats->spi_I_TX_DMA_IRQHandler_counter++;
    HAL_DMA_IRQHandler(spi1_tx_dma_handler);
  }
  void SX1257_Q_TX_DMA_STREAM_HANDLER(void) {
    HAL_DMA_IRQHandler(spi2_tx_dma_handler);
  }

  // User callbacks to handle events
  void SPI_DMAError(DMA_HandleTypeDef *hdma) {   
    my_stats->SPI_DMAError_run = 1; 
  }
  void SPI_DMAAbort(DMA_HandleTypeDef *hdma) {    
    my_stats->SPI_DMAAbort_run = 1;
  }

  void SPI_DMAReceiveCplt(DMA_HandleTypeDef *hdma) {
    my_stats->SPI_DMAReceiveCplt_run = 1;
    if ( hdma == spi1_rx_dma_handler ) {
      my_stats->spi_I_RX_DMAComplete_counter++;
    } 
    else if ( hdma == spi2_rx_dma_handler) {
      my_stats->spi_Q_RX_DMAComplete_counter++;
    }
  }
  void SPI_DMAHalfReceiveCplt(DMA_HandleTypeDef *hdma) {  
    my_stats->SPI_DMAHalfReceiveCplt_run = 1;
    if ( hdma == spi1_rx_dma_handler ) {
      my_stats->spi_I_RX_DMAHalfComplete_counter++;
    } 
    else if ( hdma == spi2_rx_dma_handler) {
      my_stats->spi_Q_RX_DMAHalfComplete_counter++;
    }
  }
  void SPI_DMATransmitCplt(DMA_HandleTypeDef *hdma) {
    if ( hdma == spi1_tx_dma_handler ) {
      my_stats->spi_I_TX_DMAComplete_counter++;
    } 
    else if ( hdma == spi2_tx_dma_handler) {
      my_stats->spi_Q_TX_DMAComplete_counter++;
    }
  }
  void SPI_DMAHalfTransmitCplt(DMA_HandleTypeDef *hdma) {   
    if ( hdma == spi1_tx_dma_handler ) {
      my_stats->spi_I_TX_DMAHalfComplete_counter++;
    } 
    else if ( hdma == spi2_tx_dma_handler) {
      my_stats->spi_Q_TX_DMAHalfComplete_counter++;
    }
  }

  void HAL_DMA_ErrorCallback(DMA_HandleTypeDef *hdma) {
    // DMA error handling
    my_stats->HAL_DMA_ErrorCallback_run = 1;
  }

  void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    my_stats->HAL_SPI_RxCpltCallback_counter++;
  }
  void HAL_SPI_RxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
    my_stats->HAL_SPI_RxHalfCpltCallback_counter++;
  }
  void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    my_stats->HAL_SPI_ErrorCallback_run = 1;
  }
  void HAL_SPI_SuspendCallback(SPI_HandleTypeDef *hspi) {
    my_stats->HAL_SPI_SuspendCallback_run = 1;
  }

  void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) // called by HAL_SPI_Init
  {
    my_stats->HAL_SPI_MspInit_run = 1;
    if (hspi == spi1_handler ){ 
      // Init the STM32 SPI1 interface
      my_stats->HAL_SPI_MspInit_SPI1_run = 1;
      GPIO_InitTypeDef GPIO_InitStruct = {0};

      // SPI1 SCK Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SX1257_CLK_OUT_SPI1].GPIO_Pin;  
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;  // Alternate function for SPI1
      HAL_GPIO_Init(WWVB_Pins[SX1257_CLK_OUT_SPI1].GPIO_Group, &GPIO_InitStruct);

      // SPI1 MISO Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SX1257_I_IN].GPIO_Pin;  
      HAL_GPIO_Init(WWVB_Pins[SX1257_I_IN].GPIO_Group, &GPIO_InitStruct);

      // SPI1 MOSI Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SX1257_I_OUT].GPIO_Pin; 
      HAL_GPIO_Init(WWVB_Pins[SX1257_I_OUT].GPIO_Group, &GPIO_InitStruct);


      // I , RX DMA , NOT USING AS NORMAL SPI DMA!!!!
      spi1_rx_dma_handler->Instance = SX1257_I_RX_DMA_STREAM; // Example stream, adjust as needed
      spi1_rx_dma_handler->Init.Request = DMA_REQUEST_SPI1_RX; // Make sure to use the correct request number
      spi1_rx_dma_handler->Init.Direction = DMA_PERIPH_TO_MEMORY;
      spi1_rx_dma_handler->Init.PeriphInc = DMA_PINC_DISABLE;
      spi1_rx_dma_handler->Init.MemInc = DMA_MINC_ENABLE;
      spi1_rx_dma_handler->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
      spi1_rx_dma_handler->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD; 
      spi1_rx_dma_handler->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
      spi1_rx_dma_handler->Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
      //spi1_rx_dma_handler->Init.MemBurst = DMA_MBURST_INC4; 
      spi1_rx_dma_handler->Init.Mode = DMA_CIRCULAR; // Or DMA_CIRCULAR for continuous reception
      //spi1_rx_dma_handler->Init.Mode = DMA_NORMAL; // HACK NON CIRCULAR
      spi1_rx_dma_handler->Init.Priority = DMA_PRIORITY_HIGH;

      HAL_DMA_Init(spi1_rx_dma_handler);

      // __HAL_DMA_LINK(spi1_handler, spi1_rx_dma_handler)
      spi1_handler->hdmarx = spi1_rx_dma_handler; 
      spi1_rx_dma_handler->Parent = spi1_handler; // Link DMA to SPI1 RX, NEED BOTH 

      //HAL_DMA_RegisterCallback(spi1_rx_dma_handler, HAL_DMA_XFER_CPLT_CB_ID, SPI_DMAReceiveCplt);
      //HAL_DMA_RegisterCallback(spi1_rx_dma_handler, HAL_DMA_XFER_HALFCPLT_CB_ID, SPI_DMAHalfReceiveCplt);
      //HAL_DMA_RegisterCallback(spi1_rx_dma_handler, HAL_DMA_XFER_ERROR_CB_ID, SPI_DMAError);
      //HAL_DMA_RegisterCallback(spi1_rx_dma_handler, HAL_DMA_XFER_ABORT_CB_ID, SPI_DMAAbort);
      //HAL_NVIC_SetPriority(SX1257_I_RX_DMA_STREAM_IRQ, 1, 0);
      //HAL_NVIC_EnableIRQ(SX1257_I_RX_DMA_STREAM_IRQ); 

      // I , TX DMA
      spi1_tx_dma_handler->Instance = SX1257_I_TX_DMA_STREAM; // Example stream, adjust as needed
      spi1_tx_dma_handler->Init.Request = DMA_REQUEST_SPI1_TX; // Make sure to use the correct request number
      spi1_tx_dma_handler->Init.Direction = DMA_MEMORY_TO_PERIPH;
      spi1_tx_dma_handler->Init.PeriphInc = DMA_PINC_DISABLE;
      spi1_tx_dma_handler->Init.MemInc = DMA_MINC_ENABLE;
      spi1_tx_dma_handler->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
      spi1_tx_dma_handler->Init.MemDataAlignment = DMA_PDATAALIGN_HALFWORD;
      spi1_tx_dma_handler->Init.Mode = DMA_CIRCULAR; // Or DMA_CIRCULAR for continuous reception
      spi1_tx_dma_handler->Init.Priority = DMA_PRIORITY_HIGH;

      HAL_DMA_Init(spi1_tx_dma_handler);

      spi1_handler->hdmatx = spi1_tx_dma_handler;
      spi1_tx_dma_handler->Parent = spi1_handler;

      /* Data rate is very high, too many interrupts, don't use interrupts
      HAL_DMA_RegisterCallback(&SX1257_SDR.hdma_spi1_tx, HAL_DMA_XFER_CPLT_CB_ID, SPI_DMATransmitCplt);
      HAL_DMA_RegisterCallback(&SX1257_SDR.hdma_spi1_tx, HAL_DMA_XFER_HALFCPLT_CB_ID, SPI_DMAHalfTransmitCplt);
      HAL_DMA_RegisterCallback(&SX1257_SDR.hdma_spi1_tx, HAL_DMA_XFER_ERROR_CB_ID, SPI_DMAError);
      HAL_DMA_RegisterCallback(&SX1257_SDR.hdma_spi1_tx, HAL_DMA_XFER_ABORT_CB_ID, SPI_DMAAbort);
      HAL_NVIC_SetPriority(SX1257_I_TX_DMA_STREAM_IRQ, 9, 0);
      HAL_NVIC_EnableIRQ(SX1257_I_TX_DMA_STREAM_IRQ);   
      */

      //HAL_NVIC_SetPriority(SPI1_IRQn, 1, 0);
      //HAL_NVIC_EnableIRQ(SPI1_IRQn);  

      
    } 
    else if (hspi == spi2_handler  )
    {
      //Serial.println("HAL SPI MSPINIT for Q Interface start");  
      // Init the STM32 SPI1 interface
      GPIO_InitTypeDef GPIO_InitStruct = {0};

      // SPI1 SCK Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SX1257_CLK_OUT_SPI2].GPIO_Pin;  
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;  // Alternate function for SPI2
      HAL_GPIO_Init(WWVB_Pins[SX1257_CLK_OUT_SPI2].GPIO_Group, &GPIO_InitStruct);

      // SPI1 MISO Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SX1257_Q_IN].GPIO_Pin;  
      HAL_GPIO_Init(WWVB_Pins[SX1257_Q_IN].GPIO_Group, &GPIO_InitStruct);

      // SPI1 MOSI Pin Configuration
      GPIO_InitStruct.Pin = WWVB_Pins[SX1257_Q_OUT].GPIO_Pin; 
      HAL_GPIO_Init(WWVB_Pins[SX1257_Q_OUT].GPIO_Group, &GPIO_InitStruct);


      // I , RX DMA
      spi2_rx_dma_handler->Instance = SX1257_Q_RX_DMA_STREAM; // Example stream, adjust as needed
      spi2_rx_dma_handler->Init.Request = DMA_REQUEST_SPI2_RX; // Make sure to use the correct request number
      spi2_rx_dma_handler->Init.Direction = DMA_PERIPH_TO_MEMORY;
      spi2_rx_dma_handler->Init.PeriphInc = DMA_PINC_DISABLE;
      spi2_rx_dma_handler->Init.MemInc = DMA_MINC_ENABLE;
      spi2_rx_dma_handler->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
      spi2_rx_dma_handler->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
      spi2_rx_dma_handler->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
      spi2_rx_dma_handler->Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
      spi2_rx_dma_handler->Init.Mode = DMA_CIRCULAR; // Or DMA_CIRCULAR for continuous reception
      spi2_rx_dma_handler->Init.Priority = DMA_PRIORITY_HIGH;

      HAL_DMA_Init(spi2_rx_dma_handler);

      spi2_handler->hdmarx = spi2_rx_dma_handler; 
      spi2_rx_dma_handler->Parent = spi2_handler; // Link DMA to SPI2 RX , NEED BOTH, SAME AS __HAL_DMA_LINK

      /* Data rate is very high, too many interrupts, don't use interrupts
      HAL_DMA_RegisterCallback(spi2_rx_dma_handler, HAL_DMA_XFER_CPLT_CB_ID, SPI_DMAReceiveCplt);
      HAL_DMA_RegisterCallback(spi2_rx_dma_handler, HAL_DMA_XFER_HALFCPLT_CB_ID, SPI_DMAHalfReceiveCplt);
      HAL_DMA_RegisterCallback(spi2_rx_dma_handler, HAL_DMA_XFER_ERROR_CB_ID, SPI_DMAError);
      HAL_DMA_RegisterCallback(spi2_rx_dma_handler, HAL_DMA_XFER_ABORT_CB_ID, SPI_DMAAbort);
      HAL_NVIC_SetPriority(SX1257_Q_RX_DMA_STREAM_IRQ, 9, 0);
      HAL_NVIC_EnableIRQ(SX1257_Q_RX_DMA_STREAM_IRQ); 
      */

      // I , TX DMA
      spi2_tx_dma_handler->Instance = SX1257_Q_TX_DMA_STREAM; // Example stream, adjust as needed
      spi2_tx_dma_handler->Init.Request = DMA_REQUEST_SPI2_TX; // Make sure to use the correct request number
      spi2_tx_dma_handler->Init.Direction = DMA_MEMORY_TO_PERIPH;
      spi2_tx_dma_handler->Init.PeriphInc = DMA_PINC_DISABLE;
      spi2_tx_dma_handler->Init.MemInc = DMA_MINC_ENABLE;
      spi2_tx_dma_handler->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
      spi2_tx_dma_handler->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
      spi2_tx_dma_handler->Init.Mode = DMA_CIRCULAR; // Or DMA_CIRCULAR for continuous reception
      spi2_tx_dma_handler->Init.Priority = DMA_PRIORITY_HIGH;

      HAL_DMA_Init(spi2_tx_dma_handler);
      spi2_handler->hdmatx = spi2_tx_dma_handler;
      spi2_tx_dma_handler->Parent = spi2_handler;

      /* Data rate is very high, too many interrupts, don't use interrupts
      HAL_DMA_RegisterCallback(&SX1257_SDR.hdma_spi2_tx, HAL_DMA_XFER_CPLT_CB_ID, SPI_DMATransmitCplt);
      HAL_DMA_RegisterCallback(&SX1257_SDR.hdma_spi2_tx, HAL_DMA_XFER_HALFCPLT_CB_ID, SPI_DMAHalfTransmitCplt);
      HAL_DMA_RegisterCallback(&SX1257_SDR.hdma_spi2_tx, HAL_DMA_XFER_ERROR_CB_ID, SPI_DMAError);
      HAL_DMA_RegisterCallback(&SX1257_SDR.hdma_spi2_tx, HAL_DMA_XFER_ABORT_CB_ID, SPI_DMAAbort);
      HAL_NVIC_SetPriority(SX1257_Q_TX_DMA_STREAM_IRQ, 9, 0);
      HAL_NVIC_EnableIRQ(SX1257_Q_TX_DMA_STREAM_IRQ); 
      HAL_NVIC_SetPriority(SPI2_IRQn, 10, 0);
      HAL_NVIC_EnableIRQ(SPI2_IRQn);  
      */

    }
  }
}








SX1257Class::SX1257Class()
{
  _ss = SX1257_NSS;
  _reset = SX1257_RST;


  sx1257_stats.spi_I_irq_counter = 0;
  sx1257_stats.spi_I_RX_DMA_IRQHandler_counter = 0;
  sx1257_stats.spi_I_RX_DMAHalfComplete_counter = 0;
  sx1257_stats.spi_I_RX_DMAComplete_counter = 0;
  sx1257_stats.spi_I_TX_DMA_IRQHandler_counter = 0;
  sx1257_stats.spi_I_TX_DMAHalfComplete_counter = 0;
  sx1257_stats.spi_I_TX_DMAComplete_counter = 0;

  sx1257_stats.spi_Q_irq_counter = 0;
  sx1257_stats.spi_Q_RX_DMAHalfComplete_counter = 0;
  sx1257_stats.spi_Q_RX_DMAComplete_counter = 0;
  sx1257_stats.spi_Q_TX_DMAHalfComplete_counter = 0;
  sx1257_stats.spi_Q_TX_DMAComplete_counter = 0;

  sx1257_stats.HAL_SPI_RxCpltCallback_counter = 0;
  sx1257_stats.HAL_SPI_RxHalfCpltCallback_counter = 0;
  sx1257_stats.HAL_SPI_ErrorCallback_run = 0;
  sx1257_stats.HAL_SPI_MspInit_run = 0;
  sx1257_stats.HAL_SPI_MspInit_SPI1_run = 0;
  sx1257_stats.HAL_DMA_ErrorCallback_run = 0;
  sx1257_stats.SPI_DMAError_run = 0;
  sx1257_stats.SPI_DMAAbort_run = 0;
  sx1257_stats.SPI_DMAReceiveCplt_run = 0;
  sx1257_stats.SPI_DMAHalfReceiveCplt_run = 0;
  sx1257_stats.HAL_SPI_SuspendCallback_run = 0;

  rx_dma_state = DMA_STOPPED;
  tx_dma_state = DMA_STOPPED;



}



int SX1257Class::init(bool first) {

    if ( first ) {

    //I_rxBuffer = (uint32_t*)D2_AHBSRAM_BASE;
    //I_txBuffer = I_rxBuffer + IQ_BUFFER_SIZE;
    //Q_rxBuffer = I_txBuffer + IQ_BUFFER_SIZE;
    //Q_txBuffer = Q_rxBuffer + IQ_BUFFER_SIZE;

    // Init the GPIOs used just for control
    wwvb_gpio_pinmode(SX1257_NSS, OUTPUT);
    wwvb_gpio_pinmode(SX1257_RST, OUTPUT);  

    wwvb_digital_write(SX1257_NSS, 1);
    wwvb_digital_write(SX1257_RST, 0); // RESET IS ACTIVE HIGH


    // Init the STM32 SPI6 interface
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // SPI6 SCK Pin Configuration
    GPIO_InitStruct.Pin = WWVB_Pins[SX1257_SCK].GPIO_Pin;  
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI6;  // Alternate function for SPI6
    HAL_GPIO_DeInit(WWVB_Pins[SX1257_SCK].GPIO_Group, GPIO_InitStruct.Pin); // Deinit, previously used as GPIO
    HAL_GPIO_Init(WWVB_Pins[SX1257_SCK].GPIO_Group, &GPIO_InitStruct);

    // SPI6 MISO Pin Configuration
    GPIO_InitStruct.Pin = WWVB_Pins[SX1257_MISO].GPIO_Pin; 
    HAL_GPIO_DeInit(WWVB_Pins[SX1257_MISO].GPIO_Group, GPIO_InitStruct.Pin); // Deinit, previously used as GPIO
    HAL_GPIO_Init(WWVB_Pins[SX1257_MISO].GPIO_Group, &GPIO_InitStruct);

    // SPI6 MOSI Pin Configuration
    GPIO_InitStruct.Pin = WWVB_Pins[SX1257_MOSI].GPIO_Pin; 
    GPIO_InitStruct.Alternate = GPIO_AF8_SPI6; // this pin uses AF8 for some reason
    HAL_GPIO_DeInit(WWVB_Pins[SX1257_MOSI].GPIO_Group, GPIO_InitStruct.Pin); // Deinit, previously used as GPIO
    HAL_GPIO_Init(WWVB_Pins[SX1257_MOSI].GPIO_Group, &GPIO_InitStruct);

    _spi_mgmt.Instance = SPI6;
    _spi_mgmt.Init.Mode = SPI_MODE_MASTER;  // SPI mode (Master/Slave)
    _spi_mgmt.Init.Direction = SPI_DIRECTION_2LINES;  // Full duplex mode
    _spi_mgmt.Init.DataSize = SPI_DATASIZE_8BIT;  // 8-bit data frame format
    _spi_mgmt.Init.CLKPolarity = SPI_POLARITY_LOW;  // Clock polarity, V1 code was LOW!!!
    _spi_mgmt.Init.CLKPhase = SPI_PHASE_1EDGE;  // Clock phase , V1 code was 1EDGE!!!
    _spi_mgmt.Init.NSS = SPI_NSS_SOFT;  // NSS signal is managed by software
    _spi_mgmt.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    _spi_mgmt.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;  // Baud rate prescaler
    _spi_mgmt.Init.FirstBit = SPI_FIRSTBIT_MSB;  // Data is transmitted MSB first
    _spi_mgmt.Init.TIMode = SPI_TIMODE_DISABLE;  // Disable TI mode
    _spi_mgmt.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Disable CRC calculation
    _spi_mgmt.Init.CRCPolynomial = 7;  // Polynomial for CRC calculation

    if ( HAL_SPI_Init(&_spi_mgmt) != HAL_OK ) {
      Serial.println("FAILED TO INIT SX1257 MANAGEMENT SPI");
      return -1;
    }
    _spi_mgmt.Instance->CFG2 |= SPI_CFG2_AFCNTR; // hack, keep it as SPI mode always




    // Init the STM32 SPI1 interface, I Data
    _spi_I_Data.Instance = SPI1;
    _spi_I_Data.Init.Mode = SPI_MODE_SLAVE;  // SPI mode (Master/Slave)
    _spi_I_Data.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;  // different speed limits
    _spi_I_Data.Init.DataSize = SPI_DATASIZE_16BIT;  // data frame format, int16t from FPGA on V2 board
    _spi_I_Data.Init.CLKPolarity = SPI_POLARITY_LOW;  // Clock polarity CPOL
    _spi_I_Data.Init.CLKPhase = SPI_PHASE_2EDGE;  // Clock phase CPHA
    _spi_I_Data.Init.NSS = SPI_NSS_SOFT;  // NSS signal is managed by software
    _spi_I_Data.Init.NSSPolarity = SPI_NSS_POLARITY_HIGH;
    _spi_I_Data.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  // Baud rate prescaler
    _spi_I_Data.Init.FirstBit = SPI_FIRSTBIT_LSB;  //
    _spi_I_Data.Init.TIMode = SPI_TIMODE_DISABLE;  // Disable TI mode
    _spi_I_Data.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Disable CRC calculation
    _spi_I_Data.Init.CRCPolynomial = 7;  // Polynomial for CRC calculation
    //_spi_I_Data.Init.IOSwap = SPI_IO_SWAP_ENABLE; // messed up on board, swap MISO and MOSI with respect to STM32 -> V1 board, V2 doesn't need
    _spi_I_Data.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    _spi_I_Data.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    _spi_I_Data.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    _spi_I_Data.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;

    if ( HAL_SPI_Init(&_spi_I_Data) != HAL_OK ) { // Calls MSP Init
      Serial.println("FAILED TO INIT SX1257 I Data SPI");
      return -1;
    }
    _spi_I_Data.Instance->CFG2 |= SPI_CFG2_AFCNTR; // hack, keep it as SPI mode always

    // Init the STM32 SPI2 interface, Q Data
    _spi_Q_Data.Instance = SPI2;
    _spi_Q_Data.Init.Mode = SPI_MODE_SLAVE;  // SPI mode (Master/Slave)
    _spi_Q_Data.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;  // RX only mode, different speed limits
    _spi_Q_Data.Init.DataSize = SPI_DATASIZE_16BIT;  // data frame format, int16t from FPGA on V2 board
    _spi_Q_Data.Init.CLKPolarity = SPI_POLARITY_LOW;  // Clock polarity, CPOL
    _spi_Q_Data.Init.CLKPhase = SPI_PHASE_2EDGE;  // Clock phase, CPHA
    _spi_Q_Data.Init.NSS = SPI_NSS_SOFT;  // NSS signal is managed by software
    _spi_Q_Data.Init.NSSPolarity = SPI_NSS_POLARITY_HIGH;
    _spi_Q_Data.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  // Baud rate prescaler
    _spi_Q_Data.Init.FirstBit = SPI_FIRSTBIT_LSB;  // 
    _spi_Q_Data.Init.TIMode = SPI_TIMODE_DISABLE;  // Disable TI mode
    _spi_Q_Data.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Disable CRC calculation
    _spi_Q_Data.Init.CRCPolynomial = 7;  // Polynomial for CRC calculation
    //_spi_Q_Data.Init.IOSwap = SPI_IO_SWAP_ENABLE; // messed up on board, swap MISO and MOSI with respect to STM32 -> V1 board, V2 doesn't need
    _spi_Q_Data.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    _spi_Q_Data.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    _spi_Q_Data.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    _spi_Q_Data.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;

    if ( HAL_SPI_Init(&_spi_Q_Data) != HAL_OK ) {
      Serial.println("FAILED TO INIT SX1257 I Data SPI");
      return -1;
    }
    _spi_Q_Data.Instance->CFG2 |= SPI_CFG2_AFCNTR; // hack, keep it as SPI mode always
    __DSB();
    __ISB();
    __DMB(); // just to guarantee what I read back next is up to date

    
    Serial.println("******************* DEBUG SPI CONFIG , SPI_I_DATA *****************");
    Serial.print("SPI_CR1: 0x");
    Serial.print(_spi_I_Data.Instance->CR1, HEX);
    Serial.print(" , SPI_CR2: 0x");
    Serial.print(_spi_I_Data.Instance->CR2, HEX);
    Serial.print(" , SPI_CFG1: 0x");
    Serial.print(_spi_I_Data.Instance->CFG1, HEX);
    Serial.print(" , SPI_CFG2: 0x");
    Serial.print(_spi_I_Data.Instance->CFG2, HEX);
    Serial.print(" , SPI_I2SCFGR: 0x");
    Serial.println(_spi_I_Data.Instance->I2SCFGR, HEX);
    


  }

  // toggle the reset
  wwvb_digital_write(SX1257_RST, 1);
  delay(10);
  wwvb_digital_write(SX1257_RST, 0); 
  delay(10);


  Serial.println("SX1257 init successful!");

  // write some basic registers that should never change for current board design
  writeRegister(REG_RX_ANA_GAIN, 0x3c); // 50 ohm LNA input impedance is key setting
  writeRegister(REG_RX_BW, (RX_ADC_BW_GT400K << RX_ADC_BW_SHIFT) + 
      (RX_ADC_TRIM_32M << RX_ADC_TRIM_SHIFT) + RX_ANALOG_FILT_BW_500K ); //32MHz is key, that's what PLL is giving
  //writeRegister(REG_CLK_SELECT, DIG_LOOPBACK + CLK_OUT_ENABLE + TX_DAC_CLK_SEL_XTAL); // Need clock out and crystal path for TX
  //Serial.println("********************* HACK ENABLE DIGITAL LOOPBACK **************************");

  writeRegister(REG_CLK_SELECT, CLK_OUT_ENABLE + TX_DAC_CLK_SEL_XTAL); // Need clock out and crystal path for TX
  
  dumpRegisters(Serial);
  // Hacking it, using digital loopback, if I write 1, i should see 1, and vice versa
  // just use as GPIOs
  /*
  wwvb_gpio_pinmode(SX1257_I_IN, OUTPUT);
  wwvb_gpio_pinmode(SX1257_I_OUT, INPUT);
  wwvb_gpio_pinmode(SX1257_Q_IN, OUTPUT);
  wwvb_gpio_pinmode(SX1257_Q_OUT, INPUT);
  */

  isTransmitting = 0;
  isReceiving = 0;

  //dumpRegisters(Serial);

  wwvb_gpio_pinmode(SDR_TX_RX_SEL, OUTPUT);
  set_antenna(0);
  return 0;
}


void SX1257Class::debug() {
  uint8_t val = 0;
  writeRegister(REGMODE, 0xf);
  val = readRegister(REGMODE);
  Serial.print("SX1257 debug mode = 0x");
  Serial.println(val, HEX);
}


int SX1257Class::set_tx_mode(bool tx_frontend_pll, bool tx_pa_driver) {
  uint8_t cur_reg = 0;
  cur_reg = readRegister(REGMODE);

  Serial.print("SX1257 Set TX frontend_pll=");
  Serial.print(tx_frontend_pll);
  Serial.print(" , pa_driver=");
  Serial.print(tx_pa_driver);
  Serial.print(" , cur mode = 0x");
  Serial.println(cur_reg, HEX);

  if ( tx_frontend_pll ) cur_reg |= TX_FRONTEND_PLL_ENABLE;
  else cur_reg &= (~TX_FRONTEND_PLL_ENABLE);

  if ( tx_pa_driver ) cur_reg |= TX_PA_DRIVER_ENABLE;
  else cur_reg &= (~TX_PA_DRIVER_ENABLE);

  writeRegister(REGMODE, cur_reg);
  return 0;

}
int SX1257Class::set_rx_mode(bool rx_frontend_pll, bool standby) {
  uint8_t cur_reg = 0;
  cur_reg = readRegister(REGMODE);

  Serial.print("SX1257 Set RX frontend_pll=");
  Serial.print(rx_frontend_pll);
  Serial.print(" , standby_mode=");
  Serial.println(standby);
  Serial.println("");
  Serial.println("");

  if ( rx_frontend_pll ) cur_reg |= RX_FRONTEND_PLL_ENABLE;
  else cur_reg &= (~RX_FRONTEND_PLL_ENABLE);

  if ( standby ) cur_reg |= RX_STANDBY_ENABLE;
  else cur_reg &= (~RX_STANDBY_ENABLE);

  writeRegister(REGMODE, cur_reg);
  return 0;
}


// frequency calculation
// 36MHz = frequency resolution = 68.66455 Hz, 0xCB5555 = 915MHz
int SX1257Class::set_tx_freq(long frequency) {
  _txfrequency = frequency;

  //uint32_t frf = (uint32_t)( ((float)frequency) / 68.66455);
  uint32_t frf = (uint32_t)( ((float)frequency) / 61.035156); // 32MHz
  Serial.print("SX1257 Set TX Frequency 0x");
  Serial.println(frf, HEX);

  writeRegister(REG_FRF_TX_MSB, (uint8_t)(frf >> 16));
  writeRegister(REG_FRF_TX_MID, (uint8_t)(frf >> 8));
  writeRegister(REG_FRF_TX_LSB, (uint8_t)(frf >> 0));
  return 0;
}

int SX1257Class::set_rx_freq(long frequency) {
  _rxfrequency = frequency;
  
  //uint32_t frf = (uint32_t)( ((float)frequency) / 68.66455);
  uint32_t frf = (uint32_t)( ((float)frequency) / 61.035156); // 32MHz
  Serial.print("SX1257 Set RX Frequency 0x");
  Serial.println(frf, HEX);

  writeRegister(REG_FRF_RX_MSB, (uint8_t)(frf >> 16));
  writeRegister(REG_FRF_RX_MID, (uint8_t)(frf >> 8));
  writeRegister(REG_FRF_RX_LSB, (uint8_t)(frf >> 0));
  return 0;
}


int SX1257Class::set_tx_parameters(uint8_t dac_gain, uint8_t mixer_gain,
  uint8_t pll_bw, uint8_t analog_filter_bw,
  uint8_t tx_fir_ntaps) {

  writeRegister(REG_TX_GAIN, ( (dac_gain & TX_GAIN_MAX) << TX_GAIN_SHIFT) +
    (mixer_gain & TX_MIXER_GAIN_MASK) );
  writeRegister(REG_TX_BW, ( (pll_bw & TX_PLL_BW_300K) << TX_PLL_BW_SHIFT) +
    (analog_filter_bw & TX_ANAFILT_BW_MASK) );
  writeRegister(REG_TX_DAC_BW, tx_fir_ntaps & TX_DAC_FIR_TAP_CNT_MASK);

  return 0;
}

int SX1257Class::set_rx_parameters(uint8_t lna_gain, uint8_t baseband_gain,
  uint8_t adc_bw, uint8_t analog_filter_bw, 
  uint8_t pll_bw) {

  writeRegister(REG_RX_ANA_GAIN, ((lna_gain & RX_LNA_GAIN_MASK) << RX_LNA_GAIN_SHIFT) + 
    ((baseband_gain & RX_LNA_BASEBAND_GAIN_MASK) << RX_LNA_BASEBAND_GAIN_SHIFT) + RX_LNA_IMPEDANCE_50);
  
  writeRegister(REG_RX_BW, ((adc_bw & RX_ADC_BW_MASK) << RX_ADC_BW_SHIFT) + 
    (RX_ADC_TRIM_32M << RX_ADC_TRIM_SHIFT) + (analog_filter_bw & RX_ANALOG_FILT_BW_250K)  );

  writeRegister(REG_RX_PLL_BW, ((pll_bw & RX_PLL_BW_MASK) << RX_PLL_BW_SHIFT) ); // always disable temperature

}

int SX1257Class::get_status(bool * rx_pll_lock, bool * tx_pll_lock) {
  uint8_t val = 0;
  val = readRegister(REG_MODE_STATUS);
  if ( rx_pll_lock ) {
    if ( val & RX_PLL_LOCK ) {
      *rx_pll_lock = 1;
    } else {
      *rx_pll_lock = 0;
    }
  }
  if ( tx_pll_lock ){
    if ( val & TX_PLL_LOCK ) {
      *tx_pll_lock = 1;
    } else {
      *rx_pll_lock = 0;
    }
  }
  return 0;
}

void SX1257Class::dumpRegisters(arduino::Stream& out) {
  for (int i = 0; i < 0x1a; i++) {
    out.print("SX1257 Register 0x");
    out.print(i, HEX);
    out.print(": 0x");
    out.println(readRegister(i), HEX);
  }
}


uint8_t SX1257Class::readRegister(uint8_t address) {
  //Serial.println("SX1257 read register");
  return singleTransfer(address & 0x7f, 0x00);
}
void SX1257Class::writeRegister(uint8_t address, uint8_t value) {
  //Serial.println("SX1257 write register");
  singleTransfer(address | 0x80, value);
}

uint8_t SX1257Class::singleTransfer(uint8_t address, uint8_t value) {

  uint8_t response;
  wwvb_digital_write(SX1257_NSS, LOW);
  delayMicroseconds(10);

  HAL_SPI_TransmitReceive(&_spi_mgmt, &address, &response, sizeof(address), HAL_MAX_DELAY);  //ignore receive data

  HAL_SPI_TransmitReceive(&_spi_mgmt, &value, &response, sizeof(value), HAL_MAX_DELAY);

  delayMicroseconds(10);
  wwvb_digital_write(SX1257_NSS, HIGH);
  delayMicroseconds(10);
  
  /*
  Serial.print("SX1257 Single transfer addr=0x");
  Serial.print(address,HEX);
  Serial.print(", value = 0x");
  Serial.print(value,HEX);
  Serial.print(" , response = 0x");
  Serial.println(response, HEX);
  */

  return response;

}

int SX1257Class::set_antenna(bool tx) {
  if ( tx ) {
    wwvb_digital_write(SDR_TX_RX_SEL, 1); // 1 for output1, TX
  } else {
    wwvb_digital_write(SDR_TX_RX_SEL, 0); // 0 for output2, RX
  }
  return 0;
}


void SX1257Class::write_I(uint8_t val) {
  HAL_SPI_Transmit(&_spi_I_Data, &val, sizeof(val), HAL_MAX_DELAY);
}

void SX1257Class::write_Q(uint8_t val) {
  HAL_SPI_Transmit(&_spi_Q_Data, &val, sizeof(val), HAL_MAX_DELAY);
}



/************* Stream class stuff ******/
size_t SX1257Class::write(uint8_t byte) {
  return 0;
}
size_t SX1257Class::write(const uint8_t *buffer, size_t size) {
  return 0;
}

  // from Stream
int SX1257Class::available() {
  return 0;
}

int SX1257Class::read() {
  return 0;
}
int SX1257Class::peek() {
  return 0;
}
void SX1257Class::flush() {
  return;
}






SX1257Class SX1257_SDR;

