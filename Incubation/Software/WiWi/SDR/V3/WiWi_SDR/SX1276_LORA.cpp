// https://github.com/sandeepmistry/arduino-LoRa/blob/master/src/LoRa.cpp

// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "SX1276_LORA.h"

// registers
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_OCP                  0x0b
#define REG_LNA                  0x0c
#define REG_FIFO_ADDR_PTR        0x0d
#define REG_FIFO_TX_BASE_ADDR    0x0e
#define REG_FIFO_RX_BASE_ADDR    0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_MASK             0x11
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1a
#define REG_RSSI_VALUE           0x1b
#define REG_MODEM_CONFIG_1       0x1d
#define REG_MODEM_CONFIG_2       0x1e
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_FREQ_ERROR_MSB       0x28
#define REG_FREQ_ERROR_MID       0x29
#define REG_FREQ_ERROR_LSB       0x2a
#define REG_RSSI_WIDEBAND        0x2c
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_INVERTIQ             0x33
#define REG_DETECTION_THRESHOLD  0x37
#define REG_SYNC_WORD            0x39
#define REG_INVERTIQ2            0x3b
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4d

// modes
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05
#define MODE_RX_SINGLE           0x06
#define MODE_CAD                 0x07

// PA config
#define PA_BOOST                 0x80

// IRQ masks
#define IRQ_TX_DONE_MASK           0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK           0x40
#define IRQ_CAD_DONE_MASK          0x04
#define IRQ_CAD_DETECTED_MASK      0x01

#define RF_MID_BAND_THRESHOLD    525E6
#define RSSI_OFFSET_HF_PORT      157
#define RSSI_OFFSET_LF_PORT      164

#define MAX_PKT_LENGTH           255

#if (ESP8266 || ESP32)
    #define ISR_PREFIX ICACHE_RAM_ATTR
#else
    #define ISR_PREFIX
#endif

LoRaClass SX1276_Lora;

LoRaClass::LoRaClass() :
  _ss(LORA_DEFAULT_SS_PIN), _reset(LORA_DEFAULT_RESET_PIN), _dio0(LORA_DEFAULT_DIO0_PIN),
  _frequency(0),
  _packetIndex(0),
  _implicitHeaderMode(0),
  _onReceive(NULL),
  _onCadDone(NULL),
  _onTxDone(NULL)
{
  // overide Stream timeout value
  setTimeout(0);

}

int LoRaClass::init() {

  // Init the STM32 SPI5 interface
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // SPI5 SCK Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1276_SCK].GPIO_Pin;  
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI5;  // Alternate function for SPI5
  HAL_GPIO_Init(WWVB_Pins[SX1276_SCK].GPIO_Group, &GPIO_InitStruct);

  // SPI5 MISO Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1276_MISO].GPIO_Pin;  
  HAL_GPIO_Init(WWVB_Pins[SX1276_MISO].GPIO_Group, &GPIO_InitStruct);

  // SPI5 MOSI Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1276_MOSI].GPIO_Pin; 
  HAL_GPIO_Init(WWVB_Pins[SX1276_MOSI].GPIO_Group, &GPIO_InitStruct);

  _spi.Instance = SPI5;
  _spi.Init.Mode = SPI_MODE_MASTER;  // SPI mode (Master/Slave)
  _spi.Init.Direction = SPI_DIRECTION_2LINES;  // Full duplex mode
  _spi.Init.DataSize = SPI_DATASIZE_8BIT;  // 8-bit data frame format
  _spi.Init.CLKPolarity = SPI_POLARITY_LOW;  // Clock polarity
  _spi.Init.CLKPhase = SPI_PHASE_1EDGE;  // Clock phase
  _spi.Init.NSS = SPI_NSS_SOFT;  // NSS signal is managed by software
  _spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;  // Baud rate prescaler
  _spi.Init.FirstBit = SPI_FIRSTBIT_MSB;  // Data is transmitted MSB first
  _spi.Init.TIMode = SPI_TIMODE_DISABLE;  // Disable TI mode
  _spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Disable CRC calculation
  _spi.Init.CRCPolynomial = 7;  // Polynomial for CRC calculation

  if ( HAL_SPI_Init(&_spi) != HAL_OK ) {
    Serial.println("FAILED TO INIT SX1276 LORA SPI");
    return -1;
  }

  wwvb_gpio_pinmode(SX1276_DIO0,INPUT);

  wwvb_gpio_pinmode(LORA_LF_TXRX_SEL, OUTPUT);
  wwvb_gpio_pinmode(LORA_HF_TXRX_SEL, OUTPUT);
  wwvb_gpio_pinmode(LORA_LF_HF_SEL, OUTPUT);

  setantenna(1,1,0); // default SMA -> HF -> RX

  return 0;
}

int LoRaClass::setantenna(bool sma, bool hf, bool tx) {

  if ( hf ) {
    wwvb_digital_write(LORA_LF_TXRX_SEL, 1); // set LF to RX, output 1
    wwvb_digital_write(LORA_LF_HF_SEL, 0); // set to HF, output 2
    if ( tx ) wwvb_digital_write(LORA_HF_TXRX_SEL, 1); // output 1 for HF TX
    else wwvb_digital_write(LORA_HF_TXRX_SEL, 0); // output 2 for HF RX
  } else {
    wwvb_digital_write(LORA_LF_TXRX_SEL, 0); // set HF to RX, output 2
    wwvb_digital_write(LORA_LF_HF_SEL, 1); // set to LF, output 1
    if ( tx ) wwvb_digital_write(LORA_LF_TXRX_SEL, 0); // output 2 for LF TX
    else wwvb_digital_write(LORA_LF_TXRX_SEL, 1); // output 1 for LF RX
  }
}



// a bit hacky, but config parameters used by begin
long config_sbw = 125e3;
int codingratedenom = 1;
int spreadingfactor = 7;
int preamblelen = 8;
long config_freq = 900e6;

int LoRaClass::begin(long frequency)
{
  Serial.println("SX1276 begin start");
  config_freq = frequency;
  // setup pins
  wwvb_gpio_pinmode(_ss, OUTPUT);
  // set SS high
  wwvb_digital_write(_ss, HIGH);

  if (_reset != -1) {
    wwvb_gpio_pinmode(_reset, OUTPUT);

    // perform reset
    wwvb_digital_write(_reset, LOW);
    delay(10);
    wwvb_digital_write(_reset, HIGH);
    delay(10);
  }



  // check version
  Serial.println("SX1276 version check");
  uint8_t version = readRegister(REG_VERSION);
  if (version != 0x12) {
    Serial.println("LORA Init read version register didn't get 0x12! FAIL");
    return 0;
  }

  // put in sleep mode
  Serial.println("SX1276 put in sleep");
  sleep();

  // set frequency
  setFrequency(frequency);

  // set base addresses
  writeRegister(REG_FIFO_TX_BASE_ADDR, 0);
  writeRegister(REG_FIFO_RX_BASE_ADDR, 0);

  // set LNA boost
  writeRegister(REG_LNA, readRegister(REG_LNA) | 0x03);

  // set auto AGC
  writeRegister(REG_MODEM_CONFIG_3, 0x04);

  // set output power to 17 dBm
  //setTxPower(17);

  writeRegister(REG_IRQ_MASK, 0xA7); // mask everything except RxDone and ValidHeader and TxDone
  setTxPower(17,PA_OUTPUT_PA_BOOST_PIN); // V2 board , HF output uses boost pin!
  setCodingRate4(codingratedenom);
  setSignalBandwidth(config_sbw);
  setSpreadingFactor(spreadingfactor);
  setPreambleLength(preamblelen);
  enableLowDataRateOptimize();
  //disableLowDataRateOptimize();
  //setGain(0); // AGC mode
  disableCrc();



  // put in standby mode
  idle();
  Serial.println("SX1276 begin end");

  return 1;
}

void LoRaClass::end()
{
  Serial.println("SX1276 end start");
  // put in sleep mode
  sleep();
  Serial.println("SX1276 end end");

}

int LoRaClass::beginPacket(int implicitHeader)
{
  //Serial.println("SX1276 Begin packet start");
  if (isTransmitting()) {
    Serial.println("LoRA begin packet end not transmitting");
    return 0;
  }

  // put in standby mode
  //idle();

  if (implicitHeader) {
    implicitHeaderMode();
  } else {
    explicitHeaderMode();
  }

  // reset FIFO address and paload length
  writeRegister(REG_FIFO_ADDR_PTR, 0);
  writeRegister(REG_PAYLOAD_LENGTH, 0);
  //Serial.println("SX1276 Begin packet end");

  return 1;
}

void LoRaClass::setDio0_TxDone()
{
  writeRegister(REG_DIO_MAPPING_1, 0x40); // DIO0 => TXDONE
}

void LoRaClass::setDio0_RxDone()
{
  writeRegister(REG_DIO_MAPPING_1, 0x0); // DIO0 => RXDONE
}

void LoRaClass::clearIRQs()
{
  // clear IRQ's
  writeRegister(REG_IRQ_FLAGS, 0xFF);
}

bool LoRaClass::getDio0Val() 
{
  return wwvb_digital_read(SX1276_DIO0);
}

int LoRaClass::endPacket(bool async)
{
  uint8_t val = 0;
  //Serial.println("SX1276 end packet start");
  //Serial.println("SX1276 end packet async start");
  
  //Serial.println("SX1276 end packet async end");
  //Serial.println("SX1276 end packet point 1");
  // put in TX mode
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);
  //Serial.println("SX1276 end packet point 2");
  int wait_count = 0;
  if (!async) {
    // wait for TX done
    //Serial.println("SX1276 end packet point 3");
    while ( !getDio0Val() )
    {
      delayMicroseconds(1);
      wait_count++;
    }

    //val = readRegister(REG_IRQ_FLAGS);
    //sprintf(print_buffer, "SX1276 end packet point4, count=%d\r\n",wait_count);
    //Serial.print(print_buffer);
    //while ( (val & IRQ_TX_DONE_MASK) == 0) {
    //  val = readRegister(REG_IRQ_FLAGS);
      //Serial.print("Wait TX done 0x");
      //Serial.println(val, HEX);
    //}
    //Serial.println("SX1276 end packet point 5");
    // clear IRQ's
    //writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
  }
  //Serial.println("SX1276 end packet end");
  return 1;
}

bool LoRaClass::isTransmitting()
{
  //Serial.println("SX1276 is transmitting start");
  if ((readRegister(REG_OP_MODE) & MODE_TX) == MODE_TX) {
    return true;
  }

  if (readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) {
    // clear IRQ's
    writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
  }
  //Serial.println("SX1276 is not transmitting!");
  return false;
}

int LoRaClass::checkRxDone()
{
  return (readRegister(REG_IRQ_FLAGS) & IRQ_RX_DONE_MASK);
}


// clear IRQs AFTER this, not before
int LoRaClass::parsePacket(int size)
{
  int packetLength = 0;
  int irqFlags = readRegister(REG_IRQ_FLAGS);
  //Serial.println("*******SX1276 LoRA parsePacket*******");
  sprintf(print_buffer,"ParsePacket size=%d, irqFlags = 0x%x\r\n", size, irqFlags);
  Serial.print(print_buffer);

  if (size > 0) {
    implicitHeaderMode();

    writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
  } else {
    explicitHeaderMode();
  }

  // clear IRQ's
  //writeRegister(REG_IRQ_FLAGS, irqFlags);

  if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {
    //Serial.println("Parse packet, first if case");
    // received a packet
    _packetIndex = 0;

    // read packet length
    if (_implicitHeaderMode) {
      packetLength = readRegister(REG_PAYLOAD_LENGTH);
    } else {
      packetLength = readRegister(REG_RX_NB_BYTES);
    }

    // set FIFO address to current RX address
    writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));

    // if mode is in continous mode, don't change it
    // otherwise put into idle
    // put in standby mode
    if ( readRegister(REG_OP_MODE) == (MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS) ) {
      //Serial.println("Parse packet, in continous mode, leave alone");
    } else {
      idle();
    }
  } else if (readRegister(REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE)) {
    Serial.println("Parse packet, not currently in rx mode");
    // not currently in RX mode

    // reset FIFO address
    writeRegister(REG_FIFO_ADDR_PTR, 0);

    // put in single RX mode
    writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
  }

  return packetLength;
}

int LoRaClass::packetRssi()
{
  return (readRegister(REG_PKT_RSSI_VALUE) - (_frequency < RF_MID_BAND_THRESHOLD ? RSSI_OFFSET_LF_PORT : RSSI_OFFSET_HF_PORT));
}

float LoRaClass::packetSnr()
{
  return ((int8_t)readRegister(REG_PKT_SNR_VALUE)) * 0.25;
}

long LoRaClass::packetFrequencyError()
{
  int32_t freqError = 0;
  freqError = static_cast<int32_t>(readRegister(REG_FREQ_ERROR_MSB) & 0b111);
  freqError <<= 8L;
  freqError += static_cast<int32_t>(readRegister(REG_FREQ_ERROR_MID));
  freqError <<= 8L;
  freqError += static_cast<int32_t>(readRegister(REG_FREQ_ERROR_LSB));

  if (readRegister(REG_FREQ_ERROR_MSB) & 0b1000) { // Sign bit is on
     freqError -= 524288; // 0b1000'0000'0000'0000'0000
  }

  const float fXtal = 32E6; // FXOSC: crystal oscillator (XTAL) frequency (2.5. Chip Specification, p. 14)
  const float fError = ((static_cast<float>(freqError) * (1L << 24)) / fXtal) * (getSignalBandwidth() / 500000.0f); // p. 37

  return static_cast<long>(fError);
}

int LoRaClass::rssi()
{
  return (readRegister(REG_RSSI_VALUE) - (_frequency < RF_MID_BAND_THRESHOLD ? RSSI_OFFSET_LF_PORT : RSSI_OFFSET_HF_PORT));
}

size_t LoRaClass::write(uint8_t byte)
{
  return write(&byte, sizeof(byte));
}

size_t LoRaClass::write(const uint8_t *buffer, size_t size)
{
  int currentLength = readRegister(REG_PAYLOAD_LENGTH);

  // check size
  if ((currentLength + size) > MAX_PKT_LENGTH) {
    size = MAX_PKT_LENGTH - currentLength;
  }

  // write data
  for (size_t i = 0; i < size; i++) {
    writeRegister(REG_FIFO, buffer[i]);
  }

  // update length
  writeRegister(REG_PAYLOAD_LENGTH, currentLength + size);

  return size;
}

int LoRaClass::available()
{
  return (readRegister(REG_RX_NB_BYTES) - _packetIndex);
}

int LoRaClass::read()
{
  if (!available()) {
    return -1;
  }

  _packetIndex++;

  return readRegister(REG_FIFO);
}

int LoRaClass::peek()
{
  if (!available()) {
    return -1;
  }

  // store current FIFO address
  int currentAddress = readRegister(REG_FIFO_ADDR_PTR);

  // read
  uint8_t b = readRegister(REG_FIFO);

  // restore FIFO address
  writeRegister(REG_FIFO_ADDR_PTR, currentAddress);

  return b;
}

void LoRaClass::flush()
{
}

void LoRaClass::onReceive(void(*callback)(int))
{
  _onReceive = callback;
  Serial.println("SX1276 on receive start");

  if (callback) {
    wwvb_gpio_pinmode(SX1276_DIO0, INPUT);
/*
#ifdef SPI_HAS_NOTUSINGINTERRUPT
    SPI.usingInterrupt(digitalPinToInterrupt(_dio0));
#endif
    attachInterrupt(digitalPinToInterrupt(_dio0), LoRaClass::onDio0Rise, RISING);
*/
  } else {
/*
    detachInterrupt(digitalPinToInterrupt(_dio0));
#ifdef SPI_HAS_NOTUSINGINTERRUPT
    SPI.notUsingInterrupt(digitalPinToInterrupt(_dio0));
#endif
*/
  }
}

void LoRaClass::onCadDone(void(*callback)(boolean))
{
  _onCadDone = callback;
  Serial.println("SX1276 on cad done start");

  if (callback) {
    wwvb_gpio_pinmode(SX1276_DIO0, INPUT);
/*
#ifdef SPI_HAS_NOTUSINGINTERRUPT
    SPI.usingInterrupt(digitalPinToInterrupt(_dio0));
#endif
    attachInterrupt(digitalPinToInterrupt(_dio0), LoRaClass::onDio0Rise, RISING);
*/
  } else {
/*
    detachInterrupt(digitalPinToInterrupt(_dio0));
#ifdef SPI_HAS_NOTUSINGINTERRUPT
    SPI.notUsingInterrupt(digitalPinToInterrupt(_dio0));
#endif
*/
  }
}

void LoRaClass::onTxDone(void(*callback)())
{
  _onTxDone = callback;
  Serial.println("SX1276 TX DONE start");

  if (callback) {
/*
    wwvb_gpio_pinmode(SX1276_DIO0, INPUT);
#ifdef SPI_HAS_NOTUSINGINTERRUPT
    SPI.usingInterrupt(digitalPinToInterrupt(_dio0));
#endif
    attachInterrupt(digitalPinToInterrupt(_dio0), LoRaClass::onDio0Rise, RISING);
*/
  } else {
/*
    detachInterrupt(digitalPinToInterrupt(_dio0));
#ifdef SPI_HAS_NOTUSINGINTERRUPT
    SPI.notUsingInterrupt(digitalPinToInterrupt(_dio0));
#endif
*/
  }
}

void LoRaClass::receive(int size)
{
  // page 41 of datasheet, put into sleep or standby mode first before putting in rxcont mode

  // put in standby mode
  //Serial.println("************SX1276_LoRA receive**************");
  //idle();  
  //writeRegister(REG_DIO_MAPPING_1, 0x0); // DIO0 => RXDONE ([7:6] = 0x0) , DIO3 => ValidHeader ([1:0] = 0x1)

  if (size > 0) {
    implicitHeaderMode();

    writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
  } else {
    explicitHeaderMode();
  }

  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
}

void LoRaClass::channelActivityDetection(void)
{
  writeRegister(REG_DIO_MAPPING_1, 0x80);// DIO0 => CADDONE
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_CAD);
}


void LoRaClass::idle()
{
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void LoRaClass::sleep()
{
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
}

void LoRaClass::setTxPower(int level, int outputPin)
{
  if (PA_OUTPUT_RFO_PIN == outputPin) {
    // RFO
    if (level < 0) {
      level = 0;
    } else if (level > 14) {
      level = 14;
    }

    writeRegister(REG_PA_CONFIG, 0x70 | level);
  } else {
    // PA BOOST
    if (level > 17) {
      if (level > 20) {
        level = 20;
      }

      // subtract 3 from level, so 18 - 20 maps to 15 - 17
      level -= 3;

      // High Power +20 dBm Operation (Semtech SX1276/77/78/79 5.4.3.)
      writeRegister(REG_PA_DAC, 0x87);
      setOCP(140);
    } else {
      if (level < 2) {
        level = 2;
      }
      //Default value PA_HF/LF or +17dBm
      writeRegister(REG_PA_DAC, 0x84);
      setOCP(100);
    }

    writeRegister(REG_PA_CONFIG, PA_BOOST | (level - 2));
  }
}

void LoRaClass::setFrequency(long frequency)
{
  _frequency = frequency;

  uint64_t frf = ((uint64_t)frequency << 19) / 32000000;

  writeRegister(REG_FRF_MSB, (uint8_t)(frf >> 16));
  writeRegister(REG_FRF_MID, (uint8_t)(frf >> 8));
  writeRegister(REG_FRF_LSB, (uint8_t)(frf >> 0));
}

int LoRaClass::getSpreadingFactor()
{
  return readRegister(REG_MODEM_CONFIG_2) >> 4;
}

void LoRaClass::setSpreadingFactor(int sf)
{
  if (sf < 6) {
    sf = 6;
  } else if (sf > 12) {
    sf = 12;
  }

  if (sf == 6) {
    writeRegister(REG_DETECTION_OPTIMIZE, 0xc5);
    writeRegister(REG_DETECTION_THRESHOLD, 0x0c);
  } else {
    writeRegister(REG_DETECTION_OPTIMIZE, 0xc3);
    writeRegister(REG_DETECTION_THRESHOLD, 0x0a);
  }

  writeRegister(REG_MODEM_CONFIG_2, (readRegister(REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0));
  setLdoFlag();
}

long LoRaClass::getSignalBandwidth()
{
  byte bw = (readRegister(REG_MODEM_CONFIG_1) >> 4);

  switch (bw) {
    case 0: return 7.8E3;
    case 1: return 10.4E3;
    case 2: return 15.6E3;
    case 3: return 20.8E3;
    case 4: return 31.25E3;
    case 5: return 41.7E3;
    case 6: return 62.5E3;
    case 7: return 125E3;
    case 8: return 250E3;
    case 9: return 500E3;
  }

  return -1;
}

void LoRaClass::setSignalBandwidth(long sbw)
{
  int bw;

  if (sbw <= 7.8E3) {
    bw = 0;
  } else if (sbw <= 10.4E3) {
    bw = 1;
  } else if (sbw <= 15.6E3) {
    bw = 2;
  } else if (sbw <= 20.8E3) {
    bw = 3;
  } else if (sbw <= 31.25E3) {
    bw = 4;
  } else if (sbw <= 41.7E3) {
    bw = 5;
  } else if (sbw <= 62.5E3) {
    bw = 6;
  } else if (sbw <= 125E3) {
    bw = 7;
  } else if (sbw <= 250E3) {
    bw = 8;
  } else /*if (sbw <= 250E3)*/ {
    bw = 9;
  }

  writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
  setLdoFlag();
}

void LoRaClass::setLdoFlag()
{
  // Section 4.1.1.5
  long symbolDuration = 1000 / ( getSignalBandwidth() / (1L << getSpreadingFactor()) ) ;

  // Section 4.1.1.6
  boolean ldoOn = symbolDuration > 16;

  uint8_t config3 = readRegister(REG_MODEM_CONFIG_3);
  bitWrite(config3, 3, ldoOn);
  writeRegister(REG_MODEM_CONFIG_3, config3);
}

void LoRaClass::setLdoFlagForced(const boolean ldoOn)
{
  uint8_t config3 = readRegister(REG_MODEM_CONFIG_3);
  bitWrite(config3, 3, ldoOn);
  writeRegister(REG_MODEM_CONFIG_3, config3);
}

void LoRaClass::setCodingRate4(int denominator)
{
  if (denominator < 5) {
    denominator = 5;
  } else if (denominator > 8) {
    denominator = 8;
  }

  int cr = denominator - 4;

  writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0xf1) | (cr << 1));
}

void LoRaClass::setPreambleLength(long length)
{
  writeRegister(REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
  writeRegister(REG_PREAMBLE_LSB, (uint8_t)(length >> 0));
}

void LoRaClass::setSyncWord(int sw)
{
  writeRegister(REG_SYNC_WORD, sw);
}

void LoRaClass::enableCrc()
{
  writeRegister(REG_MODEM_CONFIG_2, readRegister(REG_MODEM_CONFIG_2) | 0x04);
}

void LoRaClass::disableCrc()
{
  writeRegister(REG_MODEM_CONFIG_2, readRegister(REG_MODEM_CONFIG_2) & 0xfb);
}

void LoRaClass::enableInvertIQ()
{
  writeRegister(REG_INVERTIQ,  0x66);
  writeRegister(REG_INVERTIQ2, 0x19);
}

void LoRaClass::disableInvertIQ()
{
  writeRegister(REG_INVERTIQ,  0x27);
  writeRegister(REG_INVERTIQ2, 0x1d);
}

void LoRaClass::enableLowDataRateOptimize()
{
   setLdoFlagForced(true);
}

void LoRaClass::disableLowDataRateOptimize()
{
   setLdoFlagForced(false);
}

void LoRaClass::setOCP(uint8_t mA)
{
  uint8_t ocpTrim = 27;

  if (mA <= 120) {
    ocpTrim = (mA - 45) / 5;
  } else if (mA <=240) {
    ocpTrim = (mA + 30) / 10;
  }

  writeRegister(REG_OCP, 0x20 | (0x1F & ocpTrim));
}

void LoRaClass::setGain(uint8_t gain)
{
  // check allowed range
  if (gain > 6) {
    gain = 6;
  }
  
  // set to standby
  idle();
  
  // set gain
  if (gain == 0) {
    // if gain = 0, enable AGC
    writeRegister(REG_MODEM_CONFIG_3, 0x04);
  } else {
    // disable AGC
    writeRegister(REG_MODEM_CONFIG_3, 0x00);
	
    // clear Gain and set LNA boost
    writeRegister(REG_LNA, 0x03);
	
    // set gain
    writeRegister(REG_LNA, readRegister(REG_LNA) | (gain << 5));
  }
}

byte LoRaClass::random()
{
  return readRegister(REG_RSSI_WIDEBAND);
}


void LoRaClass::dumpRegisters(Stream& out)
{
  for (int i = 0; i < 128; i++) {
    out.print("SX1276 Register 0x");
    out.print(i, HEX);
    out.print(": 0x");
    out.println(readRegister(i), HEX);
  }
}

void LoRaClass::explicitHeaderMode()
{
  _implicitHeaderMode = 0;

  writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) & 0xfe);
}

void LoRaClass::implicitHeaderMode()
{
  _implicitHeaderMode = 1;

  writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) | 0x01);
}

void LoRaClass::handleDio0Rise()
{
  Serial.println("SX1276 Handle DIO0 rise");
  int irqFlags = readRegister(REG_IRQ_FLAGS);

  // clear IRQ's
  writeRegister(REG_IRQ_FLAGS, irqFlags);

  if ((irqFlags & IRQ_CAD_DONE_MASK) != 0) {
    if (_onCadDone) {
      _onCadDone((irqFlags & IRQ_CAD_DETECTED_MASK) != 0);
    }
  } else if ((irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {

    if ((irqFlags & IRQ_RX_DONE_MASK) != 0) {
      // received a packet
      _packetIndex = 0;

      // read packet length
      int packetLength = _implicitHeaderMode ? readRegister(REG_PAYLOAD_LENGTH) : readRegister(REG_RX_NB_BYTES);

      // set FIFO address to current RX address
      writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));

      if (_onReceive) {
        _onReceive(packetLength);
      }
    } else if ((irqFlags & IRQ_TX_DONE_MASK) != 0) {
      if (_onTxDone) {
        _onTxDone();
      }
    }
  }
}

uint8_t LoRaClass::readRegister(uint8_t address)
{
  return singleTransfer(address & 0x7f, 0x00);
}

void LoRaClass::writeRegister(uint8_t address, uint8_t value)
{
  singleTransfer(address | 0x80, value);
}

uint8_t LoRaClass::singleTransfer(uint8_t address, uint8_t value)
{
  uint8_t response;
  HAL_StatusTypeDef retval;


  
  //_spi->beginTransaction(_spiSettings);
  wwvb_digital_write(_ss, LOW);
  delayMicroseconds(20); // setup time for slave select, SPI is super fast API apparently

  //_spi->transfer(address);
  retval = HAL_SPI_TransmitReceive(&_spi, &address, &response, sizeof(address), HAL_MAX_DELAY);  //ignore receive data
  if ( retval != HAL_OK) {
    Serial.println("SX1276 LORA Single transfer not ok 1");
  }

  //response = _spi->transfer(value);
  retval = HAL_SPI_TransmitReceive(&_spi, &value, &response, sizeof(value), HAL_MAX_DELAY);
  if ( retval != HAL_OK) {
    Serial.println("SX1276 LORA Single transfer not ok 2");
  }

  wwvb_digital_write(_ss, HIGH);
  delayMicroseconds(20); // needs some hold time for slave select , SPI HAL can be super fast


  /*
  Serial.print("SX1276 Lora single transfer 0x");
  Serial.print(address, HEX);
  Serial.print(" = 0x");
  Serial.print(value,HEX);
  Serial.print(" , response = 0x");
  Serial.println(response,HEX);
  */

  

  return response;
}

/*
ISR_PREFIX void LoRaClass::onDio0Rise()
{
  LoRa.handleDio0Rise();
}
*/




/**************** CLI functions **************/


void onSX1276Write(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t addr = 0;
  uint8_t writeval = 0;
  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("SX1276 write no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 2 ) {
    Serial.println("SX1276 write needs 2 arguments!");
    return;
  } 

  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 1), &addr ) ) {
    Serial.println("Failed to parse first argument for uint8_t");
    return;
  }
  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 2), &writeval ) ) {
    Serial.println("Failed to parse second argument for uint8_t");
    return;
  }

  // parsed everything , now do the write
  SX1276_Lora.writeRegister(addr, writeval);
  Serial.println("SX1276 Register write done");
}

void onSX1276Read(EmbeddedCli *cli, char *args, void *context)
{
  uint8_t addr = 0;
  uint8_t read_val = 0;
  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("SX1276 read no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 1 ) {
    Serial.println("SX1276 read needs 1 argument!");
    return;
  } 

  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 1), &addr ) ) {
    Serial.println("Failed to parse first argument for uint16_t");
    return;
  }

  read_val = SX1276_Lora.readRegister(addr);


  sprintf(print_buffer, "SX1276 addr=0x%x, read back 0x%x\r\n", 
    addr, read_val);
  Serial.print(print_buffer);

}

void onSX1276SetConfig(EmbeddedCli *cli, char *args, void *context)
{
  if ( embeddedCliGetTokenCount(args) != 5 ) {
    //Serial.println("SX1276 set config needs 5 arguments!");
    sprintf(print_buffer, "SX1276 Config:\r\n");
    sprintf(print_buffer, "%sFrequency:%ld\r\n", print_buffer, config_freq);
    sprintf(print_buffer, "%sBandwidth:%ld\r\n", print_buffer, config_sbw);
    sprintf(print_buffer, "%sCoding rate:%d\r\n", print_buffer, codingratedenom);
    sprintf(print_buffer, "%sSpreading factor:%ld\r\n", print_buffer, spreadingfactor);
    sprintf(print_buffer, "%sPreamble length:%ld\r\n", print_buffer, preamblelen);
    Serial.print(print_buffer);

    return;
  } 
  uint32_t freq=0;
  uint32_t bw=0;
  uint8_t codingrate=0;
  uint8_t sf=0;
  uint8_t prelen=0;


  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 1), &freq ) ) {
    Serial.println("Failed to parse first argument for uint32_t");
    return;
  }
  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 2), &bw ) ) {
    Serial.println("Failed to parse second argument for uint32_t");
    return;
  }
  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 3), &codingrate ) ) {
    Serial.println("Failed to parse third argument for uint8_t");
    return;
  }
  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 4), &sf ) ) {
    Serial.println("Failed to parse fourth argument for uint8_t");
    return;
  }
  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 5), &prelen ) ) {
    Serial.println("Failed to parse fifth argument for uint8_t");
    return;
  }

  // set the global variables 
  config_sbw = bw;
  codingratedenom = codingrate;
  spreadingfactor = sf;
  preamblelen = prelen;
  config_freq = ((long)freq) * 1000;

  // basically re-init the transceiver , argument is in KHz, convert to Hz as expected by API
  SX1276_Lora.begin( config_freq );
}

void onSX1276SetFreq(EmbeddedCli *cli, char *args, void *context)
{
  if ( embeddedCliGetTokenCount(args) != 1 ) {
    //Serial.println("SX1276 set Frequency needs 1 argument1!");
    sprintf(print_buffer, "SX1276 Frequency: %ld Hz\r\n", config_freq); Serial.print(print_buffer);
    return;
  } 
  uint32_t val = 0;
  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 1), &val ) ) {
    Serial.println("Failed to parse first argument for uint32_t");
    return;
  }
  config_freq = ((long) val)*1000;
  SX1276_Lora.begin( config_freq );
}

void onSX1276SetPreambleLen(EmbeddedCli *cli, char *args, void *context)
{
  if ( embeddedCliGetTokenCount(args) != 1 ) {
    //Serial.println("SX1276 set Preamble Len needs 1 argument1!");
    sprintf(print_buffer, "SX1276 Preamble length: %d Hz\r\n", preamblelen); Serial.print(print_buffer);
    return;
  } 
  uint32_t val = 0;
  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 1), &val ) ) {
    Serial.println("Failed to parse first argument for uint32_t");
    return;
  }
  preamblelen = (uint32_t) val;
  SX1276_Lora.begin( config_freq );
}

void onSX1276SetCodeRate(EmbeddedCli *cli, char *args, void *context)
{
  if ( embeddedCliGetTokenCount(args) != 1 ) {
    //Serial.println("SX1276 set Code rate needs 1 argument1!");
    sprintf(print_buffer, "SX1276 Code rate: %d\r\n", codingratedenom); Serial.print(print_buffer);
    return;
  } 
  uint32_t val = 0;
  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 1), &val ) ) {
    Serial.println("Failed to parse first argument for uint32_t");
    return;
  }
  codingratedenom = (uint32_t) val;
  SX1276_Lora.begin( config_freq );
}

void onSX1276SetBW(EmbeddedCli *cli, char *args, void *context)
{
  if ( embeddedCliGetTokenCount(args) != 1 ) {
    //Serial.println("SX1276 set BW needs 1 argument1!");
    sprintf(print_buffer, "SX1276 Bandwidth: %ld\r\n", config_sbw); Serial.print(print_buffer);
    return;
  } 
  uint32_t val = 0;
  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 1), &val ) ) {
    Serial.println("Failed to parse first argument for uint32_t");
    return;
  }
  config_sbw = (long)val;
  SX1276_Lora.begin( config_freq );
}

void onSX1276SetSF(EmbeddedCli *cli, char *args, void *context)
{
  if ( embeddedCliGetTokenCount(args) != 1 ) {
    //Serial.println("SX1276 set SF needs 1 argument1!");
    sprintf(print_buffer, "SX1276 Spreading Factor: %d\r\n", spreadingfactor); Serial.print(print_buffer);
    return;
  } 
  uint32_t val = 0;
  if ( !try_parse_hex_uint32t( embeddedCliGetToken(args, 1), &val ) ) {
    Serial.println("Failed to parse first argument for uint32_t");
    return;
  }
  spreadingfactor = (uint8_t) val;
  SX1276_Lora.begin( config_freq );
}



static Node sx1276_set_freq_node = { .name = "freq", 
  .type = MY_FILE, 
  .cliBinding = {
    "freq",
    "Config SX1276 LoRA Frequency (In KHz), pass nothing to print. ex: freq 900000",
    true,
    nullptr,
    onSX1276SetFreq
  }
};

static Node sx1276_set_prelen_node = { .name = "preamblelen", 
  .type = MY_FILE, 
  .cliBinding = {
    "preamblelen",
    "Config SX1276 LoRA Preamble length, pass nothing to print. ex: preamblelen 8",
    true,
    nullptr,
    onSX1276SetPreambleLen
  }
};


static Node sx1276_set_coderate_node = { .name = "coderate", 
  .type = MY_FILE, 
  .cliBinding = {
    "coderate",
    "Config SX1276 LoRA coderate, pass nothing to print. ex: coderate 1",
    true,
    nullptr,
    onSX1276SetCodeRate
  }
};

static Node sx1276_set_bw_node = { .name = "bw", 
  .type = MY_FILE, 
  .cliBinding = {
    "bw",
    "Config SX1276 LoRA Bandwidth, pass nothing to print. ex: bw 125000",
    true,
    nullptr,
    onSX1276SetBW
  }
};

static Node sx1276_set_sf_node = { .name = "sf", 
  .type = MY_FILE, 
  .cliBinding = {
    "sf",
    "Config SX1276 LoRA SF, pass nothing to print. ex: sf 7",
    true,
    nullptr,
    onSX1276SetSF
  }
};

static Node sx1276_config_node = { .name = "config", 
  .type = MY_FILE, 
  .cliBinding = {
    "config",
    "Config SX1276 LoRA, pass frequency(KHz) / bandwidth / codingrate / SF / Preamble Length, pass nothing to print.  ex: config 900000 125000 1 7 8",
    true,
    nullptr,
    onSX1276SetConfig
  }
};

static Node sx1276_write_reg_node = { .name = "sx1276-write-reg", 
  .type = MY_FILE, 
  .cliBinding = {
    "sx1276-write-reg",
    "Write a SX1276 register, pass address (8-bit) / value (8-bit) ex: sx1276-write-reg 0x6 0x6c",
    true,
    nullptr,
    onSX1276Write
  }
};

static Node sx1276_read_reg_node = { .name = "sx1276-read-reg", 
  .type = MY_FILE, 
  .cliBinding = {
    "sx1276-read-reg",
    "Read a SX1276 register, pass address (8-bit)  ex: sx1276-read-reg 0x6",
    true,
    nullptr,
    onSX1276Read
  }
};

void sx1276_dir_operation(EmbeddedCli *cli, char *args, void *context); // forward declaration

static Node * sx1276_files[] = { &sx1276_write_reg_node, &sx1276_read_reg_node, &sx1276_config_node,
  &sx1276_set_freq_node,
  &sx1276_set_prelen_node,
  &sx1276_set_coderate_node,
  &sx1276_set_bw_node,
  &sx1276_set_sf_node };

static Node sx1276_dir = {
    .name = "sx1276",
    .type = MY_DIRECTORY,
    .cliBinding = {"sx1276",
          "SX1276 mode",
          true,
          nullptr,
          sx1276_dir_operation},
    .parent = 0,
    .children = sx1276_files,
    .num_children = sizeof(sx1276_files) / sizeof(sx1276_files[0])
};

void sx1276_dir_operation(EmbeddedCli *cli, char *args, void *context) {
  change_to_node(&sx1276_dir);
}

// Initialize function to set the parent pointers if needed
void sx1276_fs_init() {
  for (int i = 0; i < sx1276_dir.num_children; i++) {
    sx1276_files[i]->parent = &sx1276_dir;
  }
  add_root_filesystem(&sx1276_dir);
}




// Want to create a hardware abstraction layer for software
// this thread handles the raw I/O to the transceiver
// putting packet contents into fifos 
rtos::EventFlags sx1276_io_flag;

// Create queues for read and write FIFOs
rtos::Queue<LoRA_TX_Entry, 15> to_send_fifo; // tx queue
rtos::Queue<LoRA_TX_Entry, 15> send_complete_fifo; // tx completion queue
rtos::Queue<LoRA_RX_Entry, 15> rcvd_fifo; // rx queue

void sx1276_io_thread()
{
  // TX path


  // RX path


}

int sx1276_get_rcvd_fifo_num()
{
  
}






void init_sx1276_cli()
{


  SX1276_Lora.init(); 
  Serial.println("Beginning SX1276 LORA");
  if ( !SX1276_Lora.begin(900e6) ) {
    Serial.println("LoRA SX1276 init failed!");
  } else {
    Serial.println("LoRA SX1276 init successful!");
    //SX1276_Lora.dumpRegisters(Serial);
  }

  // setup default antenna selection paths
  wwvb_gpio_pinmode(LORA_LF_TXRX_SEL, OUTPUT);
  wwvb_gpio_pinmode(LORA_HF_TXRX_SEL, OUTPUT);
  wwvb_gpio_pinmode(LORA_LF_HF_SEL, OUTPUT);

  wwvb_digital_write(LORA_LF_TXRX_SEL,1); // RF1 for RX , ctrl=1
  wwvb_digital_write(LORA_HF_TXRX_SEL,0); // RF2 for RX , ctrl=0
  wwvb_digital_write(LORA_LF_HF_SEL,0); // RF2 for HF , ctrl=0

  // expose sx1276 CLI
  sx1276_fs_init();
}
