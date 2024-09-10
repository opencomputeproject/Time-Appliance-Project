#ifndef WIWI_NETWORK_H
#define WIWI_NETWORK_H


#include "WWVB_Arduino.h"
#include "WiWi_Data.h"
#include "SiTime.h"
#include "WiWi_control.h"
#include "WiWi_masterAnchor.h"
#include "WiWi_clientAnchor.h"
#include "WiWi_tag.h"


#define DEBUG_PRINT_WIWI_NETWORK 


#define WIWI_MODE_MASTER_ANCHOR 0
#define WIWI_MODE_SLAVE_ANCHOR 1
#define WIWI_MODE_TAG 2




// WiWi Client State machine states
#define WIWI_START 0 // haven't done anything
#define WIWI_SEARCH 1 // try to find another module
#define WIWI_RUNNING 2 // connected to another module, normal operation, listen to new modules as well

extern unsigned long master_request_interval; // millis counter
extern uint8_t wiwi_state;
extern uint8_t wiwi_network_mode;


void send_announce_message();
void send_tag_broadcast();
void wiwi_client_send_master_request(bool retransmit);
void wiwi_receive_valid_master_announce(packet * pkt, wiwi_pkt_announce * announce_pkt);


void wiwi_network_setup();

// top level wiwi network function 
void run_wiwi_network();

#endif
