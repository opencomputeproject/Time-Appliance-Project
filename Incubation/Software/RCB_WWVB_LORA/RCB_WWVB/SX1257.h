#ifndef SX1257_H
#define SX1257_H

#include "WWVB_Arduino.h"



class SX1257Class : public Stream {
public:
  SX1257Class();

  int init();

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

  void dumpRegisters(Stream& out);

  void debug();


  void write_I(uint8_t val);
  void write_Q(uint8_t val);

private:

  bool isTransmitting;
  bool isReceiving;
  SPI_HandleTypeDef _spi_mgmt;
  SPI_HandleTypeDef _spi_I_Data;
  SPI_HandleTypeDef _spi_Q_Data;
  int _ss;
  int _reset;
  long _txfrequency;
  long _rxfrequency;


  void (*_onReceive)(int);
  void (*_onCadDone)(boolean);
  void (*_onTxDone)();

  uint8_t readRegister(uint8_t address);
  void writeRegister(uint8_t address, uint8_t value);
  uint8_t singleTransfer(uint8_t address, uint8_t value);

};


#endif
