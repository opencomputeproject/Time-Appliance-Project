#ifndef WIWI_TAG_H
#define WIWI_TAG_H




#include "WWVB_Arduino.h"
#include "WiWi_Data.h"
#include "SiTime.h"
#include "WiWi_control.h"




void tag_handleReceiveAnnounce(packet * single_packet);

void tag_handleDelayResp(packet * single_packet);

void tag_handleDelayReq(packet * single_packet);

void run_wiwi_network_tag();


#endif