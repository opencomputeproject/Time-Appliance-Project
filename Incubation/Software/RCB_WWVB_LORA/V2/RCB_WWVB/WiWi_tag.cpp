

#include "WiWi_tag.h"

static bool tag_isSubscribed = 0;



void tag_sendSubcribe()
{
	// send subscribe
	if ( tx_packet_list.size() >= PACKET_BUFFER_SIZE ) {
		Serial.println("tag_sendSubcribe, tx packet list full");
		return; // packet buffer full
	}
	if ( free_packet_list.size() == 0 ){
		Serial.println("tag_sendSubcribe, free packet list empty");
		return;
	} // no free buffers
	Serial.print("tag_sendSubcribe start\r\n"); Serial.flush();

	int single_packet_index = free_packet_list.shift(); // get a free packet buffer
	packet * single_packet = &packet_buffer[single_packet_index];
	wiwi_pkt_hdr * wiwi_pkt = (wiwi_pkt_hdr*)single_packet->data;

	wiwi_pkt->wiwi_id = 0x6977;
	wiwi_pkt->mac_src = wiwi_mac_addr;
	wiwi_pkt->mac_dest = 0x0; 
	wiwi_pkt->pkt_type = WIWI_PKT_TAG_SUBSCRIBE;
	wiwi_pkt->seq_num = 0;
	wiwi_pkt->ack_num = 0;
	wiwi_pkt->checksum = 0;
	for ( int i = 0; i < sizeof(wiwi_pkt_hdr) -2; i++ ) {
		wiwi_pkt->checksum ^= ((uint8_t*)wiwi_pkt)[i];
	}

	single_packet->pkt_len = sizeof(wiwi_pkt_hdr);
	single_packet->phase.intval = 0;
	single_packet->timestamp = 0;

	sprintf(print_buffer, "tag_sendSubcribe before buffer add %d index %d\r\n", single_packet->pkt_len, single_packet_index);
	Serial.print(print_buffer); Serial.flush();
	for ( int i = 0; i < single_packet->pkt_len; i++ ) {
		sprintf(print_buffer, "0x%x ", single_packet->data[i]);
		Serial.print(print_buffer);
	}
	Serial.println("");
	// push this packet into linked list to send out 
	tx_packet_list.add(single_packet_index);
}



void tag_handleReceiveAnnounce(packet * single_packet)
{
	// similar to client anchor 
	// keep track of schedule 
	
	// determine if I need to send a subscribe message
	// if I'm not in the list of subscribers, send a subscribe message with some random delay between 1 and 50 milliseconds idk 
	wiwi_pkt_announce * annc_pkt = (wiwi_pkt_announce*) single_packet->data;
	t_tagSubscriptionInfo * tagInfo;
	uint8_t mac;
	bool master_tracking_mac = 0;
	

	for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ ) {
		if ( annc_pkt->tagList[i] == wiwi_mac_addr ) {
			// i'm in sub list , good
			Serial.println("tag got announce message, I'm in sub list already, good!");
			tag_isSubscribed = 1;
			return;
		}
	}
	// I'm not a subscriber
	int my_delay = 0;
	my_delay = random(1,250);
	// do a small delay
	sprintf(print_buffer,"tag sending subscribe message with random delay %d\r\n", my_delay);
	Serial.print(print_buffer);
	delay(my_delay);
	tag_sendSubcribe();	
}

static uint8_t rcvdDelayReqSeqNum = 0;

void tag_handleDelayResp(packet * single_packet)
{
	// for tag, just need to keep track of schedule 
	wiwi_pkt_delay_req * wiwi_pkt = (wiwi_pkt_delay_req*) single_packet->data;
	if ( wiwi_pkt->hdr.mac_src != 0 ) {
		Serial.println("Delay request not from mac 0! END"); return;
	}
	Serial.println("tag handleDelayReq start!");
	
	// need to make the schedule and also keep track of all the tags in the sub list 
	// tags are first
	sprintf(print_buffer, "Tag count subscribed: %d\r\n", wiwi_pkt->num_tag_responses_requested);
	Serial.print(print_buffer);
	
	uint8_t * mac = wiwi_pkt->data; // just the first mac 
	
	start_delayReq_schedule();
	for ( int i = 0; i < wiwi_pkt->num_tag_responses_requested; i++ ) {
		add_mac_to_schedule(*mac, 1); // 1 for tag, 0 for client anchor 
		mac += 1;
	}
	wiwi_pkt_anchor_info * ancInfo = (wiwi_pkt_anchor_info*) mac;
	for ( int i = 0; i < wiwi_pkt->num_anchor_responses_requested; i++ ) {
		add_mac_to_schedule(ancInfo->anchor_mac, 0);
		ancInfo += 1;
	}	
	rcvdDelayReqSeqNum = wiwi_pkt->hdr.seq_num;
	
	Serial.println("tag handleDelayReq end!");	
}


// very simple for tag 
// just send a single packet 
static uint8_t tagDelayRespSeqNum = 0;
void tag_sendDelayResp()
{
	// send subscribe
	if ( tx_packet_list.size() >= PACKET_BUFFER_SIZE ) {
		Serial.println("tag_sendDelayResp, tx packet list full");
		return; // packet buffer full
	}
	if ( free_packet_list.size() == 0 ){
		Serial.println("tag_sendDelayResp, free packet list empty");
		return;
	} // no free buffers
	Serial.print("tag_sendDelayResp start\r\n"); Serial.flush();

	int single_packet_index = free_packet_list.shift(); // get a free packet buffer
	packet * single_packet = &packet_buffer[single_packet_index];
	wiwi_pkt_hdr * wiwi_pkt = (wiwi_pkt_hdr*)single_packet->data;

	wiwi_pkt->wiwi_id = 0x6977;
	wiwi_pkt->mac_src = wiwi_mac_addr;
	wiwi_pkt->mac_dest = 0x0; 
	wiwi_pkt->pkt_type = WIWI_PKT_TAG_RESPONSE;
	wiwi_pkt->seq_num = tagDelayRespSeqNum++;
	wiwi_pkt->ack_num = rcvdDelayReqSeqNum;
	wiwi_pkt->checksum = 0;
	for ( int i = 0; i < sizeof(wiwi_pkt_hdr) -2; i++ ) {
		wiwi_pkt->checksum ^= ((uint8_t*)wiwi_pkt)[i];
	}

	single_packet->pkt_len = sizeof(wiwi_pkt_hdr);
	single_packet->phase.intval = 0;
	single_packet->timestamp = 0;

	sprintf(print_buffer, "tag_sendDelayResp before buffer add %d index %d\r\n", single_packet->pkt_len, single_packet_index);
	Serial.print(print_buffer); Serial.flush();
	for ( int i = 0; i < single_packet->pkt_len; i++ ) {
		sprintf(print_buffer, "0x%x ", single_packet->data[i]);
		Serial.print(print_buffer);
	}
	Serial.println("");
	// push this packet into linked list to send out 
	tx_packet_list.add(single_packet_index);
}	

void tag_handleDelayReq(packet * single_packet)
{
	// for tag, keep track of schedule

	wiwi_pkt_delay_resp * respPkt = (wiwi_pkt_delay_resp*) single_packet->data;
	
	// push this into schedule
	schedule_see_mac_response(respPkt->hdr.mac_src);
	
	sprintf(print_buffer, "tag, see delay response mac 0x%x\r\n\r\n", respPkt->hdr.mac_src);
	Serial.print(print_buffer);
}


static bool firstRun = 1;
void run_wiwi_network_tag()
{
	if ( firstRun ) {
		firstRun = 0;
		initSchedule();
	}

	if ( schedule_is_my_turn() ) {
		Serial.println("Tag my time to talk!");
		tag_sendDelayResp();		
		schedule_used_my_slot();
	}
}
