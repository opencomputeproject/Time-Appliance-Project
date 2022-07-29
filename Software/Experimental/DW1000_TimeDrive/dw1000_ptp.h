


/***************************************
 * Julian defining a parallel protocol to typical UWB Ranging
 * Experimentally, found normal UWB Ranging scheme quite lossy and problematic
 * 
 * Implement a protocol similar to PTP (in multicast mode) and TCP
 * 
 * Each device, GUG or Time Stick, comes up and listens to traffic to 5 seconds
 * It determines what devices are present by analyzing destination and source addresses (2-octets only)
 * 
 * Addresses
 *  [15:12] = 0x0 for GUG, 0x1 for Time Stick, others reserved
 *  [11:0] = value picked by each device after listening
 *  
 *  
 *  THEORY SECTION NOT SURE, NEEDED FOR LARGER SCALE DEPLOYMENT
 * Address conflict resolution
 *  Any GUG is responsible to resolving conflicts 
 *  When a new device wants to join the network, it sends a request to any GUG that was talking during the 5 seconds
 *  with a desired source address. The GUG will then either send a reply back to that address approving that, or telling that device
 *  to retry ( DOES THIS WORK???? IMPLEMENT LATER )
 *  
 * GUGs enumerate each other, allowing for multiple GUGs in a single area. 
 *  GUGs each Broadcast "Sync+Followup" Messages, with the transmit timestamp embedded in the packet
 *  GUGs listen for a Delay Req message from other GUGs or Time Sticks and reply with their full RX TS
 * 
 * 
 * 
 * 
 * 
 * LAB TEST SETUP WITH JUST ONE "GUG" AND ONE "TIME STICK"
 * GUG (anchor) sends out sync + followup (x5, configurable) , back to back
 * GUG then listens for RANGING INIT and acts as a TAG waiting for ranging init
    to let Time Sticks estimate delay response
 */






// Standard UWB Packet Types
#define POLL_MESSAGE 0x61
#define RESPONSE_MESSAGE 0x50
#define FINAL_MESSAGE 0x69
#define RANGING_INIT_MESSAGE 0x20 

// Custom UWB Packet types!
#define SYNC_FOLLOWUP 0x23
#define DELAY_REQ 0x24
#define DELAY_RESP 0x25



// Finite state machine states for GUG and Time Stick
#define TD_IDLE 0
#define LISTEN 1
#define BROADCAST 2
#define RANGEFIND 3


// GUGs will broadcast this many sync+followups in order
#define NUM_SYNC_RETRANSMITS 5
#define TIME_BETWEEN_SYNCS_MSEC 50 


// Time stick config
#define TIME_BETWEEN_RANGEFIND_MSEC 10000

#define FREQ_RATIO_INVALID 9999999999




#ifndef _DW1000_PTP_H_
#define _DW1000_PTP_H_


#include <Arduino.h>
#include "DW1000.h"
#include "DW1000Constants.h"
#include "SoftSPI.h"



// Decawave pins 
extern const uint8_t PIN_DW_IRQ; // PA12
extern const uint8_t PIN_DW_FORCEON; // PA13
extern const uint8_t PIN_DW_WAKEUP; //PA14
extern const uint8_t PIN_DW_RST; // PA15
extern const uint8_t PIN_DW_MOSI; // PA16
extern const uint8_t PIN_DW_SCK; // PA17
extern const uint8_t PIN_DW_SS;  // PA18
extern const uint8_t PIN_DW_MISO; // PA19
extern SoftSPI mySPI;

extern SPIClass myHardSPI;

extern const uint8_t PIN_1PPS_UC_IN; //PB10


extern bool is_gug;


// Delay request / response similar to PTP is done via traditional UWB range finding

struct uwb_ptp_hdr {
  uint16_t frame_control;
  uint8_t seq_num; // general seq number between 0-255
  uint16_t pan_id; // DECA
  uint16_t dest_addr;
  uint16_t src_addr; 
  uint8_t function_code;  
} __attribute__((packed));

// cheating a bit, should be 64-bit addresses here 
struct uwb_range_init_pkt {
  struct uwb_ptp_hdr; 
  uint8_t reserved[4];
} __attribute__((packed));
#define RANGE_INIT_PKTSIZE (sizeof(struct uwb_ptp_hdr) + sizeof(uwb_range_init_pkt))

struct uwb_poll_pkt {
  struct uwb_ptp_hdr hdr;
} __attribute__((packed));
#define POLL_PKTSIZE (sizeof(struct uwb_ptp_hdr))

struct uwb_response_pkt {
  struct uwb_ptp_hdr hdr;
  uint32_t calculated_tof;
} __attribute__((packed));
#define RESPONSE_PKTSIZE (sizeof(struct uwb_ptp_hdr) + sizeof(uwb_response_pkt))

struct uwb_final_message_pkt {
  struct uwb_ptp_hdr hdr;
  uint32_t respRxTimeSubPollTX;
  uint32_t finalTxTimeSubRespRX;
} __attribute__((packed));
#define FINALMSG_PKTSIZE (sizeof(struct uwb_ptp_hdr) + sizeof(uwb_final_message_pkt))


// sync packet, has sync + previous followups
struct uwb_ptp_sync_followup_pkt {
  struct uwb_ptp_hdr hdr;
  uint8_t sync_num; // from 0 to NUM_SYNC_RETRANSMITS, also tells how many valid followups are present
  uint8_t num_syncs_sending; // constant, NUM_SYNC_RETRANSMITS
  uint8_t gps_time_sec[6]; // the 48 bit gross time from GPS
  uint8_t followups[NUM_SYNC_RETRANSMITS-1][LEN_STAMP]; // 40-bit timestamps of previous syncs sent  
  uint8_t local_micros[NUM_SYNC_RETRANSMITS-1][4]; // 32-bit micros from GUG , micros that the TX timestamp was read, just for reference 
}__attribute__((packed));

#define SYNCFOLLOWUP_PKTSIZE (sizeof(struct uwb_ptp_hdr) + sizeof(uwb_ptp_sync_followup_pkt))

// delay request packet, to GUG
struct uwb_ptp_delay_request_pkt {
  struct uwb_ptp_hdr hdr;
}__attribute__((packed));
#define DELAYREQUEST_PKTSIZE (sizeof(struct uwb_ptp_hdr))

// delay response packet, from GUG
struct uwb_ptp_delay_response_pkt {
  struct uwb_ptp_hdr hdr;
  uint8_t receivedTime[LEN_STAMP]; // 40-bit timestamp received delay request
}__attribute__((packed));
#define DELAYRESPONSE_PKTISZE (sizeof(struct uwb_ptp_hdr) + LEN_STAMP)






void print_int64t(int64_t val);


void decawave_ptp_init();
void TopLevelFSM();
void debug_dw_irq();

void BasicReceiver();
void BasicSender();

#endif
