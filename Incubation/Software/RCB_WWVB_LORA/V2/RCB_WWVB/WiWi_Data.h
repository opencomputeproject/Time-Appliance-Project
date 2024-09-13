#ifndef WIWI_DATA_H
#define WIWI_DATA_H


#include "WWVB_Arduino.h"
#include <stdint.h>
#include "Wire.h"
#include <Arduino.h>
#include <LinkedList.h> // Arduino Linkedlist library
#include <math.h>
#include "WiWi_control.h"

#define ENABLE_WIWI_STACK 1 // DEBUG FLAG

//#define VERBOSE_PRINT 1

#define MAX_TAG_CONNECTIONS 10 // same count for client anchors and tags 
#define MAX_ANCHOR_CONNECTIONS 10
#define MAX_HISTORY_PER_TAG 10



extern uint8_t wiwi_mac_addr;



/************ WiWi OTA packets ************************/


#define WIWI_PKT_DELAY_REQ 0xa
#define WIWI_PKT_DELAY_RESP 0xb
#define WIWI_PKT_ANNOUNCE 0xc
#define WIWI_PKT_TAG_RESPONSE 0xd
#define WIWI_PKT_ANCHOR_SUBSCRIBE 0xe
#define WIWI_PKT_TAG_SUBSCRIBE 0xf

union phaseUnion {
  byte array[4];
  uint32_t intval; 
  float value;
} ;



typedef struct __attribute__ ((packed)) wiwi_pkt_anchor_info {
  uint8_t anchor_mac;
  phaseUnion prev_phi_aa; // bb for client anchor , client anchor is always b
  phaseUnion prev_phi_ab; // ba for client anchor 
  uint32_t previous_tx_ts; 
} wiwi_pkt_anchor_info;

typedef struct __attribute__ ((packed)) wiwi_tag_data {
	phaseUnion phase;
	uint32_t rx_timestamp;
	uint8_t seq_num;
} t_wiwi_tag_data;

typedef struct __attribute__ ((packed)) wiwi_pkt_tag_hdr_info {
  uint8_t tag_mac;
  uint8_t history_count;
  uint8_t data[1]; // at least one byte, can keep this in the structure
  // after this is variable number of phase and timestamps 
  // [ phaseUnion phi_aa, uint32_t rx_timestamp, uint8_t tag_seq_num ] [history_count]
  // 9 bytes per entry 
} wiwi_pkt_tag_hdr_info;

// 7 bytes
typedef struct __attribute__ ((packed)) wiwi_pkt_hdr {
  uint16_t wiwi_id; // fixed value, 'w' 'i' , 0x7769
  uint8_t mac_src;
  uint8_t mac_dest;
  uint8_t pkt_type;
  uint8_t seq_num; // put TCP style inside WiWi as well
  uint8_t ack_num;
  uint8_t checksum;
} wiwi_pkt_hdr;

typedef struct __attribute__ ((packed)) wiwi_pkt_tag_response {
  wiwi_pkt_hdr hdr;
} wiwi_pkt_tag_response;


// DYNAMIC PACKET SIZE 
typedef struct __attribute__ ((packed)) wiwi_pkt_delay_req {
  wiwi_pkt_hdr hdr; 
  uint8_t num_tag_responses_requested;
  uint8_t num_anchor_responses_requested; 
  
  // dynamic data size, but this is max size 
  // tag_response_macs go first, just one byte per tag mac requesting from
  // then anchor data structures next , with phase and time info as well 
  uint8_t data[(MAX_TAG_CONNECTIONS) + (MAX_TAG_CONNECTIONS*sizeof(wiwi_pkt_anchor_info)) ];
  //uint8_t tag_macs_requesting[MAX_CONNECTIONS];  // overprovision 
  //wiwi_pkt_anchor_info response_macs[MAX_CONNECTIONS]; // overprovision, don't use
  
  // will also append one byte of zero for wiwi purposes, for SF7 use case 
} wiwi_pkt_delay_req;

// DYNAMIC PACKET SIZE 
// data is of structure type wiwi_pkt_tag_hdr_info
// packet will contain variable number of entries, each of variable size
typedef struct __attribute__ ((packed)) wiwi_pkt_delay_resp {
  wiwi_pkt_hdr hdr; 
  wiwi_pkt_anchor_info my_prev_info;
  uint8_t num_tags_seen;
  uint8_t data[1]; 
} wiwi_pkt_delay;

// in theory can do frequency lock just from announce messages, include timestamps
// 9 + 1 + (4) + 4 + 2 + 20 = 40 bytes
typedef struct __attribute__ ((packed))  wiwi_pkt_announce {
  wiwi_pkt_hdr hdr; // 9 bytes
  uint8_t clientAnchorList[MAX_ANCHOR_CONNECTIONS]; 
  uint8_t tagList[MAX_TAG_CONNECTIONS];
  uint32_t reserved[2];
  uint8_t unusedtwo[1];
} wiwi_pkt_announce;


#define WIWI_PKT_ANNOUNCE_LEN (sizeof(wiwi_pkt_announce) ) 
#define WIWI_PKT_DELAY_LEN (sizeof(wiwi_pkt_delay))
#define WIWI_PKT_TAG_BROADCAST_LEN (sizeof(wiwi_pkt_tag_broadcast))




/************* Packet data structures ***************/

// 1. a free buffer stack
//      simple int data structure, contains indexes in packet list that are free
// 2. rx buffer stack
//      simple int data structure, contains indexes in packet list for RX processing
// 3. tx buffer stack
//      simple int data structure, contains indexes in packet list for TX processing


// not super efficient at removal, come back to this later 
#define PACKET_BUFFER_SIZE 100
#define WIWI_MAX_PACKET_SIZE 200

// https://stackoverflow.com/questions/12991054/can-we-use-doubly-linked-list-in-c-without-dynamic-memory-allocation


// generic packet
// want timestamp and iq meta data for RX and TX packets
typedef struct __attribute__ ((packed))  packet {
	uint8_t data[WIWI_MAX_PACKET_SIZE]; // assume max packet size
	uint8_t pkt_len;
	uint32_t timestamp;
	phaseUnion phase; // actually double, but uint64_t for data purposes
} packet;


extern LinkedList_git<int> free_packet_list;
extern LinkedList_git<int> rx_packet_list;
extern LinkedList_git<int> tx_packet_list;
extern packet packet_buffer[PACKET_BUFFER_SIZE]; 




/*************** Network structures *************/

#define MASTER_TAG_TIMEOUT 50000
#define MAX_ANCHORTAG_TIMESLOT 500

// Master anchor data structures


////////////////////// Used by master anchor and client anchors to manage tag subscriptions
typedef struct tagSubscriptionInfo {
	uint8_t tag_mac;
	bool valid;
	unsigned long last_response_seen; // millis
	bool valid_history[MAX_HISTORY_PER_TAG];
	uint8_t seq_num[MAX_HISTORY_PER_TAG];
	phaseUnion prev_phase[MAX_HISTORY_PER_TAG];
	uint32_t prev_rx_ts[MAX_HISTORY_PER_TAG];	
} t_tagSubscriptionInfo;

typedef struct tagSubscriptionList {
	t_tagSubscriptionInfo tagSubscriptionInfo[MAX_TAG_CONNECTIONS];
} tagSubscriptionList;

// functions for these 
extern tagSubscriptionList masterAnchor_TagSubs;

void masterAnchor_InitTagSubs(); // just init data structures
void masterAnchor_CleanupTagSubs(); // run periodically to invalidate old/stale
bool masterAnchor_isTagSubbed(uint8_t mac, t_tagSubscriptionInfo ** tag_info);
void masterAnchor_startTagSub(uint8_t mac); 
void masterAnchor_gotTagPacket(uint8_t mac); 
void masterAnchor_invalidateTagSub(uint8_t mac);

// higher level function, to allow looping over all valid tags 
bool masterAnchor_getValidTagSubFromZero(int number, uint8_t * mac); 
void masterAnchor_pushTagHistory(uint8_t mac, uint8_t seq_num, uint32_t phase, uint32_t ts);

// 0 for newest, 1 for next oldest, etc. 
bool masterAnchor_getTagHistory(uint8_t mac, uint8_t countFromNewest, uint8_t * seq_num,
	uint32_t * phase, uint32_t * ts);





///////////////// Used by anchors for wiwi ping pong data collection of phase / time
// master is keeping track of this as part of its client anchor subscription data


// a wiwi ping pong is defined by the actual packets that were measured over there air
// Master -> client , packet 0 , creates phi_aa_0 on master / phi_ab_0 on client , seq = 0 / ack = 0
// Client -> master , packet 1 , creates phi_bb_0 on client / phi_ba_0 on master , seq = 0 / ack = 0
// However, an additional ping pong is needed to transfer those values to both sides 
// Master -> client , packet 2, creates phi_aa_1 on master / phi_ab_1 on client , has phi_aa_0 / phi_ba_0 , seq = 1 / ack = 0
// Client -> master , packet 3, creates phi_bb_1 on client / phi_ba_1 on master , has phi_bb_0 / phi_ab_0 , seq = 1 / ack = 0
// not really using ack in my code, not worth it, just keep ping ponging and verify seq number is sequential
// need to collect all 4 phase measurements with respect to two packets from four packets 
// on client anchors, since they're going to do disciplining, keep a history as well
// the key for each set of four phases on client anchor is the seq number of request from master


#define ANCHOR_WIWI_DATA_HISTORY 10 
typedef struct __attribute__ ((packed)) t_Anchor_selfWiWiData {
	bool valid;
	bool complete; 
	uint8_t seq_num_start; 
	phaseUnion phi_aa;
	phaseUnion phi_ab;
	phaseUnion phi_bb;
	phaseUnion phi_ba; 	
	uint32_t time_started; 
	uint32_t time_completed;	
} t_Anchor_selfWiWiData;



// used by client anchors in WiWi with master anchor 
extern t_Anchor_selfWiWiData client_wiwidata[ANCHOR_WIWI_DATA_HISTORY];
extern phaseUnion client_unwrapped_phi_c;
extern phaseUnion client_unwrapped_phi_d;
extern phaseUnion client_prev_phi_c;
extern phaseUnion client_prev_phi_d;
extern uint64_t num_wiwi_calculations;

void printAnchorData(t_Anchor_selfWiWiData * arr,
	int count);
void initAnchorWiWidata(t_Anchor_selfWiWiData * arr, int count);

// for any received delay request from master, there will be the received phase, seq_num, 
// and the phase the master is giving about last time 
// returns true if this info gives a new full set of wiwi data to compute wiwi
int Anchor_newRcvdWiwi_dat(t_Anchor_selfWiWiData * arr, int count, 
	uint8_t seq_num, uint32_t rcvd_phase, uint32_t told_rx_phase,
	uint32_t told_tx_phase, bool isMaster);

// for every client anchor reply, there will be transmitted phase and sequence number 
// returns true if this info gives a new full set of wiwi data to compute wiwi 
void Anchor_newTxWiWi(t_Anchor_selfWiWiData * arr, 
	int count,
	uint8_t seq_num, uint32_t sent_phase, bool isMaster);
	


uint32_t get_prev_rcvd_phase(t_Anchor_selfWiWiData * arr,
	int count,
	uint8_t seq_num, bool isMaster);
uint32_t get_prev_tx_phase(t_Anchor_selfWiWiData * arr,
	int count,
	uint8_t seq_num, bool isMaster);
int get_newest_wiwi_complete_index(t_Anchor_selfWiWiData * arr, int count);

int Anchor_startNewRcvdWiWiEntry(t_Anchor_selfWiWiData * arr, int count,uint8_t seq_num,
	bool isMaster);









////////////////////// used by master anchor to manage anchors subscriptions

// info specific to this anchor subscription
typedef struct __attribute__ ((packed)) wiwi_anchor_info {
  uint8_t anchor_mac;
  bool valid;
  unsigned long last_response_seen; // millis
  t_Anchor_selfWiWiData ancData[ANCHOR_WIWI_DATA_HISTORY];
  uint32_t previous_tx_ts; 
  uint8_t sequenceNumber;
  uint8_t ackNumber;
  int last_complete_index;
  phaseUnion unwrapped_phi_c;
  phaseUnion unwrapped_phi_d;
  phaseUnion prev_phi_c;
  phaseUnion prev_phi_d;  
  uint64_t num_wiwi_calculations;
} wiwi_anchor_info;

// info from the client anchor for each tag its seen 
typedef struct __attribute__ ((packed)) wiwi_tag_info {
  uint8_t tag_mac; // mac of tag seen by anchor 
  bool valid; // is this data base entry valid 
  unsigned long last_data_seen; // millis 
  
  bool dataValid[MAX_HISTORY_PER_TAG]; // per history entry, which are valid
  phaseUnion prev_phase[MAX_HISTORY_PER_TAG]; // anchor phase measurement
  uint32_t previous_rx_ts[MAX_HISTORY_PER_TAG]; // anchor time measurement
  uint8_t prev_seq_num[MAX_HISTORY_PER_TAG];
} wiwi_tag_info;


typedef struct anchorSubscriptionInfo {
	wiwi_anchor_info anchorInfo;
	wiwi_tag_info anchorTagInfo[MAX_TAG_CONNECTIONS];	
} t_anchorSubscriptionInfo;
#define ANCH_SUBINFO_SIZE (sizeof(wiwi_anchor_info) + (sizeof(wiwi_tag_info) * MAX_TAG_CONNECTIONS)) 

typedef struct anchorSubscriptionList {
	t_anchorSubscriptionInfo anchorSubscriptionInfo[MAX_ANCHOR_CONNECTIONS];
} anchorSubscriptionList;


// functions for these 
extern anchorSubscriptionList masterAnchor_AnchorSubs;

void masterAnchor_InitAnchorSubs(); // just init data structure
void masterAnchor_CleanupAnchorSubs(); // run periodically to invalidate old/stale
bool masterAnchor_isAnchorSubbed(uint8_t mac, t_anchorSubscriptionInfo ** anchor_info);
void masterAnchor_startAnchorSub(uint8_t mac); 
void masterAnchor_gotAnchorPacket(uint8_t mac); 

// higher level function, to allow looping over all valid tags 
bool masterAnchor_getValidAnchorSubFromZero(int number, uint8_t * mac); 

void masterAnchor_pushclientAnchorTagHistory(wiwi_tag_info * tag_info, 
	t_wiwi_tag_data * tag_data);

void masterAnchor_pushAnchorTagData(uint8_t ancMac, uint8_t tagMac,
	t_wiwi_tag_data * tag_data);

bool masterAnchor_getclientAnchorTagData(uint8_t ancMac, uint8_t tagMac, 
	wiwi_tag_info ** tag_info);
void masterAnchor_pushclientAnchorTagData(uint8_t ancMac, uint8_t tagMac,
	t_wiwi_tag_data * tag_data);

int masterAnchor_get_newest_wiwi_complete_index(wiwi_anchor_info * ancInfo);




/////////////////// Used by client anchor and tags for scheduling
// simple schedule

#define SCHEDULE_COUNT (MAX_TAG_CONNECTIONS + MAX_ANCHOR_CONNECTIONS)
void initSchedule();
void start_delayReq_schedule();
void add_mac_to_schedule(uint8_t mac, bool isTag);
void schedule_see_mac_response(uint8_t mac);
uint32_t schedule_get_my_latest_time();
void schedule_used_my_slot();
bool schedule_is_my_turn();










///////////////////// Used to compute items from full WiWI phi data

void Anchor_wiwi_compute_unwrapped_phi_c_d(float phi_aa, float phi_ab, 
	float phi_ba, float phi_bb, bool isMaster,
	wiwi_anchor_info * ancInfo);



#endif