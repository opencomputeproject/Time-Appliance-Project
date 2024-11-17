// https://github.com/sandeepmistry/arduino-LoRa/blob/master/src/LoRa.cpp

// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


// USING STM32H747 EXTI0

#ifndef LORA_H
#define LORA_H

#include <Arduino.h>
#include "WWVB_Arduino.h"
#include <stm32h7xx_hal_spi.h>
#include <stm32h7xx_hal_gpio_ex.h>
#include <stm32h7xx_hal_cortex.h>
#include "menu_cli.h"

#define LORA_DEFAULT_SPI_FREQUENCY 8E6 
#define LORA_DEFAULT_SS_PIN        SX1276_NSS
#define LORA_DEFAULT_RESET_PIN     SX1276_RST
#define LORA_DEFAULT_DIO0_PIN      SX1276_DIO0

#define PA_OUTPUT_RFO_PIN          0
#define PA_OUTPUT_PA_BOOST_PIN     1



class LoRaClass : public Stream {
public:
  LoRaClass();

  int init();

  int begin(long frequency);
  void end();

  void setDio0_TxDone();
  void setDio0_RxDone();

  bool getDio0Val();
  void clearIRQs();

  int beginPacket(int implicitHeader = false);
  int endPacket(bool async = false);

  int parsePacket(int size = 0);
  int packetRssi();
  float packetSnr();
  long packetFrequencyError();
  int checkRxDone();

  int rssi();

  // select if sma or ufl (shared with SDR), high frequency mode or low frequency, and if tx or rx
  int setantenna(bool sma, bool hf, bool tx);

  // from Print
  virtual size_t write(uint8_t byte);
  virtual size_t write(const uint8_t *buffer, size_t size);

  // from Stream
  virtual int available();
  virtual int read();
  virtual int peek();
  virtual void flush();

  void onReceive(void(*callback)(int));
  void onCadDone(void(*callback)(boolean));
  void onTxDone(void(*callback)());

  void receive(int size = 0);
  void channelActivityDetection(void);

  void idle();
  void sleep();

  void setTxPower(int level, int outputPin = PA_OUTPUT_RFO_PIN); // WWVB board, PA boost is for LF, not sure API works for that but this works for HF
  void setFrequency(long frequency);
  void setSpreadingFactor(int sf);
  void setSignalBandwidth(long sbw);
  void setCodingRate4(int denominator);
  void setPreambleLength(long length);
  void setSyncWord(int sw);
  void enableCrc();
  void disableCrc();
  void enableInvertIQ();
  void disableInvertIQ();
  void enableLowDataRateOptimize();
  void disableLowDataRateOptimize();
  
  void setOCP(uint8_t mA); // Over Current Protection control
  
  void setGain(uint8_t gain); // Set LNA gain

  // deprecated
  void crc() { enableCrc(); }
  void noCrc() { disableCrc(); }

  byte random();



  void dumpRegisters(Stream& out);

//private:
  void explicitHeaderMode();
  void implicitHeaderMode();

  void handleDio0Rise();
  bool isTransmitting();

  int getSpreadingFactor();
  long getSignalBandwidth();

  void setLdoFlag();
  void setLdoFlagForced(const boolean);

  uint8_t readRegister(uint8_t address);
  void writeRegister(uint8_t address, uint8_t value);
  uint8_t singleTransfer(uint8_t address, uint8_t value);

  static void onDio0Rise();


  SPI_HandleTypeDef _spi;
private:
  int _ss;
  int _reset;
  int _dio0;
  long _frequency;
  int _packetIndex;
  int _implicitHeaderMode;
  void (*_onReceive)(int);
  void (*_onCadDone)(boolean);
  void (*_onTxDone)();
};
extern LoRaClass SX1276_Lora;




/************ CLI functions ************/
extern rtos::EventFlags sx1276_io_flag;


typedef struct LoRA_TX_Entry {
  uint32_t id;
  char data[50];  
  int datalen;
} LoRA_TX_Entry;

typedef struct LoRA_RX_Entry {
  uint32_t id;
  char data[50];
  int datalen;
} LoRA_RX_Entry;

// placeholder for TX metadata , same as Linux ethernet
typedef struct LoRA_TX_Complete_Entry {
  uint32_t id;
} LoRA_TX_Complete_Entry;


void init_sx1276_cli();

#endif