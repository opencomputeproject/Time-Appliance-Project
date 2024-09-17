
#include "WiWi_clientAnchor.h"




static bool clientAnchor_isSubscribed = 0;
void clientAnchor_sendSubcribe()
{
	// send subscribe
	if ( tx_packet_list.size() >= PACKET_BUFFER_SIZE ) {
		Serial.println("clientAnchor_sendSubcribe, tx packet list full");
		return; // packet buffer full
	}
	if ( free_packet_list.size() == 0 ){
		Serial.println("clientAnchor_sendSubcribe, free packet list empty");
		return;
	} // no free buffers
	Serial.print("clientAnchor_sendSubcribe start\r\n"); Serial.flush();

	int single_packet_index = free_packet_list.shift(); // get a free packet buffer
	packet * single_packet = &packet_buffer[single_packet_index];
	wiwi_pkt_hdr * wiwi_pkt = (wiwi_pkt_hdr*)single_packet->data;

	wiwi_pkt->wiwi_id = 0x6977;
	wiwi_pkt->mac_src = wiwi_mac_addr;
	wiwi_pkt->mac_dest = 0x0; 
	wiwi_pkt->pkt_type = WIWI_PKT_ANCHOR_SUBSCRIBE;
	wiwi_pkt->seq_num = 0;
	wiwi_pkt->ack_num = 0;
	wiwi_pkt->checksum = 0;
	for ( int i = 0; i < sizeof(wiwi_pkt_hdr) -2; i++ ) {
		wiwi_pkt->checksum ^= ((uint8_t*)wiwi_pkt)[i];
	}

	single_packet->pkt_len = sizeof(wiwi_pkt_hdr);
	single_packet->phase.intval = 0;
	single_packet->timestamp = 0;

	sprintf(print_buffer, "clientAnchor_sendSubcribe before buffer add %d index %d\r\n", single_packet->pkt_len, single_packet_index);
	Serial.print(print_buffer); Serial.flush();
	for ( int i = 0; i < single_packet->pkt_len; i++ ) {
		sprintf(print_buffer, "0x%x ", single_packet->data[i]);
		Serial.print(print_buffer);
	}
	Serial.println("");
	// push this packet into linked list to send out 
	tx_packet_list.add(single_packet_index);
}

void clientAnchor_handleReceiveAnnounce(packet * single_packet)
{
	// determine if I need to send a subscribe message
	// if I'm not in the list of subscribers, send a subscribe message with some random delay between 1 and 50 milliseconds idk 
	wiwi_pkt_announce * annc_pkt = (wiwi_pkt_announce*) single_packet->data;
	t_tagSubscriptionInfo * tagInfo;
	uint8_t mac;
	bool master_tracking_mac = 0;
	
	
	// clean up my sub list, client anchor follows master anchor list
	// if a tag a client anchor is tracking isn't in master list, drop it
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		// loop over the tag macs I'm tracking 
		if ( !masterAnchor_getValidTagSubFromZero(i, &mac) ) {
			break;
		}		
		master_tracking_mac = 0;
		for ( int j = 0; j < MAX_TAG_CONNECTIONS; j++ ) {
			if ( annc_pkt->tagList[j] == mac ) {
				// master is tracking this mac, I'm good 
				master_tracking_mac = 1;
				break;
			}
		}
		if ( !master_tracking_mac ) {
			// I have a tag in my sub list, but master isn't considering it
			// remove it 
			masterAnchor_invalidateTagSub(mac);
		}		
	}
	
	// update the tag subscription info with tags subscribed according to master
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		mac = annc_pkt->tagList[i];
		if ( masterAnchor_isTagSubbed( mac, &tagInfo ) ) {
			// tag is already being tracked, good 
		} else {
			// tag isn't being tracked, add it
			masterAnchor_startTagSub(mac);
		}
	}
	
	for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ ) {
		if ( annc_pkt->clientAnchorList[i] == wiwi_mac_addr ) {
			// i'm in sub list , good
			Serial.println("Client anchor got announce message, I'm in sub list already, good!");
			clientAnchor_isSubscribed = 1;
			return;
		}
	}
	// I'm not a subscriber
	int my_delay = 0;
	my_delay = random(1,250);
	// do a small delay
	sprintf(print_buffer,"Client anchor sending subscribe message with random delay %d\r\n", my_delay);
	Serial.print(print_buffer);
	delay(my_delay);
	clientAnchor_sendSubcribe();	
} 

// WiWi info for myself
static uint32_t prev_tx_ts;
static uint8_t rcvdDelayReqSeqNum = 0;

///////////////////////////////// Client anchor wiwi management data 



void clientAnchor_handleDelayReq(packet * single_packet)
{
	wiwi_pkt_delay_req * wiwi_pkt = (wiwi_pkt_delay_req*) single_packet->data;
	if ( wiwi_pkt->hdr.mac_src != 0 ) {
		Serial.println("Delay request not from mac 0! END"); return;
	}
	Serial.println("clientAnchor handleDelayReq start!");
	
	// need to make the schedule and also keep track of all the tags in the sub list 
	// tags are first
	sprintf(print_buffer, "Tag count subscribed: %d\r\n", wiwi_pkt->num_tag_responses_requested);
	Serial.print(print_buffer);
	sprintf(print_buffer, "Anchor count subscribed: %d\r\n", wiwi_pkt->num_anchor_responses_requested);
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
		if ( ancInfo->anchor_mac == wiwi_mac_addr ) {
			// client anchor, make a new entry 
			Anchor_startNewRcvdWiWiEntry( client_wiwidata,
				ANCHOR_WIWI_DATA_HISTORY,
				wiwi_pkt->hdr.seq_num,
				0 );
			
			// put this data in 
			if ( Anchor_newRcvdWiwi_dat( client_wiwidata,
				ANCHOR_WIWI_DATA_HISTORY,
				wiwi_pkt->hdr.seq_num, 
				single_packet->phase.intval,
				ancInfo->prev_phi_ab.intval, 
				ancInfo->prev_phi_aa.intval, 0 ) != -1 ) {
				Serial.println("************** WIWI HAS FULL DATA *******************");
			}
		}
		ancInfo += 1;
	}	
	rcvdDelayReqSeqNum = wiwi_pkt->hdr.seq_num;
	//prev_phi_ab.intval = single_packet->phase.intval;
	
	printAnchorData(client_wiwidata, ANCHOR_WIWI_DATA_HISTORY);
	sprintf(print_buffer, "*******Client Anchor, Store Master anchor delay req, seq=%d, phase=0x%x\r\n",
		wiwi_pkt->hdr.seq_num, single_packet->phase.intval);
	Serial.print(print_buffer);
	
	Serial.println("clientAnchor handleDelayReq end!");	
}






void clientAnchor_handleSentDelayResp(packet * single_packet)
{
	wiwi_pkt_hdr * single_hdr;
	single_hdr = (wiwi_pkt_hdr *)single_packet->data;
	wiwi_pkt_delay * delay_pkt = (wiwi_pkt_delay*) single_packet->data;
	
	// just my reply back to master anchor
	// just need to store phase and timestamp when sending to master anchor
	//phi_bb.intval = single_packet->phase.intval;
	prev_tx_ts = single_packet->timestamp;
	
	Anchor_newTxWiWi( client_wiwidata,
		ANCHOR_WIWI_DATA_HISTORY,
		single_hdr->seq_num, single_packet->phase.intval, 0);
	
	printAnchorData(client_wiwidata, ANCHOR_WIWI_DATA_HISTORY);
	sprintf(print_buffer, "*******Client anchor, sent Delay resp, phase=0x%x time=%ld\r\n",
		single_packet->phase.intval, prev_tx_ts);
	Serial.print(print_buffer);
	
	
}


static uint8_t delayRespSeqNum = 0;
void clientAnchor_sendDelayResp()
{
	// need to send my delay response to master 
	if ( tx_packet_list.size() >= PACKET_BUFFER_SIZE ) {
		Serial.println("clientAnchor_sendDelayResp, tx packet list full");
		return; // packet buffer full
	}
	if ( free_packet_list.size() == 0 ){
		Serial.println("clientAnchor_sendDelayResp, free packet list empty");
		return;
	} // no free buffers
	Serial.print("clientAnchor_sendDelayResp start\r\n"); Serial.flush();

	int single_packet_index = free_packet_list.shift(); // get a free packet buffer
	packet * single_packet = &packet_buffer[single_packet_index];
	wiwi_pkt_delay * wiwi_pkt = (wiwi_pkt_delay*)single_packet->data;

	wiwi_pkt->hdr.wiwi_id = 0x6977;
	wiwi_pkt->hdr.mac_src = wiwi_mac_addr;
	wiwi_pkt->hdr.mac_dest = 0x0; 
	wiwi_pkt->hdr.pkt_type = WIWI_PKT_DELAY_RESP;
	wiwi_pkt->hdr.seq_num = delayRespSeqNum;
	wiwi_pkt->hdr.ack_num = rcvdDelayReqSeqNum;
	wiwi_pkt->hdr.checksum = 0;
	for ( int i = 0; i < sizeof(wiwi_pkt_hdr) -2; i++ ) {
		wiwi_pkt->hdr.checksum ^= ((uint8_t*)wiwi_pkt)[i];
	}

	single_packet->pkt_len = sizeof(wiwi_pkt_delay_resp); // dynamic packet size, need to increase this later
	single_packet->phase.intval = 0;
	single_packet->timestamp = 0;
	
	// my info sharing with anchor 
	wiwi_pkt->my_prev_info.anchor_mac = 0;
	wiwi_pkt->my_prev_info.prev_phi_ab.intval = get_prev_rcvd_phase(client_wiwidata,
		ANCHOR_WIWI_DATA_HISTORY, delayRespSeqNum, 0);
	wiwi_pkt->my_prev_info.prev_phi_aa.intval = get_prev_tx_phase(client_wiwidata,
		ANCHOR_WIWI_DATA_HISTORY, delayRespSeqNum-1, 0);
	

	wiwi_pkt->my_prev_info.previous_tx_ts = prev_tx_ts;
	sprintf(print_buffer, "Client anchor delay resp, prev_phi_ab=0x%x, prev_phi_aa=0x%x, ts=0x%x\r\n",
		get_prev_rcvd_phase(client_wiwidata, ANCHOR_WIWI_DATA_HISTORY, delayRespSeqNum, 0), 
		get_prev_tx_phase(client_wiwidata, ANCHOR_WIWI_DATA_HISTORY, delayRespSeqNum, 0), 
		prev_tx_ts);
	Serial.print(print_buffer);
	
	// info about all the tags I've seen 
	wiwi_pkt->num_tags_seen = 0;
	uint8_t mac = 0;
	uint8_t seq_num = 0;
	uint32_t phase = 0;
	uint32_t ts = 0;
	wiwi_pkt_tag_hdr_info * tag_hdr = (wiwi_pkt_tag_hdr_info*) wiwi_pkt->data;
	t_wiwi_tag_data * tag_data = (t_wiwi_tag_data *) tag_hdr->data;
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) { // loop over each tag
		if ( !masterAnchor_getValidTagSubFromZero(i, &mac) ) {
			break;
		}
		for ( int j = 0; j < MAX_HISTORY_PER_TAG; j++ ) { // loop over each history point per tag
			if ( !masterAnchor_getTagHistory(mac, j, &seq_num, &phase, &ts) ) {
				//update pointer to next tag hdr 
				if ( j != 0 ) { // had a history point added, 
					tag_hdr = (wiwi_pkt_tag_hdr_info*) ( (uint8_t*) tag_data );
				}				
				break;
			}
			// j == 0, new tag, and has at least one valid history point, increment packet counter
			if ( j == 0 ) {
				wiwi_pkt->num_tags_seen++;
				single_packet->pkt_len += 2; // tag_mac / history_count bytes 
			}
			sprintf(print_buffer, "Client anchor delay resp, tag mac 0x%x history %d, seq=0x%x, phase=0x%x, ts=0x%x\r\n",
				mac, j, seq_num, phase, ts);
			Serial.println(print_buffer);
			tag_hdr->tag_mac = mac;	
			tag_hdr->history_count = j+1;			
			
			tag_data->phase.intval = phase;
			tag_data->rx_timestamp = ts;
			tag_data->seq_num = seq_num; 				
			
			single_packet->pkt_len += 9; // one entry , 4 bytes phase, 4 bytes timestamp, 1 byte seq num

			// update pointer
			tag_data += 1; 
		}
	}
	*((uint8_t*)tag_data) = 0; // append one byte of zero
	single_packet->pkt_len++;
	*(((uint8_t*)tag_data)+1) = 0; // append one byte of zero
	single_packet->pkt_len++;
	*(((uint8_t*)tag_data)+2) = 0; // append one byte of zero
	single_packet->pkt_len++;
	*(((uint8_t*)tag_data)+3) = 0; // append one byte of zero
	single_packet->pkt_len++;
	delayRespSeqNum++;
	

	sprintf(print_buffer, "clientAnchor_sendDelayResp before buffer add %d index %d\r\n", single_packet->pkt_len, single_packet_index);
	Serial.print(print_buffer); Serial.flush();
	for ( int i = 0; i < single_packet->pkt_len; i++ ) {
		sprintf(print_buffer, "0x%x ", single_packet->data[i]);
		Serial.print(print_buffer);
	}
	Serial.println("");
	// push this packet into linked list to send out 
	tx_packet_list.add(single_packet_index);
	
}

void clientAnchor_handleDelayResp(packet * single_packet)
{

	wiwi_pkt_delay_resp * respPkt = (wiwi_pkt_delay_resp*) single_packet->data;
	
	// push this into schedule
	schedule_see_mac_response(respPkt->hdr.mac_src);
	
	sprintf(print_buffer, "Client anchor, see delay response mac 0x%x\r\n\r\n", respPkt->hdr.mac_src);
	Serial.print(print_buffer);
	
	// not sure what else to do here, basically another client anchor's delay response to master
	// can use later for positioning or clock characterization of other anchors?
	// nothing needed for bare minimum demo 
}





void clientAnchor_handleTagResponse(packet * single_packet)
{
	wiwi_pkt_tag_response * tagPkt = (wiwi_pkt_tag_response*) single_packet->data;
	
	// push this into schedule 
	schedule_see_mac_response(tagPkt->hdr.mac_src);
	
	sprintf(print_buffer, "Client anchor handle tag response mac 0x%x\r\n", tagPkt->hdr.mac_src);
	Serial.print(print_buffer);
	
	// store this packets meta data 
	// reuse the master tag subscription stuff 
	masterAnchor_pushTagHistory(tagPkt->hdr.mac_src, tagPkt->hdr.seq_num,
		single_packet->phase.intval,
		single_packet->timestamp);	
}



void clientAnchor_handleFullWiWiData()
{
	static int last_complete_index = -1;
	int cur_latest_complete = -1;
	t_Anchor_selfWiWiData * wiwiData;
	
	cur_latest_complete = get_newest_wiwi_complete_index(client_wiwidata, ANCHOR_WIWI_DATA_HISTORY);
	
	if ( cur_latest_complete != -1 && cur_latest_complete != last_complete_index ) {
		Serial.println("clientAnchor handleFullWiWidata!");
		last_complete_index = cur_latest_complete;
		wiwiData = &client_wiwidata[cur_latest_complete];
		sprintf(print_buffer, "ClientAnchor handleFullWiwidata new index %d, aa=0x%x, ab=0x%x, ba=0x%x, bb=0x%x\r\n",
			cur_latest_complete, wiwiData->phi_aa.intval, wiwiData->phi_ab.intval,
			wiwiData->phi_ba.intval, wiwiData->phi_bb.intval);
		// compute phi_c and phi_d
		Anchor_wiwi_compute_unwrapped_phi_c_d( wiwiData->phi_aa.value, wiwiData->phi_ab.value, wiwiData->phi_ba.value, 
			wiwiData->phi_bb.value, 0, 0);
	}
}



static bool firstRun = 1;
static unsigned long lastCleanTime = 0;
void run_wiwi_network_client()
{
	if ( firstRun ) {
		firstRun = 0;
		initSchedule();
		initAnchorWiWidata(client_wiwidata, ANCHOR_WIWI_DATA_HISTORY);
		client_unwrapped_phi_c.intval = 0;
		client_unwrapped_phi_d.intval = 0;
		client_prev_phi_c.intval = 0;
		client_prev_phi_d.intval = 0;
	}
	
	if ( millis() - lastCleanTime > 1000 ) {
		lastCleanTime = millis();
		// dont worry about clean up, just leave the code stub here in case
	}
	
	if ( schedule_is_my_turn() ) {
		Serial.println("Client anchor my time to talk!");
		clientAnchor_sendDelayResp();
		
		schedule_used_my_slot();
	}
}