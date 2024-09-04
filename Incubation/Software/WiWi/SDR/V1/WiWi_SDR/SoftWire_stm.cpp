// If possible disable interrupts whilst switching pin direction. Sadly
// there is no generic Arduino function to read the current interrupt
// status, only to enable and disable interrupts.  As a result the
// protection against spurious signals on the I2C bus is only available
// for AVR architectures where ATOMIC_BLOCK is defined.

#if defined(ARDUINO_ARCH_AVR)
#include <util/atomic.h>
#endif

#include "SoftWire_stm.h"


// Force SDA low
void SoftWire::sdaLow(const SoftWire *p)
{
  uint8_t sda = p->getSda();

  {
    wwvb_digital_write(sda, LOW);
    wwvb_gpio_pinmode(sda, OUTPUT);
  }
}


// Release SDA to float high
void SoftWire::sdaHigh(const SoftWire *p)
{
  wwvb_gpio_pinmode(p->getSda(), p->getInputMode());
}


// Force SCL low
void SoftWire::sclLow(const SoftWire *p)
{
  uint8_t scl = p->getScl();

  {
    wwvb_digital_write(scl, LOW);
    wwvb_gpio_pinmode(scl, OUTPUT);
  }
}


// Release SCL to float high
void SoftWire::sclHigh(const SoftWire *p)
{
  wwvb_gpio_pinmode(p->getScl(), p->getInputMode());
}


// Read SDA (for data read)
uint8_t SoftWire::readSda(const SoftWire *p)
{
  return wwvb_digital_read(p->getSda());
}


// Read SCL (to detect clock-stretching)
uint8_t SoftWire::readScl(const SoftWire *p)
{
  return wwvb_digital_read(p->getScl());
}


// For testing the CRC-8 calculator may be useful:
// http://smbus.org/faq/crc8Applet.htm
uint8_t SoftWire::crc8_update(uint8_t crc, uint8_t data)
{
  const uint16_t polynomial = 0x107;
  crc ^= data;
  for (uint8_t i = 8; i; --i) {
    if (crc & 0x80)
      crc = (uint16_t(crc) << 1) ^ polynomial;
    else
      crc <<= 1;
  }

  return crc;
}


SoftWire::SoftWire(uint8_t sda, uint8_t scl) :
  _sda(sda),
  _scl(scl),
  _inputMode(INPUT), // Pullups disabled by default
  _delay_us(defaultDelay_us),
  _timeout_ms(defaultTimeout_ms),
  _rxBuffer(NULL),
  _rxBufferSize(0),
  _rxBufferIndex(0),
  _rxBufferBytesRead(0),
  _txAddress(8),  // First non-reserved address
  _txBuffer(NULL),
  _txBufferSize(0),
  _txBufferIndex(0),
  _transmissionInProgress(false),
  _sdaLow(sdaLow),
  _sdaHigh(sdaHigh),
  _sclLow(sclLow),
  _sclHigh(sclHigh),
  _readSda(readSda),
  _readScl(readScl)
{
  ;
}


void SoftWire::begin(void) const
{
  /*
    // Release SDA and SCL
    _sdaHigh(this);
    delayMicroseconds(_delay_us);
    _sclHigh(this);
  */
  stop();
}


SoftWire::result_t SoftWire::stop(bool allowClockStretch) const
{
  AsyncDelay timeout(_timeout_ms, AsyncDelay::MILLIS);
  _transmissionInProgress = false;

  // Force SCL low
  _sclLow(this);
  delayMicroseconds(_delay_us);

  // Force SDA low
  _sdaLow(this);
  delayMicroseconds(_delay_us);

  // Release SCL
  if (allowClockStretch) {
    if (!sclHighAndStretch(timeout))
      return timedOut;
  } else {
    sclHigh();
  }
  delayMicroseconds(_delay_us);

  // Release SDA
  _sdaHigh(this);
  delayMicroseconds(_delay_us);

  return ack;
}


SoftWire::result_t SoftWire::llStart(uint8_t rawAddr) const
{
  //Serial.print("LLStart 0x");
  //Serial.println(rawAddr,HEX);

  // Force SDA low
  _sdaLow(this);
  delayMicroseconds(_delay_us);

  // Force SCL low
  _sclLow(this);
  delayMicroseconds(_delay_us);
  return llWrite(rawAddr);
}


SoftWire::result_t SoftWire::llRepeatedStart(uint8_t rawAddr) const
{
  AsyncDelay timeout(_timeout_ms, AsyncDelay::MILLIS);
  //.print("llRepeatedStart 0x");
  //Serial.println(rawAddr,HEX);
  
  // Force SCL low
  _sclLow(this);
  delayMicroseconds(_delay_us);

  // Release SDA
  _sdaHigh(this);
  delayMicroseconds(_delay_us);

  // Release SCL
  if (!sclHighAndStretch(timeout))
    return timedOut;
  delayMicroseconds(_delay_us);

  // Force SDA low
  _sdaLow(this);
  delayMicroseconds(_delay_us);

  return llWrite(rawAddr);
}


SoftWire::result_t SoftWire::llStartWait(uint8_t rawAddr) const
{
  AsyncDelay timeout(_timeout_ms, AsyncDelay::MILLIS);
  //Serial.print("llStartWait 0x");
  //Serial.println(rawAddr,HEX);
  while (!timeout.isExpired()) {
    // Force SDA low
    _sdaLow(this);
    delayMicroseconds(_delay_us);

    switch (llWrite(rawAddr)) {
      case ack:
        return ack;
      case nack:
        stop();
        return nack;
      default:
        // timeout, and anything else we don't know about
        stop();
        return timedOut;
    }
  }
  return timedOut;
}


SoftWire::result_t SoftWire::llWrite(uint8_t data) const
{
  //Serial.print("llWrite 0x");
  //Serial.println(data,HEX);
  AsyncDelay timeout(_timeout_ms, AsyncDelay::MILLIS);
  for (uint8_t i = 8; i; --i) {
    // Force SCL low
    _sclLow(this);

    if (data & 0x80) {
      // Release SDA
      _sdaHigh(this);
    }
    else {
      // Force SDA low
      _sdaLow(this);
    }
    delayMicroseconds(_delay_us);

    // Release SCL
    if (!sclHighAndStretch(timeout))
      return timedOut;

    delayMicroseconds(_delay_us);

    data <<= 1;
    if (timeout.isExpired()) {
      stop(); // Reset bus
      return timedOut;
    }
  }

  // Get ACK
  // Force SCL low
  _sclLow(this);

  // Release SDA
  _sdaHigh(this);

  delayMicroseconds(_delay_us);

  // Release SCL
  if (!sclHighAndStretch(timeout))
    return timedOut;

  result_t res = (_readSda(this) == LOW ? ack : nack);

  delayMicroseconds(_delay_us);

  // Keep SCL low between bytes
  _sclLow(this);

  return res;
}


SoftWire::result_t SoftWire::llRead(uint8_t &data, bool sendAck) const
{
  data = 0;
  AsyncDelay timeout(_timeout_ms, AsyncDelay::MILLIS);
  
  //Serial.print("llRead sendAck=");
  //Serial.println(sendAck,HEX);
  for (uint8_t i = 8; i; --i) {
    data <<= 1;

    // Force SCL low
    _sclLow(this);

    // Release SDA (from previous ACK)
    _sdaHigh(this);
    delayMicroseconds(_delay_us);

    // Release SCL
    if (!sclHighAndStretch(timeout))
      return timedOut;
    delayMicroseconds(_delay_us);

    // Read clock stretch
    while (_readScl(this) == LOW)
      if (timeout.isExpired()) {
        stop(); // Reset bus
        return timedOut;
      }

    if (_readSda(this))
      data |= 1;
  }

  // Put ACK/NACK
  // Force SCL low
  _sclLow(this);
  if (sendAck) {
    // Force SDA low
    _sdaLow(this);
  }
  else {
    // Release SDA
    _sdaHigh(this);
  }

  delayMicroseconds(_delay_us);

  // Release SCL
  if (!sclHighAndStretch(timeout))
    return timedOut;
  delayMicroseconds(_delay_us);

  // Wait for SCL to return high
  while (_readScl(this) == LOW)
    if (timeout.isExpired()) {
      stop(); // Reset bus
      return timedOut;
    }

  delayMicroseconds(_delay_us);

  // Keep SCL low between bytes
  _sclLow(this);

  return ack;
}


int SoftWire::available(void)
{
  return _rxBufferBytesRead - _rxBufferIndex;
}


size_t SoftWire::write(uint8_t data)
{
  if (_txBufferIndex >= _txBufferSize) {
    setWriteError();
    return 0;
  }

  _txBuffer[_txBufferIndex++] = data;
  return 1;
}


// Unlike the Wire version this function returns the actual amount of data written into the buffer
size_t SoftWire::write(const uint8_t *data, size_t quantity)
{
  size_t r = 0;
  for (size_t i = 0; i < quantity; ++i) {
    r += write(data[i]);
  }
  return r;
}


int SoftWire::read(void)
{
  if (_rxBufferIndex < _rxBufferBytesRead)
    return _rxBuffer[_rxBufferIndex++];
  else
    return -1;
}


int SoftWire::peek(void)
{
  if (_rxBufferIndex < _rxBufferBytesRead)
    return _rxBuffer[_rxBufferIndex];
  else
    return -1;
}


// Restore pins to inputs, with no pullups
void SoftWire::end(void)
{
  enablePullups(false);
  _sdaHigh(this);
  _sclHigh(this);
}


void SoftWire::setClock(uint32_t frequency)
{
  uint32_t period_us = uint32_t(1000000UL) / frequency;
  if (period_us < 2)
    period_us = 2;
  else if (period_us > 2 * 255)
    period_us = 2 * 255;

  setDelay_us(period_us / 2);
}


void SoftWire::beginTransmission(uint8_t address)
{
  _txAddress = address;
  _txBufferIndex = 0;
}


uint8_t SoftWire::endTransmission(uint8_t sendStop)
{
  uint8_t r = endTransmissionInner();
  if (sendStop)
    stop();
  else
    _transmissionInProgress = true;
  return r;
}


uint8_t SoftWire::endTransmissionInner(void) const
{
  result_t r;
  if (_transmissionInProgress) {
    r = repeatedStart(_txAddress, writeMode);
  } else {
    r = start(_txAddress, writeMode);
  }
  if (r == nack)
    return 2;
  else if (r == timedOut)
    return 4;

  for (uint8_t i = 0; i < _txBufferIndex; ++i) {
    r = llWrite(_txBuffer[i]);
    if (r == nack)
      return 3;
    else if (r == timedOut)
      return 4;
  }

  return 0;
}


uint8_t SoftWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop)
{
  result_t r;
  _rxBufferIndex = 0;
  _rxBufferBytesRead = 0;

  if (_transmissionInProgress) {
    r = repeatedStart(address, readMode);
  } else {
    r = start(address, readMode);
  }

  if (r == ack) {
    for (uint8_t i = 0; i < quantity; ++i) {
      if (i >= _rxBufferSize)
        break; // Don't write beyond buffer
      result_t res = llRead(_rxBuffer[i], i != (quantity - 1));
      if (res != ack)
        break;

      ++_rxBufferBytesRead;
    }
  }

  if (sendStop) {
    stop();
  } else {
    _transmissionInProgress = true;
  }

  return _rxBufferBytesRead;
}