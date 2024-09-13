#ifndef WIWI_CLIENTANCHOR_H
#define WIWI_CLIENTANCHOR_H


#include "WWVB_Arduino.h"
#include "WiWi_Data.h"
#include "SiTime.h"
#include "WiWi_control.h"



void clientAnchor_handleSentDelayResp(packet * single_packet); 
void clientAnchor_handleReceiveAnnounce(packet * single_packet); 
void clientAnchor_handleDelayResp(packet * single_packet);
void clientAnchor_handleDelayReq(packet * single_packet);
void clientAnchor_handleTagResponse(packet * single_packet);

void clientAnchor_handleFullWiWiData();



void run_wiwi_network_client();




#endif

