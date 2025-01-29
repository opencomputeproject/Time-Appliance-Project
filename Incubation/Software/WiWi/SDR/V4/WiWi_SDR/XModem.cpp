#include "Arduino.h"
#include "XModem.h"

XModem::XModem() {}

//NOTE: the type argument has a default value - see header file
void XModem::begin(HardwareSerial &serial, XModem::ProtocolType type) {
  _serial = &serial;
  switch(type) {
    case ProtocolType::XMODEM:
      _id_bytes = 1;
      _chksum_bytes = 1;
      _data_bytes = 128;
      _rx_init_byte = NAK;
      calc_chksum = XModem::basic_chksum;
      break;
    case ProtocolType::CRC_XMODEM:
      _id_bytes = 1;
      _chksum_bytes = 2;
      _data_bytes = 128;
      _rx_init_byte = 'C';
      calc_chksum = XModem::crc_16_chksum;
      break;
  }
  retry_limit = 10;
  _signal_retry_delay_ms = 100;
  _allow_nonsequential = false;
  _buffer_packet_reads = true;
  process_rx_block = XModem::dummy_rx_block_handler;
  block_lookup = XModem::dummy_block_lookup;
}

// SETTERS
void XModem::setIdSize(size_t size) {
  _id_bytes = size;
}

void XModem::setChecksumSize(size_t size) {
  _chksum_bytes = size;
}

void XModem::setDataSize(size_t size) {
  _data_bytes = size;
}

void XModem::setSendInitByte(byte b) {
  _rx_init_byte = b;
}

void XModem::setRetryLimit(byte limit) {
  retry_limit = limit;
}

void XModem::setSignalRetryDelay(unsigned long ms) {
  _signal_retry_delay_ms = ms;
}

void XModem::allowNonSequentailBlocks(bool b) {
  _allow_nonsequential = b;
}

void XModem::bufferPacketReads(bool b) {
  _buffer_packet_reads = b;
}

void XModem::setRecieveBlockHandler(bool (*handler) (void *blk_id, size_t idSize, byte *data, size_t dataSize)) {
  process_rx_block = handler;
}

void XModem::setBlockLookupHandler(void (*handler) (void *blk_id, size_t idSize, byte *send_data, size_t dataSize)) {
  block_lookup = handler;
}

void XModem::setChksumHandler(void (*handler) (byte *data, size_t dataSize, byte *chksum)) {
  calc_chksum = handler;
}

// PUBLIC METHODS
bool XModem::receive() {
  if(!init_rx() || !rx()) {
    //An unrecoverable error occured send cancels to terminate the transaction
    _serial->write(CAN);
    _serial->write(CAN);
    _serial->write(CAN);
    return false;
  }
  return true;
}

bool XModem::lookup_send(unsigned long long id) {
  return send((byte*) NULL, 0, id);
}

bool XModem::send(byte *data, size_t data_len, unsigned long long start_id) {
  byte *id = (byte *) malloc(_id_bytes);

  //convert the start_id to big endian format
  unsigned long long temp = start_id;
  for(size_t i = 0; i < _id_bytes; ++i) {
    id[_id_bytes-i-1] = (byte) (temp & 0xFF);
    temp >>=8;
  }

  struct bulk_data container;
  container.data_arr = &data;
  container.len_arr = &data_len;
  container.id_arr = id;
  container.count = 1;

  bool result = send_bulk_data(container);
  free(id);
  return result;
}

bool XModem::send_bulk_data(struct bulk_data container) {
  if(container.count == 0) return false;

  struct packet p;

  //bundle all our memory allocations together
  //need to store:
  //2 id blocks - blk_id and packet struct
  //1 checksum block - packet struct
  //1 data block - packet struct
  byte *buffer = (byte *) malloc(2*_id_bytes + 1*_chksum_bytes + 1*_data_bytes);
  byte *blk_id = buffer + _data_bytes;
  p.id = blk_id + _id_bytes;
  p.chksum = p.id + _id_bytes;
  p.data = buffer;

  bool result = init_tx();
  for(size_t j = 0; result && j < container.count; ++j) {
    for(size_t i = 0; i < _id_bytes; ++i) blk_id[i] = container.id_arr[j*_id_bytes + i];
    result &= tx(&p, container.data_arr[j], container.len_arr[j], blk_id);
  }

  if(result) {
    result = close_tx();
  } else {
    //An unrecoverable error occured send cancels to terminate the transaction
    _serial->write(CAN);
    _serial->write(CAN);
    _serial->write(CAN);
  }

  free(buffer);
  return result;
}

bool XModem::send(byte *data, size_t data_len) {
  return send(data, data_len, 1);
}

// INTERNAL RECEIVE METHODS
bool XModem::init_rx() {
  byte i = 0;
  do {
    _serial->write(_rx_init_byte);
    if(find_byte_timed(SOH, 10)) return true;
  } while(i++ < retry_limit);
  return false;
}

bool XModem::find_header() {
  byte i = 0;
  do {
    if(i != 0) _serial->write(NAK);
    if(find_byte_timed(SOH, 10)) return true;
  } while(i++ < retry_limit);
  return false;
}

bool XModem::rx() {
  bool result = false;

  byte *buffer;
  byte *prev_blk_id;
  byte * expected_id;
  struct packet p;

  //bundle all our memory allocations together
  if(_buffer_packet_reads) {
    //need to store:
    //5 id blocks - prev_blk_id, expected_id, packet struct, buffer id and buffer compl_id
    //2 chksum block - packet struct and buffer chksum
    //2 data block - packet struct and buffer data
    buffer = (byte *) malloc(5*_id_bytes + 2*_chksum_bytes + 2*_data_bytes);

    prev_blk_id = buffer + 2*_id_bytes + _chksum_bytes + _data_bytes;
  } else {
    //need to store:
    //3 id blocks - prev_blk_id, expected_id and packet struct
    //1 checksum block - packet struct
    //1 data block - packet struct
    buffer = (byte *) malloc(3*_id_bytes + _chksum_bytes + _data_bytes);
    prev_blk_id = buffer;
  }

  expected_id = prev_blk_id + _id_bytes;
  p.id = expected_id + _id_bytes;
  p.chksum = p.id + _id_bytes;
  p.data = p.chksum + _chksum_bytes;

  for(size_t i = 0; i < _id_bytes; ++i) prev_blk_id[i] = expected_id[i] = 0;

  byte errors = 0;
  while(true) {
    if(read_block(&p, buffer)) {
      //reset errors
      errors = 0;

      //ignore resends of the last received block
      size_t matches = 0;
      for(size_t i = 0; i < _id_bytes; ++i) {
        if(prev_blk_id[i] == p.id[i]) ++matches;
      }

      //if its a duplicate block we still need to send an ACK
      if(matches != _id_bytes) {
        if(_allow_nonsequential) {
          for(size_t i = 0; i < _id_bytes; ++i) expected_id[i] = p.id[i];
        } else {
          increment_id(expected_id, _id_bytes);

          matches = 0;
          for(size_t i = 0; i < _id_bytes; ++i) {
            if(expected_id[i] == p.id[i]) ++matches;
          }

          if(matches != _id_bytes) break;
        }

        size_t padding_bytes = 0;
        //count number of padding SUB bytes
        while(p.data[_data_bytes - 1 - padding_bytes] == SUB) ++padding_bytes;

        //process packet
        if(!process_rx_block(p.id, _id_bytes, p.data, _data_bytes - padding_bytes)) break;

        for(size_t i = 0; i < _id_bytes; ++i) prev_blk_id[i] = expected_id[i];
      }

      //signal acknowledgment
      byte response = tx_signal(ACK);
      if(response == CAN) break;
      if(response == EOT) {
        response = tx_signal(NAK);
        if(response == CAN) break; // This is not strictly neccessary
        if(response == EOT) {
          _serial->write(ACK);
          result = true;
          break;
        }
      }
      // Unexpected response and resync attempt failed so fail out
      if(response != SOH && !find_header()) break;
    } else {
      if(++errors > retry_limit) break;
      byte response = tx_signal(NAK);
      if(response == CAN) break;
      if(response != SOH && !find_header()) break;
    }
  }

  free(buffer);
  return result;
}

bool XModem::read_block(struct packet *p, byte *buffer) {
  if(_buffer_packet_reads) {
    return read_block_buffered(p, buffer);
  } else {
    return read_block_unbuffered(p);
  }
}

bool XModem::read_block_buffered(struct packet *p, byte *buffer) {
  size_t b_pos = 2*_id_bytes + _data_bytes + _chksum_bytes;
  if(!fill_buffer(buffer, b_pos)) return false;

  b_pos = 0;
  for(size_t i = 0; i < _id_bytes; ++i) {
    p->id[i] = buffer[b_pos++];
    //Because of C integer promotion rules the ~ operator changes
    //the variable type of an unsigned char (byte) to a char so we need to
    //cast it back
    if(p->id[i] != (byte) ~buffer[b_pos++]) return false;
  }

  for(size_t i = 0; i < _data_bytes; ++i) {
    p->data[i] = buffer[b_pos++];
  }

  calc_chksum(p->data, _data_bytes, p->chksum);
  for(size_t i = 0; i < _chksum_bytes; ++i) {
    if(p->chksum[i] != buffer[b_pos++]) return false;
  }

  return true;
}

bool XModem::read_block_unbuffered(struct packet *p) {
  byte tmp;
  for(size_t i = 0; i < _id_bytes; ++i) {
    if(!_serial->readBytes(p->id + i, 1)) return false;
    if(!_serial->readBytes(&tmp, 1)) return false;

    //Because of C integer promotion rules the ~ operator changes
    //the variable type of an unsigned char (byte) to a char so we need to
    //cast it back
    if(p->id[i] != (byte) ~tmp) return false;
  }

  if(!fill_buffer(p->data, _data_bytes)) return false;

  calc_chksum(p->data, _data_bytes, p->chksum);
  for(size_t i = 0; i < _chksum_bytes; ++i) {
    if(!_serial->readBytes(&tmp, 1)) return false;
    if(p->chksum[i] != tmp) return false;
  }

  return true;
}

bool XModem::fill_buffer(byte *buffer, size_t bytes) {
  size_t count = 0;
  while(count < bytes) {
    size_t r = _serial->readBytes(buffer + count, bytes - count);

    //the baud rate / sending device may be much slower than ourselves so we
    //only signal an error condition if no data has been received at all within
    //the serial timeout period
    if(r == 0) return false;

    count += r;
  }
  return true;
}

// INTERNAL SEND METHODS
bool XModem::init_tx() {
  byte i = 0;
  do {
    if(find_byte_timed(_rx_init_byte, 60)) return true;
  } while(i++ < retry_limit);
  return false;
}

bool XModem::tx(struct packet *p, byte *data, size_t data_len, byte *blk_id) {
  byte *data_ptr = data;
  byte *data_end = data_ptr + data_len;

  //flush incoming data before starting
  while(_serial->available()) _serial->read();

  if(data == NULL) {
    //need to use block_lookup to fill in the packet data
    build_packet(p, blk_id, NULL, _data_bytes);
    return send_packet(p);
  }

  while(data_ptr + _data_bytes < data_end) {
    build_packet(p, blk_id, data_ptr, _data_bytes);
    increment_id(blk_id, _id_bytes);
    if(!send_packet(p)) return false;
    data_ptr += _data_bytes;
  }

  if(data_ptr != data_end) {
    memset(p->data, SUB, _data_bytes);

    build_packet(p, blk_id, data_ptr, data_end - data_ptr);
    if(!send_packet(p)) return false;
  }

  return true;
}

void XModem::build_packet(struct packet *p, byte *id, byte *data, size_t data_len) {
  memcpy(p->id, id, _id_bytes);
  if(data == NULL) block_lookup(id, _id_bytes, p->data, data_len);
  else memcpy(p->data, data, data_len);
  calc_chksum(p->data, _data_bytes, p->chksum);
}

bool XModem::send_packet(struct packet *p) {
  byte tries = 0;
  do {
    _serial->write(SOH);

    for(size_t i = 0; i < _id_bytes; ++i) {
      _serial->write(p->id[i]);
      _serial->write(~p->id[i]);
    }

    _serial->write(p->data, _data_bytes);
    _serial->write(p->chksum, _chksum_bytes);

    byte response = rx_signal();
    if(response == ACK) return true;
    if(response == NAK) continue;
    if(response == CAN) {
      response = rx_signal();
      if(response == CAN) break;
    }
  } while(tries++ < retry_limit);

  return false;
}

bool XModem::close_tx() {
  byte error_responses = 0;
  while(error_responses < retry_limit) {
    byte response = tx_signal(EOT);
    if(response == ACK) return true;
    if(response == NAK) continue;
    if(response == CAN) {
      if(rx_signal() == CAN) break;
    } else ++error_responses;
  }
  return false;
}

// INTERNAL SHARED METHODS
void XModem::increment_id(byte *id, size_t length) {
  size_t index = length-1;
  do {
    id[index]++;
    if(id[index]) return; //if the current byte is non-zero then there is no overflow and we are done
  } while(index--); //when our index is zero before decrementing then we have incremented all the bytes
}

byte XModem::tx_signal(byte signal) {
  if(signal == NAK) {
    //flush to make sure the line is clear
    while(_serial->available()) _serial->read();
  }
  byte i = 0;
  byte val;
  do {
    byte read_attempt = 0;
    _serial->write(signal);
    while(_serial->readBytes(&val, 1) == 0 && read_attempt++ < retry_limit) delay(_signal_retry_delay_ms);

    switch(val) {
      case SOH:
      case EOT:
      case CAN:
      case ACK:
      case NAK:
        return val;
    }
  } while(++i < retry_limit);
  return 255;
}

byte XModem::rx_signal() {
  byte i = 0;
  byte val;
  while(_serial->readBytes(&val, 1) == 0 && ++i < retry_limit) delay(_signal_retry_delay_ms);

  switch(val) {
    case ACK:
    case NAK:
    case CAN:
      return val;
  }
  return 255;
}

bool XModem::find_byte_timed(byte b, byte timeout_secs) {
  unsigned long end = millis() + ((unsigned long) timeout_secs * 1000UL);
  do {
    if(_serial->find(b)) return true;
  } while(millis() < end);
  return false;
}

// DEFAULT HANDLERS
bool XModem::dummy_rx_block_handler(void *blk_id, size_t idSize, byte *data, size_t dataSize) {
  return true;
}

void XModem::dummy_block_lookup(void *blk_id, size_t idSize, byte *send_data, size_t dataSize) {
  memset(send_data, 0x3A, dataSize);
}

void XModem::basic_chksum(byte *data, size_t dataSize, byte *chksum) {
  byte sum = 0;
  for(size_t i = 0; i < dataSize; ++i) sum += data[i];
  *chksum = sum;
}

void XModem::crc_16_chksum(byte *data, size_t dataSize, byte *chksum) {
  //XModem CRC prime number is 69665 -> 2^16 + 2^12 + 2^5 + 2^0 -> 10001000000100001 -> 0x11021
  //normal notation of this bit pattern omits the leading bit and represents it as 0x1021
  //in code we can omit the 2^16 term due to shifting before XORing when the MSB is a 1
  const unsigned short crc_prime = 0x1021;
  unsigned short *crc = (unsigned short *) chksum;
  *crc = 0;

  //We can ignore crc calulations that cross byte boundaries by just assuming
  //that the following byte is 0 and then fixup our simplification at the end
  //by XORing in the true value of the next byte into the most sygnificant byte
  //of the CRC
  for(size_t i = 0; i < dataSize; ++i) {
    *crc ^= (((unsigned short) data[i]) << 8);
    for(byte j = 0; j < 8; ++j) {
      if(*crc & 0x8000) *crc = (*crc << 1) ^ crc_prime;
      else *crc <<= 1;
    }
  }
}
