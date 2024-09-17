#ifndef WIWI_MASTERANCHOR_H
#define WIWI_MASTERANCHOR_H


#include "WWVB_Arduino.h"
#include "WiWi_Data.h"
#include "SiTime.h"
#include "WiWi_control.h"



#define MASTER_ANNOUNCE_INTERVAL 20000
#define MASTER_ANNOUNCE_RESPONSE_GUARDBAND 200

void masterAnchor_handleSentAnnounce(packet * single_packet);
void masterAnchor_handleAnchorSubscribe(packet * single_packet);
void masterAnchor_handleTagSubscribe(packet * single_packet) ;
void masterAnchor_handleSentDelayReq(packet * single_packet) ;
void masterAnchor_handleDelayResp(packet * single_packet) ;
void masterAnchor_handleTagResponse(packet * single_packet);

void masterAnchor_handleFullWiWiData();

void init_masterAnchor();
void run_masterAnchor();



#endif

