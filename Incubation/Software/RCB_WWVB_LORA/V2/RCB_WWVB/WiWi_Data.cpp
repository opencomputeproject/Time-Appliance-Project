#include "WiWi_Data.h"

/******************** Global variables ***************/
uint8_t wiwi_mac_addr = 0;


//////// Data Path 3 & 4, free buffer list
LinkedList_git<int> free_packet_list = LinkedList_git<int>();

//////// Data Path 3 structures, RX packets
LinkedList_git<int> rx_packet_list = LinkedList_git<int>();

//////// Data Path 4 structures, TX packets
LinkedList_git<int> tx_packet_list = LinkedList_git<int>();

packet packet_buffer[PACKET_BUFFER_SIZE]; // actual packet buffer







t_Anchor_selfWiWiData client_wiwidata[ANCHOR_WIWI_DATA_HISTORY];
phaseUnion client_unwrapped_phi_c;
phaseUnion client_unwrapped_phi_d;
phaseUnion client_prev_phi_c;
phaseUnion client_prev_phi_d;
uint64_t num_wiwi_calculations = 0;


/********************** Master anchor data structures ***********/
tagSubscriptionList masterAnchor_TagSubs;


void masterAnchor_InitTagSubs() {
	// just init data structures
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		memset(&masterAnchor_TagSubs.tagSubscriptionInfo[i], 0, sizeof(tagSubscriptionInfo));		
	}
}

void masterAnchor_CleanupTagSubs() {
	// run periodically to invalidate old/stale
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		if ( masterAnchor_TagSubs.tagSubscriptionInfo[i].valid ) {
			if ( (millis() - masterAnchor_TagSubs.tagSubscriptionInfo[i].last_response_seen) > 
				MASTER_TAG_TIMEOUT ) {
				masterAnchor_TagSubs.tagSubscriptionInfo[i].valid = 0;
			}
		}
	}
}

bool masterAnchor_isTagSubbed(uint8_t mac, t_tagSubscriptionInfo ** tag_info) 
{
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		if ( masterAnchor_TagSubs.tagSubscriptionInfo[i].valid &&
			masterAnchor_TagSubs.tagSubscriptionInfo[i].tag_mac == mac ) {
			*tag_info = &masterAnchor_TagSubs.tagSubscriptionInfo[i];
			return 1;
		}
	}
	return 0;
}

void masterAnchor_startTagSub(uint8_t mac)
{
	// check if its already in the data base
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		if ( masterAnchor_TagSubs.tagSubscriptionInfo[i].tag_mac == mac ) {
			masterAnchor_TagSubs.tagSubscriptionInfo[i].valid = 1;
			masterAnchor_TagSubs.tagSubscriptionInfo[i].last_response_seen = millis();
			for ( int j = 0; j < MAX_HISTORY_PER_TAG; j++ ) {
				masterAnchor_TagSubs.tagSubscriptionInfo[i].valid_history[j] = 0;
			}
			return;
		}
	}
	// not there already, find first not valid and assign it
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		if ( !masterAnchor_TagSubs.tagSubscriptionInfo[i].valid ) {
			masterAnchor_TagSubs.tagSubscriptionInfo[i].valid = 1;
			masterAnchor_TagSubs.tagSubscriptionInfo[i].tag_mac = mac;
			masterAnchor_TagSubs.tagSubscriptionInfo[i].last_response_seen = millis();
			for ( int j = 0; j < MAX_HISTORY_PER_TAG; j++ ) {
				masterAnchor_TagSubs.tagSubscriptionInfo[i].valid_history[j] = 0;
			}
		}
	}	
}

void masterAnchor_invalidateTagSub(uint8_t mac) {
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		if ( masterAnchor_TagSubs.tagSubscriptionInfo[i].tag_mac == mac ) {
			masterAnchor_TagSubs.tagSubscriptionInfo[i].valid = 0;
			masterAnchor_TagSubs.tagSubscriptionInfo[i].last_response_seen = 0;
			for ( int j = 0; j < MAX_HISTORY_PER_TAG; j++ ) {
				masterAnchor_TagSubs.tagSubscriptionInfo[i].valid_history[j] = 0;
			}
			return;
		}
	}
}

void masterAnchor_gotTagPacket(uint8_t mac)
{
	t_tagSubscriptionInfo * tag_info = 0;
	if ( masterAnchor_isTagSubbed(mac, &tag_info) ) {
		tag_info->last_response_seen = millis();
	}
}

// higher level function, to allow looping over all valid tags 
bool masterAnchor_getValidTagSubFromZero(int number, uint8_t * mac)
{
	int count = 0;
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		if ( masterAnchor_TagSubs.tagSubscriptionInfo[i].valid ) {
			if ( count == number ) {
				*mac = masterAnchor_TagSubs.tagSubscriptionInfo[i].tag_mac;
				return 1;
			} else {
				// hit a valid , but want to get the next one, keep going 
				count++;
			}
		}
	}
	// didn't find that count 
	return 0;
}

void masterAnchor_pushTagHistory(uint8_t mac, uint8_t seq_num, uint32_t phase, uint32_t ts)
{
	t_tagSubscriptionInfo * tagSub = 0;
	if ( !masterAnchor_isTagSubbed(mac, &tagSub) ) {
		return;
	}
	for ( int i = 1; i < MAX_HISTORY_PER_TAG; i++ ) {
		tagSub->valid_history[i] = tagSub->valid_history[i-1];
		tagSub->seq_num[i] = tagSub->seq_num[i-1];
		tagSub->prev_phase[i] = tagSub->prev_phase[i-1];
		tagSub->prev_rx_ts[i] = tagSub->prev_rx_ts[i-1];
	}
	tagSub->valid_history[0] = 1;
	tagSub->seq_num[0] = seq_num;
	tagSub->prev_phase[0].intval = phase;
	tagSub->prev_rx_ts[0] = ts;
}

// 0 for newest, 1 for next oldest, etc. 
bool masterAnchor_getTagHistory(uint8_t mac, uint8_t countFromNewest, uint8_t * seq_num,
	uint32_t * phase, uint32_t * ts)
{
	t_tagSubscriptionInfo * tagSub = 0;
	if ( !masterAnchor_isTagSubbed(mac, &tagSub) ) {
		return 0;
	}
	if ( countFromNewest > MAX_HISTORY_PER_TAG ) {
		return 0; // invalid number 
	}
	if ( !tagSub->valid_history[countFromNewest] ) {
		// not valid data
		return 0;
	}
	*seq_num = tagSub->seq_num[countFromNewest];
	*phase = tagSub->prev_phase[countFromNewest].intval;
	*ts = tagSub->prev_rx_ts[countFromNewest];
	return 1;	
}









anchorSubscriptionList masterAnchor_AnchorSubs;


void masterAnchor_InitAnchorSubs() {
	// just init data structure
	Serial.println("masterAnchor_InitAnchorSubs");
	
	for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ ) {
		masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.valid = 0;
		masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.last_complete_index = -1;
		masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.unwrapped_phi_c.intval = 0;
		masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.unwrapped_phi_d.intval = 0;
		masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.prev_phi_c.intval = 0;
		masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.prev_phi_d.intval = 0;
		masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.num_wiwi_calculations = 0;
		//sprintf(print_buffer, "%d anchorInfo.valid = 0\r\n", i);
		//Serial.print(print_buffer);
		initAnchorWiWidata(masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.ancData,
			ANCHOR_WIWI_DATA_HISTORY);
		//Serial.println("initClientAnchorWiWidata done");

		for ( int j = 0; j < MAX_TAG_CONNECTIONS; j++ ) {
			masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorTagInfo[j].valid = 0;
			//sprintf(print_buffer, "%d anchorTagInfo valid = 0\r\n", j);
			//Serial.print(print_buffer);
			for ( int k = 0; k < MAX_HISTORY_PER_TAG; k++ ) {
				masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorTagInfo[j].dataValid[k] = 0;
				//sprintf(print_buffer,"%d tag datavalid = 0\r\n", k);
				//Serial.print(print_buffer);
			}
		}
	}
	Serial.println("masterAnchor_InitAnchorSubs done!");
}

void masterAnchor_CleanupAnchorSubs() { 
	// run periodically to invalidate old/stale
	for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ ) {
		if ( masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.valid ) {
			if ( (millis() - masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.last_response_seen) > 
				MASTER_TAG_TIMEOUT ) {
				sprintf(print_buffer, "Master anchor, invalid sub of 0x%x\r\n", 
					masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.anchor_mac );
				Serial.print(print_buffer);
				masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.valid = 0;
				for ( int j = 0; j < MAX_TAG_CONNECTIONS; j++ ) {
					masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorTagInfo[j].valid = 0;
				}
			} 
		}
	}
}
bool masterAnchor_isAnchorSubbed(uint8_t mac, t_anchorSubscriptionInfo ** anchor_info)
{
	for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ ) {
		if ( masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.valid &&
			masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.anchor_mac == mac ) {
			*anchor_info = (t_anchorSubscriptionInfo*) &masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo;
			return 1;
		}
	}
	return 0;
}

void masterAnchor_startAnchorSub(uint8_t mac) 
{
	// check if its already in the data base
	for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ ) {
		if ( masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.anchor_mac == mac ) {
			masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.valid = 1;
			masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.last_response_seen = millis();
			return;
		}
	}
	// not there already, find first not valid and assign it
	for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ ) {
		if ( !masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.valid ) {
			masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.valid = 1;
			masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.anchor_mac = mac;
			masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.last_response_seen = millis();
			return;
		}
	}	
}

void masterAnchor_gotAnchorPacket(uint8_t mac)
{
	t_anchorSubscriptionInfo * anchor_info = 0;
	if ( masterAnchor_isAnchorSubbed(mac, &anchor_info) ) {
		anchor_info->anchorInfo.last_response_seen = millis();
	}
}


// higher level function, to allow looping over all valid tags 
bool masterAnchor_getValidAnchorSubFromZero(int number, uint8_t * mac)
{
	int count = 0;
	for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ ) {
		if ( masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.valid ) {
			//sprintf(print_buffer,"getValidAnchorFromZero, %d is valid\r\n", i); Serial.print(print_buffer);
			if ( count == number ) {
				*mac = masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.anchor_mac;
				//sprintf(print_buffer, "getValidAnchorFromZero, count=%d, num=%d, found mac 0x%x\r\n", 
				//	count, number, *mac); Serial.print(print_buffer);
				return 1;
			} else {
				// hit a valid , but want to get the next one, keep going 
				count++;
			}
		}
	}
	// didn't find that count 
	return 0;
}

bool masterAnchor_getclientAnchorTagData(uint8_t ancMac, uint8_t tagMac, 
	wiwi_tag_info ** tag_info)
{
	t_anchorSubscriptionInfo * ancInfo;
	if ( !masterAnchor_isAnchorSubbed(ancMac, &ancInfo) ) {
		return 0;
	}
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		if ( ancInfo->anchorTagInfo[i].tag_mac ) {
			*tag_info = &ancInfo->anchorTagInfo[i];
			return 1;
		}
	}	
	return 0;
}

void masterAnchor_pushclientAnchorTagHistory(wiwi_tag_info * tag_info, 
	t_wiwi_tag_data * tag_data)
{
	tag_info->last_data_seen = millis();
	// push back the history
	for ( int i = 1; i < MAX_HISTORY_PER_TAG; i++ ) {
		tag_info->dataValid[i] = tag_info->dataValid[i-1];
		tag_info->prev_phase[i].intval = tag_info->prev_phase[i-1].intval;
		tag_info->previous_rx_ts[i] = tag_info->previous_rx_ts[i-1];
		tag_info->prev_seq_num[i] = tag_info->prev_seq_num[i-1];
	}
	tag_info->dataValid[0] = 1;
	tag_info->prev_phase[0].intval = tag_data->phase.intval;
	tag_info->previous_rx_ts[0] = tag_data->rx_timestamp;
	tag_info->prev_seq_num[0] = tag_data->seq_num;
}

void masterAnchor_pushAnchorTagData(uint8_t ancMac, uint8_t tagMac,
	t_wiwi_tag_data * tag_data)
{
	wiwi_tag_info * tag_info;
	
	if ( masterAnchor_getclientAnchorTagData(ancMac, tagMac, &tag_info) ) {
		// already got data previously , easier
		masterAnchor_pushclientAnchorTagHistory(tag_info, tag_data);
		return;
	}
	// need to make a new entry
	t_anchorSubscriptionInfo * ancInfo;
	if ( !masterAnchor_isAnchorSubbed(ancMac, &ancInfo) ) {
		return;
	}
	for ( int i = 0; i < MAX_TAG_CONNECTIONS; i++ ) {
		if ( !ancInfo->anchorTagInfo[i].valid ) {
			ancInfo->anchorTagInfo[i].valid = 1;
			ancInfo->anchorTagInfo[i].tag_mac = tagMac;
			tag_info = &ancInfo->anchorTagInfo[i];
			masterAnchor_pushclientAnchorTagHistory(tag_info, tag_data);
			return;			
		}
	}
}

int masterAnchor_get_newest_wiwi_complete_index(wiwi_anchor_info * ancInfo)
{
	int latest_index = -1;
	latest_index = get_newest_wiwi_complete_index( ancInfo->ancData, ANCHOR_WIWI_DATA_HISTORY);
	if ( latest_index != -1 && ancInfo->last_complete_index != latest_index ) {
		ancInfo->last_complete_index = latest_index;
		return latest_index;
	}
	return -1;
}















static uint32_t schedule_start_time = 0;
static uint8_t schedule_mac_list[SCHEDULE_COUNT];
static bool schedule_seen_mac[SCHEDULE_COUNT];


void initSchedule()
{
	schedule_start_time = 0;
	for ( int i = 0; i < SCHEDULE_COUNT; i++ ) { 
		schedule_mac_list[i] = 0;
		schedule_seen_mac[i] = 0;
	}
}

void start_delayReq_schedule() 
{
	schedule_start_time = millis(); // just relative time is fine 
	for ( int i = 0; i < SCHEDULE_COUNT; i++ ) { // init basic schedule 
		schedule_mac_list[i] = 0;
		schedule_seen_mac[i] = 0;
	}
}

void add_mac_to_schedule(uint8_t mac, bool isTag)
{
	for ( int i = 0; i < SCHEDULE_COUNT; i++ ) {
		if ( schedule_mac_list[i] == 0 ) {
			schedule_mac_list[i] = mac;
			return;
		}
	}	
}

void schedule_see_mac_response(uint8_t mac)
{
	for ( int i = 0; i < SCHEDULE_COUNT; i++ ) {
		if ( schedule_mac_list[i] == mac ) {
			schedule_seen_mac[i] = 1;
			return;
		}
	}
}

uint32_t schedule_get_my_latest_time()
{
	for (int i = 0; i < SCHEDULE_COUNT; i++ ) {
		if ( schedule_mac_list[i] == wiwi_mac_addr ) {
			// found my location
			// from the start of schedule
			// this is the start of my time slot worst case 
			return  schedule_start_time + ( i  * MAX_ANCHORTAG_TIMESLOT );
		}		
	}
	return 0;
}

void schedule_used_my_slot() {
	schedule_start_time = 0; // use this as psuedo tag
	// not bound by schedule anymore 
}

bool schedule_is_my_turn()
{
	if ( schedule_start_time == 0 ) {
		return 0; // schedule hasn't started, not my time
	}
	for ( int i = 0; i < SCHEDULE_COUNT; i++ ) {
		// check through schedule order
		// have all the macs before mine been seen
		// OR has my worst case start time in the schedule passed
		if ( schedule_mac_list[i] != wiwi_mac_addr && !schedule_seen_mac[i] )
		{
			// seen a mac before me but it hasn't replied
			// has my scheduled time past as well
			if ( millis() > schedule_get_my_latest_time() ) {
				Serial.println("Schedule is my turn, didn't see all replies but in my time slot");
				return 1;
			}			
		} else if ( schedule_mac_list[i] == wiwi_mac_addr ) {
			// hit my item in the schedule 
			// yep go ahead
			return 1;
		}
	}
	return 0;
}




void initAnchorWiWidata(t_Anchor_selfWiWiData * arr, int count)
{
	//Serial.println("Init client anchor wiwi data");
	for ( int i = 0; i < count; i++ ) {
		//sprintf(print_buffer,"data init %d\r\n", i);
		//Serial.print(print_buffer);
		arr[i].valid = 0;
		arr[i].complete = 0;
		arr[i].seq_num_start = 0;
		arr[i].phi_aa.intval = 0;
		arr[i].phi_ab.intval = 0;
		arr[i].phi_bb.intval = 0;
		arr[i].phi_ba.intval = 0;
		arr[i].time_started = 0;
		arr[i].time_completed = 0;
	}
}

void printAnchorData(t_Anchor_selfWiWiData * arr,
	int count)
{
	for ( int i = 0; i < count; i++ ) {
		sprintf(print_buffer, "Anchor data %d, valid=%d, complete=%d, "
			"seq_num_start=%d, phi_aa=0x%x, phi_ab=0x%x, phi_bb=0x%x, phi_ba=0x%x, "
			"time_started=%lu, time_completed=%lu\r\n", i,
			arr[i].valid, arr[i].complete, arr[i].seq_num_start,
			arr[i].phi_aa.intval, arr[i].phi_ab.intval,
			arr[i].phi_bb.intval, arr[i].phi_ba.intval,
			arr[i].time_started, arr[i].time_completed);
		Serial.print(print_buffer);
	}
}


void Anchor_newTxWiWi(t_Anchor_selfWiWiData * arr, 
	int count, uint8_t seq_num, uint32_t sent_phase, bool isMaster)
{
	// this one is simpler, just find the seq_num and add this phase
	for ( int i = 0; i < count; i++ ) {
		if ( isMaster ) {
			int index = 0;
			// master TX WiWi will always be a new entry, since its the delay req 
			index = Anchor_startNewRcvdWiWiEntry(arr, count, seq_num, 1);
			arr[index].phi_aa.intval = sent_phase; 
			sprintf(print_buffer, "*******Master anchor, newTxWiWi, seq=%d, phase=0x%x\r\n", seq_num, sent_phase);
			Serial.print(print_buffer);
			return;
		} else {				
			if ( arr[i].valid && arr[i].seq_num_start == seq_num ) {
				sprintf(print_buffer, "*******Client anchor, newTxWiWi, seq=%d, phase=0x%x\r\n", seq_num, sent_phase);
				Serial.print(print_buffer);
				arr[i].phi_bb.intval = sent_phase;
				return;
			}
		}
	}
}


int Anchor_startNewRcvdWiWiEntry(t_Anchor_selfWiWiData * arr, int count,uint8_t seq_num,
	bool isMaster)
{
	// find one to start new , either first non valid, or oldest
	for ( int i = 0; i < count; i++ ) {
		if ( !arr[i].valid ) {
			arr[i].valid = 1;
			arr[i].complete = 0;
			arr[i].seq_num_start = seq_num;
			arr[i].phi_ab.intval = 0;
			arr[i].phi_aa.intval = 0;
			arr[i].phi_bb.intval = 0;
			arr[i].phi_ba.intval = 0;
			arr[i].time_started = millis();
			return i;
		}
	}
	// couldn't find a free one, look for oldest 
	unsigned long oldest_time = 0xffffffff;
	int oldest_index = 0;
	for ( int i = 0; i < count; i++ ) {
		if ( arr[i].time_started < oldest_time ) {
			oldest_time = arr[i].time_started;
			oldest_index = i;
		}		
	}
	arr[oldest_index].valid = 1;
	arr[oldest_index].complete = 0;
	arr[oldest_index].seq_num_start = seq_num;
	arr[oldest_index].phi_ab.intval = 0;
	arr[oldest_index].phi_aa.intval = 0;
	arr[oldest_index].phi_bb.intval = 0;
	arr[oldest_index].phi_ba.intval = 0;
	arr[oldest_index].time_started = millis();
	arr[oldest_index].time_completed = 0;
	return oldest_index;	
}



int Anchor_newRcvdWiwi_dat(t_Anchor_selfWiWiData * arr, int count, 
	uint8_t seq_num, uint32_t rcvd_phase, uint32_t told_rx_phase,
	uint32_t told_tx_phase, bool isMaster)
{
	int toReturn = -1;
	
	if ( isMaster ) {
		sprintf(print_buffer, "*******Master Anchor_newRcvdWiwi , seq_num=%d, rcvd_phase=0x%x"
			", told_rx_phase=0x%x, told_tx_phase=0x%x\r\n", seq_num, rcvd_phase,
			told_rx_phase, told_tx_phase);
	} else {
		sprintf(print_buffer, "*******Client Anchor_newRcvdWiwi , seq_num=%d, rcvd_phase=0x%x"
			", told_rx_phase=0x%x, told_tx_phase=0x%x\r\n", seq_num, rcvd_phase,
			told_rx_phase, told_tx_phase);
	}

	Serial.print(print_buffer);
	
	// when master receives delayResp from client 
	// prev_tx_phase is for phi_bb(seq_num - 1) , prev_rx_phase is for phi_ab(seq_num) , rcvd_phase is for phi_ba(seq_num)
	// when client receives delayReq from master
	// prev_tx_phase is for phi_aa(seq_num -1) , prev_rx_phase is for phi_ba(seq_num - 1) , rcvd_phase is for phi_ab(seq_num)
	
	// look for valid seq_num - 1
	uint8_t seq_num_look_for = 0;
	if ( seq_num == 0 ) {
		seq_num_look_for = 255;
	} else {
		seq_num_look_for = seq_num-1;
	}
	bool found_seq_num = 0;

	// look for seq_num first , should always be an entry
	for ( int i = 0; i < count; i++ ) {
		if ( arr[i].valid && arr[i].seq_num_start == seq_num ) {
			if ( isMaster ) {
				arr[i].phi_ba.intval = rcvd_phase;
				arr[i].phi_ab.intval = told_rx_phase;
			} else {
				arr[i].phi_ab.intval = rcvd_phase;				
			}
			found_seq_num = 1;
			break;
		}
	}
		
	// now look for seq_num -1
	for ( int i = 0; i < count; i++ ) {
		if ( arr[i].valid && arr[i].seq_num_start == seq_num_look_for ) {
			if ( isMaster ) {
				arr[i].phi_bb.intval = told_tx_phase;				
			} else {
				arr[i].phi_aa.intval = told_tx_phase;
				arr[i].phi_ba.intval = told_rx_phase; 
			}
			// if found seq_num as well , then I think this means have complete data in prev seq num
			if ( found_seq_num ) {
				arr[i].time_completed = millis();
				arr[i].complete = 1;
				sprintf(print_buffer, "*******COMPLETE  wiwi data: phi_aa=0x%x, phi_bb=0x%x, phi_ba=0x%x, phi_ab=0x%x*******\r\n",
					arr[i].phi_aa.intval,
					arr[i].phi_bb.intval,
					arr[i].phi_ba.intval,
					arr[i].phi_ab.intval);
				Serial.print(print_buffer);	
				return i;
			}
			break;
		}
	}
	
	return -1;	
}



uint32_t get_prev_rcvd_phase(t_Anchor_selfWiWiData * arr,
	int count,
	uint8_t seq_num, bool isMaster)
{
	// look for this seq num entry , and return phi_xy
	for ( int i = 0; i < count; i++ ) {
		if ( arr[i].valid && arr[i].seq_num_start == seq_num ) {
			if ( isMaster ) {
				return arr[i].phi_ba.intval;
			} else {
				return arr[i].phi_ab.intval;
			}
			
		}
	}	
	return 0;
}

uint32_t get_prev_tx_phase(t_Anchor_selfWiWiData * arr,
	int count,
	uint8_t seq_num, bool isMaster)
{
	// look for this seq num entry, and return phi_xx
	for ( int i = 0; i < count; i++ ) {
		if ( arr[i].valid && arr[i].seq_num_start == seq_num ) {
			if ( isMaster ) {
				return arr[i].phi_aa.intval;
			} else {
				return arr[i].phi_bb.intval;
			}			
		}
	}	
	return 0;
}
int get_newest_wiwi_complete_index(t_Anchor_selfWiWiData * arr, int count)
{
	int newest_complete_index = -1;
	unsigned long highest_time_complete = 0;
	
	for ( int i = 0; i < count; i++ ) {
		if ( arr[i].valid && arr[i].complete && arr[i].time_completed > highest_time_complete ) {
			newest_complete_index = i;
			highest_time_complete = arr[i].time_completed;
		}
	}
	return newest_complete_index;
}










// Function to unwrap phase
float unwrap_phase(float current_phase, float *previous_phase, float *accumulated_phase) {
	//Serial.println("Unwrap phase start!"); Serial.flush();
    float phase_diff = current_phase - *previous_phase;

    // Handle wrapping: if the phase difference exceeds PI, correct it
    if (phase_diff > M_PI) {
        phase_diff -= 2 * M_PI;
    } else if (phase_diff < -M_PI) {
        phase_diff += 2 * M_PI;
    }

    // Accumulate the unwrapped phase
    *accumulated_phase += phase_diff;
    
    // Update the previous phase
    *previous_phase = current_phase;
	
	//Serial.println("Unwrap phase end!"); Serial.flush();
    
    return *accumulated_phase;
}


// Manual floor-like operation for positive and negative values
float custom_floor(float value) {
    int int_part = (int)value;
    return (value >= 0 || value == (float)int_part) ? (float)int_part : (float)(int_part - 1);
}

// Manual modulo operation for positive and negative wrapping
float custom_mod(float value, float mod_base) {
    float quotient = value / mod_base;
	
    // Apply manual floor function
    quotient = custom_floor(quotient);
	
    // Calculate the result of the modulo operation
    float result = value - (quotient * mod_base);
	
    // Adjust result if it's negative
    if (result < 0) {
        result += mod_base;
    }
    return result;
}


void Anchor_wiwi_compute_unwrapped_phi_c_d(float phi_aa, float phi_ab, 
	float phi_ba, float phi_bb, bool isMaster,
	wiwi_anchor_info * ancInfo)
{
	float phi_A=0, phi_B=0, twophi_C=0, twophi_D=0;
	phi_A = phi_ab - phi_aa;
	phi_B = phi_ba - phi_bb;
	
	sprintf(print_buffer,"Anchor_wiwi_compute_unwrapped_phi_c_d start, aa=%f, ab=%f, "
		"ba=%f, bb=%f, phi_A=%f, phi_B=%f\r\n",
		phi_aa, phi_ab, phi_ba, phi_bb,
		phi_A, phi_B);
	Serial.print(print_buffer);
	Serial.flush();
	
	
	// phi_c, clock phase measurement
	// a = sum
	// n = 2*M_PI
	// floor in C is non ideal , handle the sign myself
	// fmod is not good, my own version of fmod as well
	twophi_C = custom_mod(phi_A - phi_B, 2*M_PI);
	twophi_D = custom_mod(phi_A + phi_B, 2*M_PI);
	/*
	if ( ( phi_A - phi_B) < 0 ) {
		twophi_C = (phi_A - phi_B) - (-2*M_PI)*ceil(fabs((phi_A - phi_B)/(2*M_PI)));
	} else {
		twophi_C = (phi_A - phi_B) - (-2*M_PI)*floor((phi_A - phi_B)/(2*M_PI));
	}
	if ( (phi_A + phi_B) < 0 ) {
		twophi_D = (phi_A + phi_B) - (-2*M_PI)*ceil(fabs((phi_A + phi_B)/(2*M_PI)));
	} else {
		twophi_D = (phi_A + phi_B) - (2*M_PI)*floor((phi_A + phi_B)/(2*M_PI));
	}
	*/
	
	sprintf(print_buffer, "**********wiwi_compute_phi_c_d 2phi_C=%f 2phi_D=%f\r\n",
		twophi_C, twophi_D);
	Serial.print(print_buffer);
	Serial.flush();
	
	if ( isMaster ) {
		// handle unwrapping using variables in ancInfo
		float prev_phi_c , prev_phi_d, unwrapped_phi_c, unwrapped_phi_d;
		// doesnt seem to like passing phase unions around , just use floats 
		
		prev_phi_c = ancInfo->prev_phi_c.value;
		prev_phi_d = ancInfo->prev_phi_d.value;
		unwrapped_phi_c = ancInfo->unwrapped_phi_c.value;
		unwrapped_phi_d = ancInfo->unwrapped_phi_d.value;
		unwrap_phase(twophi_C, &prev_phi_c, &unwrapped_phi_c );
		unwrap_phase(twophi_D, &prev_phi_d, &unwrapped_phi_d );
		ancInfo->num_wiwi_calculations++;
		

		sprintf(print_buffer, "**********MasterAnchor wiwi_compute_phi_c_d unwrapped 2phi_C=%f 2phi_D=%f\r\n",
		unwrapped_phi_c, unwrapped_phi_d);
		Serial.print(print_buffer);
		
		
		ancInfo->prev_phi_c.value = twophi_C;
		ancInfo->prev_phi_d.value = twophi_D;
		ancInfo->unwrapped_phi_c.value = unwrapped_phi_c;
		ancInfo->unwrapped_phi_d.value = unwrapped_phi_d;
	} else {
		unwrap_phase(twophi_C, &client_prev_phi_c.value, &client_unwrapped_phi_c.value);
		unwrap_phase(twophi_D, &client_prev_phi_d.value, &client_unwrapped_phi_d.value);
		num_wiwi_calculations++;
		sprintf(print_buffer, "**********ClientAnchor wiwi_compute_phi_c_d unwrapped 2phi_C=%f 2phi_D=%f\r\n",
		client_unwrapped_phi_c.value, client_unwrapped_phi_d.value);
		Serial.print(print_buffer);
		
		wiwi_oscillator_feedback(client_unwrapped_phi_c.value);
	}
	
	Serial.println("Anchor_wiwi_compute_unwrapped_phi_c_d end!");
	Serial.flush();
}
