#ifndef WIWI_NETWORK_H
#define WIWI_NETWORK_H


#include "WWVB_Arduino.h"
#include "WiWi_Data.h"
#include "SiTime.h"
#include "WiWi_control.h"


#define DEBUG_PRINT_WIWI_NETWORK 

/***************** Network pseudo code 

A lot of this code could be improved with rtos stuff
but just keeping it simple with arduino and loops 

******* AS CLIENT
1. Start state , just boot state 
	a. go to SEARCH state
2. Search state 
	a. Send broadcast announce messages periodically
	b. Listen for any other wiwi packets
	c. Hear WiWi packets
		- if I hear an announce packet with flag set as master
			follow them
			go to RUNNING mode 
		- if my MAC is higher than someone else, 
			follow them 
			go to RUNNING mode		
	d. If get DELAY_REQUEST packet
		- Add client to connections to track timestamps and IQ
		- Send DELAY_RESPONSE
	e. Monitor client list
		- If I haven't received a message from this client for 
			some time, then invalidate them
	
3. Running state
	a. Exchange Delay_Request / Delay_Response messages with master
		- adjust local oscillator to align with master 
	b. If get DELAY_REQUEST packet
		- Add client to connections to track timestamps and IQ
		- Send DELAY_RESPONSE
	c. master connection is a pseudo TCP connection
		- retransmit last request if don't get a reply in some time
		- if master does not reply to a certain number of retransmits
			forget the master, go back to search state 
	d. Monitor client list
		- If I haven't received a message from this client for 
			some time, then invalidate them

	


******* AS MASTER , only set manually , not random 
1. Start state, just boot state
	a. Go to RUNNING mode
2. Running mode
	a. Send broadcast messages periodically with flag set 
	b. If get DELAY_REQUEST packet
		- Add client to connections to track timestamps and IQ
		- Send DELAY_RESPONSE
	c. Monitor client list
		- If I haven't received a message from this client for 
			some time, then invalidate them

********************/


/********** Network State machine *************/



// WiWi Modes
#define WIWI_MODE_CLIENT 0
#define WIWI_MODE_MASTER 1


// WiWi Client State machine states
#define WIWI_START 0 // haven't done anything
#define WIWI_SEARCH 1 // try to find another module
#define WIWI_RUNNING 2 // connected to another module, normal operation, listen to new modules as well

extern unsigned long master_request_interval; // millis counter

void wiwi_network_setup();

// top level wiwi network function 
void run_wiwi_network();

#endif
