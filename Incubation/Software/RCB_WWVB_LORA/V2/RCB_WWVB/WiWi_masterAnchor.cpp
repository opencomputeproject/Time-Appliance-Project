
#include "WiWi_masterAnchor.h"


/********
master, call it a in all the phi variables

Master controls the entire network, network is idle until master talks

Master steps 

Step 1: Periodically broadcast announce message, in announce message is list of anchors in client list IN ORDER and list of tags IN ORDER
Step 2: Listen for client anchors or tags for 500ms after announce , limits how many can subscribe per cycle
		If get client anchor subscribe, add to client list
		If get tag subscribe, add to tag list

Step 3: Send out delay req periodically to a number of anchors / tags 
		include previous phi_aa
		include list of anchor macs and/or tags requesting from
		include previous phi_ab for each anchor mac 
Step 4: wait for delay resp from those anchors and/or tags
		It will be a sequence based on sequence requested from
		Each device will wait for the previous to transmit before sending theirs
		There's a timeout each device allocates to the previous, after that it will transmit its own
		If a device times out, master will remove it from list of clients or tags 
	
		
************/

bool waitingAfterAnnounce = 0;
unsigned long timeStartedAnnounceWaiting = 0;
unsigned long timeToWaitAfterAnnounce = 500; // millis

bool waitingAfterDelayReq = 0;
unsigned long timeSentDelayReq = 0; // updated 
unsigned long DelayReqWaitTimeEnd = 0;
unsigned long perClientDelayReqTimeout = 500;
uint8_t macs_requested_from[MAX_TAG_CONNECTIONS + MAX_ANCHOR_CONNECTIONS];
bool got_mac_response[MAX_TAG_CONNECTIONS + MAX_ANCHOR_CONNECTIONS];
int cur_mac_waiting_on = 0;
uint8_t masterAnchor_announceSeqNum = 0;

void masterAnchor_announce()
{
	// send announce 

	if ( tx_packet_list.size() >= PACKET_BUFFER_SIZE ) {
		Serial.println("Send master announce message, tx packet list full");
		return; // packet buffer full
	}
	if ( free_packet_list.size() == 0 ){
		Serial.println("Send announce message, free packet list empty");
		return;
	} // no free buffers
	Serial.print("Send master announce message start\r\n"); Serial.flush();

	int single_packet_index = free_packet_list.shift(); // get a free packet buffer
	//Serial.printf("send_announce_message free_packet_index %d\r\n", single_packet_index);
	packet * single_packet = &packet_buffer[single_packet_index];
	wiwi_pkt_announce * wiwi_pkt = (wiwi_pkt_announce*)single_packet->data;

	wiwi_pkt->hdr.wiwi_id = 0x6977;
	wiwi_pkt->hdr.mac_src = wiwi_mac_addr;
	wiwi_pkt->hdr.mac_dest = 0xff; // broadcast
	wiwi_pkt->hdr.pkt_type = WIWI_PKT_ANNOUNCE;
	wiwi_pkt->hdr.seq_num = masterAnchor_announceSeqNum;
	wiwi_pkt->hdr.ack_num = 0;

	wiwi_pkt->hdr.checksum = 0;
	for ( int i = 0; i < sizeof(wiwi_pkt_hdr) -2; i++ ) {
		wiwi_pkt->hdr.checksum ^= ((uint8_t*)wiwi_pkt)[i];
	}

	single_packet->pkt_len = WIWI_PKT_ANNOUNCE_LEN;
	single_packet->phase.intval = 0;
	single_packet->timestamp = 0;
	// 0 is default value, zero these out
	for ( int i = 0; i < 10; i++ ) {
		wiwi_pkt->clientAnchorList[i] = 0;
		wiwi_pkt->tagList[i] = 0;
	}
	// fill in how many clients currently subscribed 
  uint8_t mac = 0;
  for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ )
  {
    if ( !masterAnchor_getValidTagSubFromZero(i, &mac) ) {
      break;
    }
    wiwi_pkt->clientAnchorList[i] = mac;
  }
  for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
    if ( !masterAnchor_getValidAnchorSubFromZero(i, &mac) ) {
      break;
    }
    wiwi_pkt->tagList[i] = mac;
  }


	sprintf(print_buffer, "send_announce_message before buffer add %d index %d\r\n", WIWI_PKT_ANNOUNCE_LEN, single_packet_index);
	Serial.print(print_buffer); Serial.flush();
  for ( int i = 0; i < single_packet->pkt_len; i++ ) {
    sprintf(print_buffer, "0x%x ", single_packet->data[i]);
    Serial.print(print_buffer);
  }
  Serial.println("");
	// push this packet into linked list to send out 
	tx_packet_list.add(single_packet_index);
}

void masterAnchor_handleSentAnnounce(packet * single_packet) 
{
	// don't need to do much, just set timeout for clients to subscribe 
	waitingAfterAnnounce = 1;
	timeStartedAnnounceWaiting = millis();
}

void masterAnchor_handleAnchorSubscribe(packet * single_packet)
{
	// master getting a client anchor subscribe 
	// check if its already subscribed , if not that add it if possible
	wiwi_pkt_hdr * single_hdr;
	single_hdr = (wiwi_pkt_hdr *)single_packet->data;
	
	masterAnchor_startAnchorSub(single_hdr->mac_src);
	timeStartedAnnounceWaiting = millis(); // reset this
	sprintf(print_buffer,"Master anchor subscribing anchor 0x%x\r\n", single_hdr->mac_src);
	Serial.print(print_buffer);
}

void masterAnchor_handleTagSubscribe(packet * single_packet) 
{
	// master getting a tag subscribe , very similar to anchor subscribe 
	wiwi_pkt_hdr * single_hdr;
	single_hdr = (wiwi_pkt_hdr *)single_packet->data;
	
	masterAnchor_startTagSub(single_hdr->mac_src);
	timeStartedAnnounceWaiting = millis(); // reset this 
	sprintf(print_buffer, "Master anchor subscribing tag 0x%x\r\n", single_hdr->mac_src);
	Serial.print(print_buffer);
}

void masterAnchor_handleSentDelayReq(packet * single_packet) 
{
	// master receiving its own packet back 
	// parse which clients I sent to and this time 
	wiwi_pkt_delay_req * delay_req = (wiwi_pkt_delay_req *)single_packet->data;
	Serial.println("masterAnchor_handleSentDelayReq start!");
	waitingAfterDelayReq = 1;
	timeSentDelayReq = millis();
	// max wait time is worst case, timeout for every client from now 
	DelayReqWaitTimeEnd = millis() + perClientDelayReqTimeout * 
		(delay_req->num_tag_responses_requested + 
		delay_req->num_anchor_responses_requested);
			
	for ( int i = 0; i < MAX_TAG_CONNECTIONS + MAX_ANCHOR_CONNECTIONS; i++ ) {
		macs_requested_from[i] = 0;
		got_mac_response[i] = 0;
	}
	
	sprintf(print_buffer, "masterAnchor_handleSentDelayReq, num_tags=%d, num_anchors=%d\r\n",
		delay_req->num_tag_responses_requested, delay_req->num_anchor_responses_requested);
	Serial.print(print_buffer);
	
	// start at data , then move through it 
	// packet payload is dynamic based on the two counters 
	uint8_t * mac_ptr = delay_req->data;
	uint8_t mac_count = 0;
	
	for ( int i = 0; i < delay_req->num_tag_responses_requested; i++ ) {		
		macs_requested_from[i] = *(mac_ptr + i);
		sprintf(print_buffer, "Delay req from tag 0x%x\r\n", macs_requested_from[i]);
		Serial.print(print_buffer);
	}
	
	wiwi_pkt_anchor_info * ancInfo = (wiwi_pkt_anchor_info *)(delay_req->data + 
		delay_req->num_tag_responses_requested);
	t_anchorSubscriptionInfo * ancSubInfo;
	
	for ( int i = 0; i < delay_req->num_anchor_responses_requested; i++ ) {		
		macs_requested_from[i+delay_req->num_tag_responses_requested] = ancInfo->anchor_mac;
		sprintf(print_buffer, "MasterAnchor, Delay req for anchor 0x%x\r\n", 
			macs_requested_from[i+delay_req->num_tag_responses_requested] ) ;
		Serial.print(print_buffer);
		
		// store this info here as well for every anchor I broadcasted to	
		
		if ( masterAnchor_isAnchorSubbed(ancInfo->anchor_mac, &ancSubInfo) ) {
			sprintf(print_buffer, "Delay req info for 0x%x\r\n",
				ancSubInfo->anchorInfo.anchor_mac);
			Serial.print(print_buffer);
			// this is correct seq num, when I sent, store with respect to 
			// seq num in that packet 
			// for master, this will make a new wiwi entry 
			Anchor_newTxWiWi( ancSubInfo->anchorInfo.ancData, 
				ANCHOR_WIWI_DATA_HISTORY,
				delay_req->hdr.seq_num,
				single_packet->phase.intval, 1 );
			sprintf(print_buffer, "Anchor 0x%x, add tx wiwi seq=%d, tx phase=0x%x\r\n",
				ancSubInfo->anchorInfo.anchor_mac,
				delay_req->hdr.seq_num,
				single_packet->phase.intval);
			Serial.print(print_buffer);
			printAnchorData(ancSubInfo->anchorInfo.ancData, ANCHOR_WIWI_DATA_HISTORY);
			ancInfo += 1;			
		} else {
			Serial.println("!!!! Masteranchor is anchor subbed not true in sent delay req????!!!!");
		}
	}		
	
	sprintf(print_buffer, "Master anchor, delayReq phase 0x%x time %ld\r\n",
		single_packet->phase.intval,
		millis() );
	Serial.print(print_buffer);
	cur_mac_waiting_on = macs_requested_from[0];
	Serial.println("masterAnchor_handleSentDelayReq done!");
}


void masterAnchor_handleSingleTagHdrSection(t_anchorSubscriptionInfo * ancInfo,
	wiwi_pkt_tag_hdr_info * tagHdr) {
	
	t_wiwi_tag_data * tag_data = (t_wiwi_tag_data*) (tagHdr->data);
	for ( int i = 0; i < tagHdr->history_count; i++ ) {
		masterAnchor_pushAnchorTagData(ancInfo->anchorInfo.anchor_mac, 
			tagHdr->tag_mac, tag_data);
		tag_data += 1;
	}	
}

void masterAnchor_handleDelayResp(packet * single_packet) 
{
	// master getting a delay response from someone 
	// parse who its from, find its client connection and store its info
	wiwi_pkt_hdr * single_hdr;
	wiwi_pkt_delay * delay_pkt;
	single_hdr = (wiwi_pkt_hdr *)single_packet->data;
	delay_pkt = (wiwi_pkt_delay *) single_packet->data;
	Serial.println("masterAnchor_handleDelayResp start!");
	
	t_anchorSubscriptionInfo * ancInfo = 0;
	
	if ( !masterAnchor_isAnchorSubbed(single_hdr->mac_src, &ancInfo) ) {
		Serial.println("masterAnchor_handleDelayResp , anchor not subbed!");
		return;
	}
	
	masterAnchor_gotAnchorPacket(single_hdr->mac_src);
	
	// this is ba, a is master, this is received packet, b->a

	ancInfo->anchorInfo.previous_tx_ts = single_packet->timestamp;
	ancInfo->anchorInfo.ackNumber = single_hdr->ack_num;
	ancInfo->anchorInfo.last_response_seen = millis();
	ancInfo->anchorInfo.sequenceNumber = single_hdr->seq_num;
	
	// pull out the data from far side that it saw
	// 
	Anchor_newRcvdWiwi_dat( ancInfo->anchorInfo.ancData, 
		ANCHOR_WIWI_DATA_HISTORY,
		single_hdr->seq_num,
		single_packet->phase.intval,
		delay_pkt->my_prev_info.prev_phi_ab.intval,
		delay_pkt->my_prev_info.prev_phi_aa.intval, 1 );
	

	printAnchorData(ancInfo->anchorInfo.ancData, ANCHOR_WIWI_DATA_HISTORY);	
	ancInfo->anchorInfo.previous_tx_ts = delay_pkt->my_prev_info.previous_tx_ts;
	
	sprintf(print_buffer, "*******Master anchor, received phase 0x%x, rcvd ts = %d\r\n",
		single_packet->phase.intval, single_packet->timestamp);
	Serial.print(print_buffer);
	sprintf(print_buffer, "*******Master anchor, Client anchor received phase 0x%x, prev sent phase 0x%x\r\n",
		delay_pkt->my_prev_info.prev_phi_ab.intval,
		delay_pkt->my_prev_info.prev_phi_aa.intval);
	Serial.print(print_buffer);
	
	
	// this is a delay response from a valid client anchor
	// parse its tag data 
	uint8_t num_tag_data = delay_pkt->num_tags_seen;
	wiwi_pkt_tag_hdr_info * tag_hdr = (wiwi_pkt_tag_hdr_info *) delay_pkt->data[0];
	
	for ( int i = 0; i < num_tag_data; i++ ) {
		Serial.println("Master anchor, parse client anchor tag's data");
		masterAnchor_handleSingleTagHdrSection( ancInfo, tag_hdr );
		
		tag_hdr = (wiwi_pkt_tag_hdr_info *) ( ((uint8_t*) tag_hdr) + 
			2 + (tag_hdr->history_count * 9 ) ); 		
	}
	
	// now just update this specific client's info
	t_anchorSubscriptionInfo * anchor_info = 0;
	if ( masterAnchor_isAnchorSubbed(single_hdr->mac_src, &anchor_info) ) {
		anchor_info->anchorInfo.ackNumber = single_hdr->ack_num;
	}
	
	// now update higher level code 
	for ( int i = 0; i < MAX_TAG_CONNECTIONS + MAX_ANCHOR_CONNECTIONS; i++ ) {
		if ( macs_requested_from[i] == single_hdr->mac_src ) {
			got_mac_response[i] = 1;
			break;
		}
	}
	
}

void masterAnchor_handleTagResponse(packet * single_packet)
{
	wiwi_pkt_hdr * single_hdr;
	wiwi_pkt_delay_resp * delay_pkt;
	single_hdr = (wiwi_pkt_hdr *)single_packet->data;
	Serial.println("masterAnchor_handleTagResponse start!");
	masterAnchor_gotTagPacket(single_hdr->mac_src);
	masterAnchor_pushTagHistory(single_hdr->mac_src, single_hdr->seq_num,
		single_packet->phase.intval,
		single_packet->timestamp);	
		
	// now update higher level code 
	for ( int i = 0; i < MAX_TAG_CONNECTIONS + MAX_ANCHOR_CONNECTIONS; i++ ) {
		if ( macs_requested_from[i] == single_hdr->mac_src ) {
			got_mac_response[i] = 1;
			break;
		}
	}
		
}


uint8_t masterAnchor_delayReq_SeqNum = 0;
bool firstDelayReq = 1;
void masterAnchor_sendDelayReq() 
{
	// request from all tags first 	
	// then request from anchors to get also get tag info 	
	// for all anchor tags , fill in phi_aa / phi_ab / previous_tx_ts

	// send announce 

	if ( tx_packet_list.size() >= PACKET_BUFFER_SIZE ) {
		Serial.println("masterAnchor_sendDelayReq, tx packet list full");
		return; // packet buffer full
	}
	if ( free_packet_list.size() == 0 ){
		Serial.println("masterAnchor_sendDelayReq, free packet list empty");
		return;
	} // no free buffers
	
	
	Serial.print("\r\nmasterAnchor_sendDelayReq start\r\n"); Serial.flush();
	
	
	// sanity check, check if I have any tags or anchors subscribed
	uint8_t mac = 0;	
	if ( !masterAnchor_getValidTagSubFromZero(0, &mac) &&
		!masterAnchor_getValidAnchorSubFromZero(0, &mac) ) 
	{
		Serial.println("sendDelayReq , no anchors or tags, skipping\r\n");
		return;
	}

	int single_packet_index = free_packet_list.shift(); // get a free packet buffer
	packet * single_packet = &packet_buffer[single_packet_index];
	wiwi_pkt_delay_req * wiwi_pkt = (wiwi_pkt_delay_req*)single_packet->data;

	wiwi_pkt->hdr.wiwi_id = 0x6977;
	wiwi_pkt->hdr.mac_src = wiwi_mac_addr;
	wiwi_pkt->hdr.mac_dest = 0xff; // broadcast
	wiwi_pkt->hdr.pkt_type = WIWI_PKT_DELAY_REQ;
	wiwi_pkt->hdr.seq_num = masterAnchor_delayReq_SeqNum;
	wiwi_pkt->hdr.ack_num = 0;

	wiwi_pkt->hdr.checksum = 0;
	for ( int i = 0; i < sizeof(wiwi_pkt_hdr) -2; i++ ) {
		wiwi_pkt->hdr.checksum ^= ((uint8_t*)wiwi_pkt)[i];
	}

	single_packet->pkt_len = sizeof(wiwi_pkt_hdr); // dynamic, add to this 
	single_packet->phase.intval = 0;
	single_packet->timestamp = 0;	
	

	uint8_t * tag_mac_ptr = &wiwi_pkt->data[0];
	// tags
	wiwi_pkt->num_tag_responses_requested = 0; 
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		if ( !masterAnchor_getValidTagSubFromZero(i, &mac) ) {
			break;
		}
		sprintf(print_buffer,"Master anchor add tag connection to delay req for mac 0x%x\r\n", mac);
		Serial.print(print_buffer);
		// add in tag request 
		wiwi_pkt->num_tag_responses_requested++;
		*tag_mac_ptr = mac;
		tag_mac_ptr += 1;	
		single_packet->pkt_len++;	
	}
	
	// anchors 
	wiwi_pkt_anchor_info * anchor_info = (wiwi_pkt_anchor_info*) tag_mac_ptr;
	t_anchorSubscriptionInfo * ancSubInfo = 0;
	wiwi_pkt->num_anchor_responses_requested = 0;
	for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ ) {
		if ( !masterAnchor_getValidAnchorSubFromZero(i, &mac) ) {
			break;
		}
		masterAnchor_isAnchorSubbed(mac, &ancSubInfo);
		// add in anchor requests
		wiwi_pkt->num_anchor_responses_requested++;
		anchor_info->anchor_mac = mac;
		if ( firstDelayReq ) {
			anchor_info->prev_phi_aa.intval = 0;
			anchor_info->prev_phi_ab.intval = 0;
		} else {
			// when master sends delay req, the data is always for the previous seq num
			anchor_info->prev_phi_aa.intval = get_prev_tx_phase( ancSubInfo->anchorInfo.ancData, 
				ANCHOR_WIWI_DATA_HISTORY, masterAnchor_delayReq_SeqNum-1, 1) ;
			anchor_info->prev_phi_ab.intval = get_prev_rcvd_phase( ancSubInfo->anchorInfo.ancData, 
				ANCHOR_WIWI_DATA_HISTORY, masterAnchor_delayReq_SeqNum-1, 1);
		}

		anchor_info->previous_tx_ts = ancSubInfo->anchorInfo.previous_tx_ts;
		single_packet->pkt_len += sizeof(wiwi_pkt_anchor_info);
		//sprintf(print_buffer,"Master anchor add client anchor connection to delay req for mac 0x%x, new size %d\r\n", mac,
		//	single_packet->pkt_len);
		sprintf(print_buffer, "*******Master anchor, sendDelayReq, client 0x%x seq=%d -> prev_aa = 0x%x , prev_ab = 0x%x\r\n",
			mac, masterAnchor_delayReq_SeqNum,
			get_prev_tx_phase( ancSubInfo->anchorInfo.ancData, 
			ANCHOR_WIWI_DATA_HISTORY, masterAnchor_delayReq_SeqNum, 1), 
			get_prev_rcvd_phase( ancSubInfo->anchorInfo.ancData, 
			ANCHOR_WIWI_DATA_HISTORY, masterAnchor_delayReq_SeqNum, 1) );
		Serial.print(print_buffer);
		anchor_info += 1;
	}	
	*((uint8_t *) anchor_info) = 0; // append one byte of zero
	single_packet->pkt_len++;
	*(((uint8_t *) anchor_info)+1) = 0; // append one byte of zero
	single_packet->pkt_len++;
	*(((uint8_t *) anchor_info)+2) = 0; // append one byte of zero
	single_packet->pkt_len++;
	*(((uint8_t *) anchor_info)+3) = 0; // append one byte of zero
	single_packet->pkt_len++;
	masterAnchor_delayReq_SeqNum++;
	if ( firstDelayReq ) {
		firstDelayReq = 0;
	}
	

	sprintf(print_buffer, "masterAnchor_sendDelayReq before buffer add %d index %d\r\n", single_packet->pkt_len, single_packet_index);
	Serial.print(print_buffer); Serial.flush();
	for ( int i = 0; i < single_packet->pkt_len; i++ ) {
		sprintf(print_buffer, "0x%x ", single_packet->data[i]);
		Serial.print(print_buffer);
	}
	Serial.println("");
	// push this packet into linked list to send out 
	tx_packet_list.add(single_packet_index);
}


void masterAnchor_handleFullWiWiData()
{
	wiwi_anchor_info * ancInfo;
	t_Anchor_selfWiWiData * ancData; 
	for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ ) {
		int index = 0;
		ancInfo = &masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo;
		index = masterAnchor_get_newest_wiwi_complete_index(ancInfo);
		if ( index != -1 ) {
			ancData = &ancInfo->ancData[index];
			sprintf(print_buffer, "Master anchor handle full WiWi data for anchor 0x%x, index %d\r\n",
				ancInfo->anchor_mac, index);
			Serial.print(print_buffer);
			Serial.flush();
			
			sprintf(print_buffer, "Phi_aa=0x%x, phi_ab=0x%x, phi_ba=0x%x, phi_bb=0x%x\r\n",
				ancData->phi_aa.intval, ancData->phi_ab.intval, ancData->phi_ba.intval, ancData->phi_bb.intval);
			Serial.print(print_buffer);
			// compute phi_c and phi_d
			Anchor_wiwi_compute_unwrapped_phi_c_d( ancData->phi_aa.value, ancData->phi_ab.value, ancData->phi_ba.value, 
				ancData->phi_bb.value, 1, ancInfo);
			
		}
	}
}




// top level run 
unsigned long last_announce_time = 0;
unsigned long last_clean_time = 0;
unsigned long announceCount = 0;
unsigned long last_potential_delay_time = 0;


void init_masterAnchor()
{
	masterAnchor_InitTagSubs();
	masterAnchor_InitAnchorSubs();	
}


void run_masterAnchor()
{

	
	if ( 0 ) {
		// hacking out announce / dynamic client logic 
		if ( millis() - last_clean_time > 1000 ) {
			// cleanup functions 
			masterAnchor_CleanupTagSubs();
			masterAnchor_CleanupAnchorSubs();
			last_clean_time = millis();
		}
	}
	
	// master anchor top level loop 
	if ( 0 ) {
		// hacking out announce logic 
		if ( (millis() - last_announce_time > MASTER_ANNOUNCE_INTERVAL) || announceCount==0 ) {
			masterAnchor_announce();
			last_announce_time = millis();
			announceCount++;
			waitingAfterAnnounce = 1;
			return;
		}
		if ( waitingAfterAnnounce ) {
			if ( millis() - timeStartedAnnounceWaiting > MASTER_ANNOUNCE_RESPONSE_GUARDBAND ) {
				Serial.println("Done waiting after announce");
				waitingAfterAnnounce = 0;
			} else {
				return; // don't do anything, just sit in receiver mode and process incoming
			}
		}
	}

	// not in announce guardband 
	if ( waitingAfterDelayReq ) {
		// sent out delay request, check if got all responses or timeout
		// timeout is based on max timeout per end node * num end nodes
		// can make it more dynamic but just hard coding for now
		if ( millis() > DelayReqWaitTimeEnd ) {
			// waited for everybody 
			waitingAfterDelayReq = 0;
			return; 
		}		
		for ( int i = 0; i < MAX_TAG_CONNECTIONS + MAX_ANCHOR_CONNECTIONS; i++ ) {
			if ( macs_requested_from[i] != 0 ) {
				if ( !got_mac_response[i] ) { // didnt get a reply from this one, just wait
					return;
				}
			}
		}
		waitingAfterDelayReq = 0; // go immediately next loop 
	} else {
		// haven't sent out delay request yet, do that 
		// put some minimum interval 
		
		masterAnchor_sendDelayReq();
		last_potential_delay_time = millis();
		/*
		if ( millis() - last_potential_delay_time > 500 ) {			
			masterAnchor_sendDelayReq();
			last_potential_delay_time = millis();
			return;
		}
		*/
	}
	
}
