#ifndef SX1257_H
#define SX1257_H

#include "WWVB_Arduino.h"
#include <stm32h7xx_hal_dma.h>
#include <stm32h7xx_hal_dfsdm.h>
#include <stm32h7xx.h>

/*
void HAL_SPI_RxCpltCallback(struct __DMA_HandleTypeDef * hdma);
void HAL_SPI_RxHalfCpltCallback(struct __DMA_HandleTypeDef * hdma);
void HAL_SPI_ErrorCallback(struct __DMA_HandleTypeDef * hdma);
*/
#define IQ_BUFFER_SIZE 2048
extern ALIGN_32BYTES(volatile uint32_t I_rxBuffer[IQ_BUFFER_SIZE])  __attribute__((section(".RAM_D2")));
extern ALIGN_32BYTES(volatile uint32_t I_txBuffer[IQ_BUFFER_SIZE])  __attribute__((section(".RAM_D2")));
extern ALIGN_32BYTES(volatile uint32_t Q_rxBuffer[IQ_BUFFER_SIZE])  __attribute__((section(".RAM_D2")));
extern ALIGN_32BYTES(volatile uint32_t Q_txBuffer[IQ_BUFFER_SIZE])  __attribute__((section(".RAM_D2")));
//extern uint32_t * I_rxBuffer;
//extern uint32_t * I_txBuffer;
//extern uint32_t * Q_rxBuffer;
//extern uint32_t * Q_txBuffer;


// RAM_D2 defiend in AppData\Local\Arduino15\packages\arduino\hardware\mbed_giga\4.1.1\variants\GIGA\linker_script.ld
/*
  FLASH (rx) : ORIGIN = 0x8040000, LENGTH = CM4_BINARY_START - 0x8040000
  DTCMRAM (rwx) : ORIGIN = 0x20000000 + (((166 * 4) + 7) & 0xFFFFFFF8), LENGTH = 128K - (((166 * 4) + 7) & 0xFFFFFFF8)
  RAM (xrw) : ORIGIN = 0x24000000, LENGTH = 0x80000
  RAM_D2 (xrw) : ORIGIN = 0x30000000, LENGTH = 288K
  RAM_D3 (xrw) : ORIGIN = 0x38000000, LENGTH = 64K
  ITCMRAM (xrw) : ORIGIN = 0x00000000, LENGTH = 64K
*/



extern "C" {
  typedef struct SDR_stats_struct {
    // Debug counters to keep track of IRQs
    uint32_t spi_I_irq_counter;
    uint32_t spi_I_RX_DMA_IRQHandler_counter;
    uint32_t spi_I_RX_DMAHalfComplete_counter;
    uint32_t spi_I_RX_DMAComplete_counter;
    uint32_t spi_I_TX_DMA_IRQHandler_counter;
    uint32_t spi_I_TX_DMAHalfComplete_counter;
    uint32_t spi_I_TX_DMAComplete_counter;

    uint32_t spi_Q_irq_counter;
    uint32_t spi_Q_RX_DMAHalfComplete_counter;
    uint32_t spi_Q_RX_DMAComplete_counter;
    uint32_t spi_Q_TX_DMAHalfComplete_counter;
    uint32_t spi_Q_TX_DMAComplete_counter;

    uint32_t HAL_SPI_RxCpltCallback_counter;    
    uint32_t HAL_SPI_RxHalfCpltCallback_counter;
    
    bool HAL_SPI_ErrorCallback_run;
    bool HAL_SPI_MspInit_run;
    bool HAL_SPI_MspInit_SPI1_run;
    bool HAL_DMA_ErrorCallback_run;
    bool SPI_DMAError_run;
    bool SPI_DMAAbort_run;
    bool SPI_DMAReceiveCplt_run;
    bool SPI_DMAHalfReceiveCplt_run;
    bool HAL_SPI_SuspendCallback_run;
  } SDR_stats;
}


#define DMA_PAUSED 0
#define DMA_RUNNING 1
#define DMA_STOPPED 2


class SX1257Class {
public:
  SX1257Class();

  int init(bool first);

  // basic mode control, top level enable more or less
  int set_tx_mode(bool tx_frontend_pll, bool tx_pa_driver);
  int set_rx_mode(bool rx_frontend_pll, bool standby);

  int set_tx_freq(long frequency); // only applied when exiting sleep mode, aka TX and RX off
  int set_rx_freq(long frequency); // only applied when exiting sleep mode, aka TX and RX off

  int set_tx_parameters(uint8_t dac_gain, uint8_t mixer_gain,
    uint8_t pll_bw, uint8_t analog_filter_bw,
    uint8_t tx_fir_ntaps);

  int set_rx_parameters(uint8_t lna_gain, uint8_t baseband_gain,
    uint8_t adc_bw, uint8_t analog_filter_bw, 
    uint8_t pll_bw);

  int get_status(bool * rx_pll_lock, bool * tx_pll_lock);

  int set_antenna(bool tx);

  int enable_rx_dma();
  void debug_print_rx_dma_registers();
  int disable_dma();

  // from Print
  virtual size_t write(uint8_t byte);
  virtual size_t write(const uint8_t *buffer, size_t size);

  // from Stream
  virtual int available();
  virtual int read();
  virtual int peek();
  virtual void flush();

  /*
  void onReceive(void(*callback)(int));
  void onCadDone(void(*callback)(boolean));
  void onTxDone(void(*callback)());
  void receive(int size = 0);
  */

  void dumpRegisters(arduino::Stream& out);

  void debug();

  void dump_dfsdm_regs();


  void write_I(uint8_t val);
  void write_Q(uint8_t val);



  bool isTransmitting;
  bool isReceiving;
  int _ss;
  int _reset;
  long _txfrequency;
  long _rxfrequency;
  int rx_dma_state;
  int tx_dma_state;

  SPI_HandleTypeDef _spi_mgmt;

  SPI_HandleTypeDef _spi_I_Data;
  DMA_HandleTypeDef hdma_spi1_rx;
  DMA_HandleTypeDef hdma_spi1_tx; // SPI1 for I data
  DFSDM_Channel_HandleTypeDef  hdfsdm_I;
  DFSDM_Filter_HandleTypeDef hdfsdm_filt_I; // Just need a filter, data is from SPI



  SPI_HandleTypeDef _spi_Q_Data;
  DMA_HandleTypeDef hdma_spi2_rx;
  DMA_HandleTypeDef hdma_spi2_tx; // SPI2 for Q data
  //DFSDM_Channel_HandleTypeDef  hdfsdm_Q;

  SDR_stats sx1257_stats;





  void print_rx_iq_data(bool long_run);
  void debug_dfsdm();
  void reset_dma_buffers();

  void (*_onReceive)(int);
  void (*_onCadDone)(boolean);
  void (*_onTxDone)();

  uint8_t readRegister(uint8_t address);
  void writeRegister(uint8_t address, uint8_t value);
  uint8_t singleTransfer(uint8_t address, uint8_t value);

};


inline int16_t convert_spi_to_dfsdm(uint32_t val);
inline uint32_t calc_ampl(uint32_t i_val, uint32_t q_val);


#endif
