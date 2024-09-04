#ifndef WIWI_DATA_H
#define WIWI_DATA_H


#include "WWVB_Arduino.h"
#include <stdint.h>
#include "Wire.h"
#include <Arduino.h>
#include <LinkedList.h> // Arduino Linkedlist library
#include <math.h>

#define ENABLE_WIWI_STACK 1 // DEBUG FLAG

//#define VERBOSE_PRINT 1

/************ WiWi OTA packets ************************/

// Local timestamps: 
//    1. T3 , when I sent my last request
//    2. T2 , when I received the last response
// Remote timestamps:
//    1. T4, when they received my last request
//    2. T1, when they sent their last response


#define WIWI_PKT_DELAY_REQ 0xa
#define WIWI_PKT_DELAY_RESP 0xb
#define WIWI_PKT_ANNOUNCE 0xc

union phaseUnion {
  byte array[4];
  uint32_t intval; 
  float value;
} ;

// 9 bytes
typedef struct __attribute__ ((packed)) wiwi_pkt_hdr {
  uint32_t wiwi_id; // fixed value, 'w' 'i' 'w' 'i' , 0x77697769
  uint8_t mac_src;
  uint8_t mac_dest;
  uint8_t pkt_type;
  uint8_t seq_num; // put TCP style inside WiWi as well
  uint8_t ack_num;
} wiwi_pkt_hdr;

// Delay Req and Delay Resp are same format
// 9 + 1 + 4 + 4 + 4 + 4 + 14 bytes = 40 bytes
typedef struct __attribute__ ((packed)) wiwi_pkt_delay {
  wiwi_pkt_hdr hdr; // 9 bytes
  uint8_t flags; // 0 if previous values are not valid, 1 if t3 is valid, 2 is t2 is valid, 3 if t3 and t2 are valid
  uint32_t previous_tx_ts; // previous delay request tx timestamp on local tx
  phaseUnion previous_tx_iq; 
  uint32_t previous_rx_ts; // previous sync response rx timestamp on local rx
  phaseUnion previous_rx_iq;
  uint8_t checksum;
  uint8_t reserved[13];
} wiwi_pkt_delay;

// in theory can do frequency lock just from announce messages, include timestamps
// 9 + 1 + (4) + 4 + 2 + 20 = 40 bytes
typedef struct __attribute__ ((packed))  wiwi_pkt_announce {
  wiwi_pkt_hdr hdr; // 9 bytes
  uint8_t flags; // 0 for client announce, 1 for master announce 
  uint32_t previous_tx_time;
  phaseUnion previous_iq;
  uint8_t checksum;
  uint8_t unused[2];
  uint32_t reserved[4];
  uint8_t unusedtwo[3];
} wiwi_pkt_announce;


#define WIWI_PKT_ANNOUNCE_LEN (sizeof(wiwi_pkt_announce) ) 
#define WIWI_PKT_DELAY_LEN (sizeof(wiwi_pkt_delay))




/************* Packet data structures ***************/

// 1. a free buffer stack
//      simple int data structure, contains indexes in packet list that are free
// 2. rx buffer stack
//      simple int data structure, contains indexes in packet list for RX processing
// 3. tx buffer stack
//      simple int data structure, contains indexes in packet list for TX processing


// not super efficient at removal, come back to this later 
#define PACKET_BUFFER_SIZE 100
#define WIWI_MAX_PACKET_SIZE 64

// https://stackoverflow.com/questions/12991054/can-we-use-doubly-linked-list-in-c-without-dynamic-memory-allocation


// generic packet
// want timestamp and iq meta data for RX and TX packets
typedef struct __attribute__ ((packed))  packet {
	uint8_t data[WIWI_MAX_PACKET_SIZE]; // assume 64 byte max packet size
	uint8_t pkt_len;
	uint32_t timestamp;
	phaseUnion phase; // actually double, but uint64_t for data purposes
} packet;


extern LinkedList_git<int> free_packet_list;
extern LinkedList_git<int> rx_packet_list;
extern LinkedList_git<int> tx_packet_list;
extern packet packet_buffer[PACKET_BUFFER_SIZE]; 


/**************** Generic client structure, similar to TCP **********/
#define MAX_CONNECTIONS 10

// Define a structure to represent a client's connection state
typedef struct  ClientConnection {
  uint8_t clientMac; // A unique identifier for the client
  uint8_t sequenceNumber; // Sequence number for outgoing packets
  uint8_t ackNumber; // Acknowledgment number for incoming packets
	bool isValid; 
	unsigned long last_transmit_time; //millis last time transmitted
		// for retransmit timing
	unsigned long last_receive_time; //millis last time received a messages
		// for timeout purposes 
	uint8_t retransmit_count; // count how many times retransmitted for timeout
	packet last_tx_pkt[2]; // for retransmit purposes and wiwi purposes
	packet last_rx_delay_pkts[2]; 
  packet last_rx_announce_pkts[2]; // for WiWi purposes need two, last two sessions, [0] is most recent
  uint8_t announce_count;
	// WiWi can run when last_tx_pkt.seq_num == last_rx_pkt.ack_num
} ClientConnection;

// Define a structure to manage multiple client connections
typedef struct  ConnectionManager {
    ClientConnection clients[MAX_CONNECTIONS];
    int numClients; // Number of active clients
	ClientConnection master; // if client in RUNNING state, use this 
	uint8_t announce_seq_num;
	uint32_t last_announce_timestamp;
	phaseUnion last_announce_phase;
} ConnectionManager;



extern uint8_t wiwi_mac_addr;

//////// Client connections
extern ConnectionManager clientmanager; 

void ConnectionManager_Init(ConnectionManager* manager);
int ConnectionManager_AddClient(ConnectionManager* manager, int clientID);
ClientConnection* ConnectionManager_GetClient(ConnectionManager* manager, int clientID);
void ConnectionManager_UpdateAckNumber(ConnectionManager* manager, int clientID, int newAckNumber);
int ConnectionManager_CheckAckEqualsSeq(ConnectionManager* manager, int clientID);
int ConnectionManager_InvalidateClient(ConnectionManager* manager, int clientID);


#endif