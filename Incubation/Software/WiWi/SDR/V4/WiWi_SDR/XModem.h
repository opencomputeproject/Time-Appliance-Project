/*
 * XModem.h - Library for communicating using the XModem protocol (half-duplex)
 *
 * Protocal Resources:
 * https://www.menie.org/georges/embedded/xmodem_specification.html
 * http://wiki.synchro.net/ref:xmodem
 * http://pauillac.inria.fr/~doligez/zmodem/ymodem.txt
 */
#ifndef XModem_h
#define XModem_h
#include "Arduino.h"

//XModem constants
#define SOH (byte) 0x01 //Start of Header
#define EOT (byte) 0x04 //End of Transmission
#define ACK (byte) 0x06 //Acknowledge
#define NAK (byte) 0x15 //Negative Acknowledge
#define CAN (byte) 0x18 //Cancel Transmission
#define SUB (byte) 0x1A //Padding

class XModem {
  public:
    enum ProtocolType {
      XMODEM,
      CRC_XMODEM
    };

    XModem();
    void begin(HardwareSerial &serial, XModem::ProtocolType type = XModem::ProtocolType::XMODEM);
    void setIdSize(size_t size);
    void setChecksumSize(size_t size);
    void setDataSize(size_t size);
    void setSendInitByte(byte b);
    void setRetryLimit(byte limit);
    void setSignalRetryDelay(unsigned long ms);
    void allowNonSequentailBlocks(bool b);
    void bufferPacketReads(bool b);
    void setRecieveBlockHandler(bool (*handler) (void *blk_id, size_t idSize, byte *data, size_t dataSize));
    void setBlockLookupHandler(void (*handler) (void *blk_id, size_t idSize, byte *send_data, size_t dataSize));
    void setChksumHandler(void (*handler) (byte *data, size_t dataSize, byte *chksum));
    bool receive();
    bool send(byte data[], size_t data_len);
    bool send(byte data[], size_t data_len, unsigned long long start_id);
    bool lookup_send(unsigned long long id);

    struct bulk_data {
      byte **data_arr;
      size_t *len_arr;
      byte *id_arr; //each id is _id_bytes long in big endian format
      size_t count;
    };

    bool send_bulk_data(struct bulk_data container);

  private:
    HardwareSerial *_serial;
    byte _rx_init_byte;
    size_t _id_bytes;
    size_t _chksum_bytes;
    size_t _data_bytes;
    byte retry_limit;
    unsigned long _signal_retry_delay_ms;
    bool _allow_nonsequential;
    bool _buffer_packet_reads;
    bool (*process_rx_block) (void *blk_id, size_t id_bytes, byte *data, size_t dataSize);
    void (*block_lookup) (void *blk_id, size_t id_bytes, byte *data, size_t dataSize);
    void (*calc_chksum) (byte *data, size_t dataSize, byte *chksum);

    //NOTE: The function definitions for these in the cpp file don't include
    //      the static keyword because static is an overloaded keyword, here it means
    //      that the function is a class function while in the cpp file it means that
    //      the method definition is scoped only to its own file (plus is invalid in C++)
    static bool dummy_rx_block_handler(void *blk_id, size_t idSize, byte *data, size_t dataSize);
    static void dummy_block_lookup(void *blk_id, size_t idSize, byte *data, size_t dataSize);
    static void basic_chksum(byte *data, size_t dataSize, byte *chksum);
    static void crc_16_chksum(byte *data, size_t dataSize, byte *chksum);

    struct packet {
      byte *id;
      byte *chksum;
      byte *data;
    };

    bool init_rx();
    bool find_header();
    bool rx();
    bool read_block(struct packet *p, byte *buffer);
    bool read_block_buffered(struct packet *p, byte *buffer);
    bool read_block_unbuffered(struct packet *p);
    bool fill_buffer(byte *buffer, size_t bytes);

    bool init_tx();
    bool tx(struct packet *p, byte *data, size_t data_len, byte *blk_id);
    void build_packet(struct packet *p, byte *id, byte *data, size_t data_len);
    bool send_packet(struct packet *p);
    bool close_tx();

    void increment_id(byte *id, size_t length);
    byte tx_signal(byte signal);
    byte rx_signal();
    bool find_byte_timed(byte b, byte timeout_secs);
};

#endif
