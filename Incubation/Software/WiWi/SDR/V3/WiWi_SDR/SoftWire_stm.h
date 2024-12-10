#ifndef SOFTWIRE_H
#define SOFTWIRE_H

#define SOFTWIRE_VERSION "2.0.9"

#include <Arduino.h>
#include <stdint.h>
#include <AsyncDelay.h>
#include "WWVB_Arduino.h"

class SoftWire : public Stream {
  public:
    enum result_t {
      ack = 0,
      nack = 1,
      timedOut = 2,
    };

    enum mode_t {
      writeMode = 0,
      readMode = 1,
    };

    static const uint8_t defaultDelay_us = 1;
    static const uint16_t defaultTimeout_ms = 200;

    static void sdaLow(const SoftWire *p);
    static void sdaHigh(const SoftWire *p);
    static void sclLow(const SoftWire *p);
    static void sclHigh(const SoftWire *p);
    static uint8_t readSda(const SoftWire *p);
    static uint8_t readScl(const SoftWire *p);

    // SMBus uses CRC-8 for its PEC
    static uint8_t crc8_update(uint8_t crc, uint8_t data);

    SoftWire(uint8_t sda, uint8_t scl);
    inline uint8_t getSda(void) const;
    inline uint8_t getScl(void) const;
    inline uint8_t getDelay_us(void) const;
    inline uint16_t getTimeout_ms(void) const;
    inline uint8_t getInputMode(void) const;

    // begin() must be called after any changes are made to SDA and/or
    // SCL pins.
    inline void setSda(uint8_t sda);
    inline void setScl(uint8_t scl);
    inline void enablePullups(bool enablePullups = true);

    inline void setDelay_us(uint8_t delay_us);
    inline void setTimeout_ms(uint16_t timeout_ms);

    // begin() must be called before use, and after any changes are made
    // to the SDA and/or SCL pins.
    void begin(void) const;
    void end(void); // Restore pins to inputs

    // Functions which take raw addresses (ie address passed must
    // already indicate read/write mode)
    result_t llStart(uint8_t rawAddr) const;
    result_t llRepeatedStart(uint8_t rawAddr) const;
    result_t llStartWait(uint8_t rawAddr) const;

    result_t stop(bool allowClockStretch = true) const;

    inline result_t startRead(uint8_t addr) const;
    inline result_t startWrite(uint8_t addr) const;
    inline result_t repeatedStartRead(uint8_t addr) const;
    inline result_t repeatedStartWrite(uint8_t addr) const;
    inline result_t startReadWait(uint8_t addr) const;
    inline result_t startWriteWait(uint8_t addr) const;

    inline result_t start(uint8_t addr, mode_t rwMode) const;
    inline result_t repeatedStart(uint8_t addr, mode_t rwMode) const;
    inline result_t startWait(uint8_t addr, mode_t rwMode) const;

    result_t llWrite(uint8_t data) const;
    result_t llRead(uint8_t &data, bool sendAck = true) const;
    inline result_t readThenAck(uint8_t &data) const;
    inline result_t readThenNack(uint8_t &data) const;

    inline void sdaLow(void) const;
    inline void sdaHigh(void) const;
    inline void sclLow(void) const;
    inline void sclHigh(void) const;
    inline bool sclHighAndStretch(AsyncDelay& timeout) const;


    // Setters to override functions which control the SDA and SCL pins
    inline void setSetSdaLow(void (*sdaLowFn)(const SoftWire*)) {
      _sdaLow = sdaLowFn;
    }
    inline void setSetSdaHigh(void (*sdaHighFn)(const SoftWire*)) {
      _sdaHigh = sdaHighFn;
    }
    inline void setSetSclLow(void (*sclLowFn)(const SoftWire*)) {
      _sclLow = sclLowFn;
    }
    inline void setSetSclHigh(void (*sclHighFn)(const SoftWire*)) {
      _sclHigh = sclHighFn;
    }

    // Setters to override the functions which read the status of SDA and SCL
    inline void setReadSda(uint8_t (*readSdaFn)(const SoftWire*)) {
      _readSda = readSdaFn;
    }
    inline void setReadScl(uint8_t (*readSclFn)(const SoftWire*)) {
      _readScl = readSclFn;
    }

    // Wrapper functions to provide direct compatibility with the Wire library (TwoWire class)
    virtual int available(void);
    virtual size_t write(uint8_t data);
    virtual size_t write(const uint8_t *data, size_t quantity);
    virtual int read(void);
    virtual int peek(void);
    inline virtual void flush (void) {
      // TODO: (to be implemented in Wire)
    }

    void setClock(uint32_t frequency); // Approximate frequency in Hz
    void beginTransmission(uint8_t address);
    inline void beginTransmission(int address) {
      beginTransmission((uint8_t)address);
    }

    uint8_t endTransmission(uint8_t);
    uint8_t endTransmission(void) {
      return endTransmission(true);
    }


    uint8_t requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop = true);
    // inline uint8_t requestFrom(int address, int quantity) {
    //     return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
    // }

    // The Wire compatibility functions require RX and TX buffers. The same address space may be used for both
    // as long as the user does not call receiveFrom between startTransmission and endTransmission.
    inline void setRxBuffer(void *rxBuffer, uint8_t rxBufferSize) {
      _rxBuffer = (uint8_t*)rxBuffer;
      _rxBufferSize = rxBufferSize;
      _rxBufferIndex = 0;
      _rxBufferBytesRead = 0;
    }

    inline void setTxBuffer(void *txBuffer, uint8_t txBufferSize) {
      _txBuffer = (uint8_t*)txBuffer;
      _txBufferSize = txBufferSize;
      _txBufferIndex = 0;
    }

  private:
    uint8_t _sda;
    uint8_t _scl;
    uint8_t _inputMode;
    uint8_t _delay_us;
    uint16_t _timeout_ms;

    // Additional member variables to support compatibility with Wire library
    uint8_t *_rxBuffer;
    uint8_t _rxBufferSize;
    uint8_t _rxBufferIndex;
    uint8_t _rxBufferBytesRead;

    uint8_t _txAddress; // The address where data is to be sent to
    uint8_t *_txBuffer; // Address of user-supplied buffer
    uint8_t _txBufferSize; // Size of user-supplied buffer
    uint8_t _txBufferIndex; // Index into buffer
    mutable bool _transmissionInProgress;

    void (*_sdaLow)(const SoftWire *p);
    void (*_sdaHigh)(const SoftWire *p);
    void (*_sclLow)(const SoftWire *p);
    void (*_sclHigh)(const SoftWire *p);
    uint8_t (*_readSda)(const SoftWire *p);
    uint8_t (*_readScl)(const SoftWire *p);


    uint8_t endTransmissionInner(void) const;
};


uint8_t SoftWire::getSda(void) const
{
  return _sda;
}


uint8_t SoftWire::getScl(void) const
{
  return _scl;
}


uint8_t SoftWire::getDelay_us(void) const
{
  return _delay_us;
}


uint16_t SoftWire::getTimeout_ms(void) const
{
  return _timeout_ms;
}


uint8_t SoftWire::getInputMode(void) const
{
  return _inputMode;
}


void SoftWire::setSda(uint8_t sda)
{
  _sda = sda;
}


void SoftWire::setScl(uint8_t scl)
{
  _scl = scl;
}


void SoftWire::enablePullups(bool enable)
{
  _inputMode = (enable ? INPUT_PULLUP : INPUT);
}


void SoftWire::setDelay_us(uint8_t delay_us)
{
  _delay_us = delay_us;
}


void SoftWire::setTimeout_ms(uint16_t timeout_ms)
{
  _timeout_ms = timeout_ms;
}


SoftWire::result_t SoftWire::startRead(uint8_t addr) const
{
  return llStart((addr << 1) + readMode);
}


SoftWire::result_t SoftWire::startWrite(uint8_t addr) const
{
  return llStart((addr << 1) + writeMode);
}


SoftWire::result_t SoftWire::repeatedStartRead(uint8_t addr) const
{
  return llRepeatedStart((addr << 1) + readMode);
}


SoftWire::result_t SoftWire::repeatedStartWrite(uint8_t addr) const
{
  return llRepeatedStart((addr << 1) + writeMode);
}


SoftWire::result_t SoftWire::startReadWait(uint8_t addr) const
{
  return llStartWait((addr << 1) + readMode);
}


SoftWire::result_t SoftWire::startWriteWait(uint8_t addr) const
{
  return llStartWait((addr << 1) + writeMode);
}


SoftWire::result_t SoftWire::start(uint8_t addr, mode_t rwMode) const
{
  return llStart((addr << 1) + rwMode);
}


SoftWire::result_t SoftWire::repeatedStart(uint8_t addr, mode_t rwMode) const
{
  return llRepeatedStart((addr << 1) + rwMode);
}


SoftWire::result_t SoftWire::startWait(uint8_t addr, mode_t rwMode) const
{
  return llStartWait((addr << 1) + rwMode);
}


SoftWire::result_t SoftWire::readThenAck(uint8_t &data) const
{
  return llRead(data, true);
}


SoftWire::result_t SoftWire::readThenNack(uint8_t &data) const
{
  return llRead(data, false);
}


void SoftWire::sdaLow(void) const
{
  _sdaLow(this);
}


void SoftWire::sdaHigh(void) const
{
  _sdaHigh(this);
}


void SoftWire::sclLow(void) const
{
  _sclLow(this);
}


void SoftWire::sclHigh(void) const
{
  _sclHigh(this);
}


bool SoftWire::sclHighAndStretch(AsyncDelay& timeout) const
{
  _sclHigh(this);

  // Wait for SCL to actually become high in case the slave keeps
  // it low (clock stretching).
  while (_readScl(this) == LOW)
    if (timeout.isExpired()) {
      stop(false); // Reset bus. Do not allow clock stretching here
      return false;
    }

  return true;
}

#endif