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
		initClientAnchorWiWidata(masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.ancData,
			CLIENT_ANCHOR_WIWI_DATA_HISTORY);

		for ( int j = 0; j < MAX_TAG_CONNECTIONS; j++ ) {
			masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorTagInfo[j].valid = 0;
			for ( int k = 0; k < MAX_HISTORY_PER_TAG; k++ ) {
				masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorTagInfo[j].dataValid[k] = 0;
			}
		}
	}
}

void masterAnchor_CleanupAnchorSubs() { 
	// run periodically to invalidate old/stale
	for ( int i = 0; i < MAX_ANCHOR_CONNECTIONS; i++ ) {
		if ( masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.valid ) {
			if ( (millis() - masterAnchor_AnchorSubs.anchorSubscriptionInfo[i].anchorInfo.last_response_seen) > 
				MASTER_TAG_TIMEOUT ) {
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













static bool rcvd_wiwi_data = 0;
void initClientAnchorWiWidata(t_clientAnchor_selfWiWiData * arr, int count)
{
	for ( int i = 0; i < count; i++ ) {
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


void clientAnchor_newTxWiWi(t_clientAnchor_selfWiWiData * arr, 
	int count, uint8_t seq_num, uint32_t sent_phase)
{
	// this one is simpler, just find the seq_num and add this phase
	for ( int i = 0; i < count; i++ ) {
		if ( arr[i].valid && arr[i].seq_num_start == seq_num ) {
			sprintf(print_buffer, "*******Client anchor, newTxWiWi, seq=%d, phase=0x%x\r\n", seq_num, sent_phase);
			Serial.print(print_buffer);
			arr[i].phi_bb.intval = sent_phase;
			return;
		}
	}
}

int clientAnchor_startNewRcvdWiWiEntry(t_clientAnchor_selfWiWiData * arr, int count,uint8_t seq_num)
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


int clientAnchor_newRcvdWiwi_dat(t_clientAnchor_selfWiWiData * arr, int count, 
	uint8_t seq_num, uint32_t rcvd_phase, uint32_t told_rx_phase,
	uint32_t told_tx_phase)
{
	int toReturn = -1;
	
	sprintf(print_buffer, "*******clientAnchor_newRcvdWiwi , seq_num=%d, rcvd_phase=0x%x"
		", told_rx_phase=0x%x, told_tx_phase=0x%x\r\n", seq_num, rcvd_phase,
		told_rx_phase, told_tx_phase);
	
	// this function will be called twice to fill out a wiwi structure
	// case 1: initial case
	// just put this sequence number into first entry 
	if ( !rcvd_wiwi_data ) {
		rcvd_wiwi_data = 1;
		arr[0].complete = 0;
		arr[0].valid = 1;
		arr[0].seq_num_start = seq_num;
		arr[0].phi_ab.intval = rcvd_phase;
		arr[0].time_started = millis();
		arr[0].time_completed = 0;
		return 0;
	}
	
	
	// case 2: running case
	// find first valid entry and add it, OR find oldest and start to override 
	// find seq_num - 1 in the list (if 0 , check for 255)
	// 		if it's not there, then that means lost data and need to start over in a new entry
	//			find first non valid entry, or the oldest one
	//		if it is there, add this as phi_ba and mark complete, and also start next entry 
	uint8_t seq_num_look_for = 0;
	if ( seq_num == 0 ) {
		seq_num_look_for = 255;
	} else {
		seq_num_look_for = seq_num-1;
	}
	for ( int i = 0; i < count; i++ ) {
		if ( arr[i].valid && arr[i].seq_num_start == seq_num_look_for ) {
			// found it , good, store this
			arr[i].phi_ba.intval = told_rx_phase; 
			arr[i].phi_aa.intval = told_tx_phase; 
			sprintf(print_buffer, "****** WIWI FULL DATA: phi_aa=0x%x, phi_bb=0x%x, phi_ba=0x%x, phi_ab=0x%x\r\n",
				arr[i].phi_aa.intval,
				arr[i].phi_bb.intval,
				arr[i].phi_ba.intval,
				arr[i].phi_ab.intval);
			Serial.print(print_buffer);		
			arr[i].time_completed = millis();
			arr[i].complete = 1;
			toReturn = i;
			break;
		}
	}
	
	// start next entry 
	int new_entry_index = 0;
	new_entry_index = clientAnchor_startNewRcvdWiWiEntry(arr, count, seq_num);
	arr[new_entry_index].valid = 1; 
	arr[new_entry_index].complete = 0;
	arr[new_entry_index].seq_num_start = seq_num;
	arr[new_entry_index].phi_aa.intval = 0;
	arr[new_entry_index].phi_bb.intval = 0;
	arr[new_entry_index].phi_ba.intval = 0;
	arr[new_entry_index].phi_ab.intval = rcvd_phase;
	arr[new_entry_index].time_started = millis();
	arr[new_entry_index].time_completed = 0;
	return toReturn;
}


uint32_t get_prev_rcvd_phase(t_clientAnchor_selfWiWiData * arr,
	int count,
	uint8_t seq_num)
{
	// look for this seq num entry , and return phi_ab
	for ( int i = 0; i < count; i++ ) {
		if ( arr[i].valid && arr[i].seq_num_start == seq_num ) {
			return arr[i].phi_ab.intval;
		}
	}	
	return 0;
}
uint32_t get_prev_tx_phase(t_clientAnchor_selfWiWiData * arr,
	int count,
	uint8_t seq_num)
{
	// look for this seq num entry, and return phi_bb
	for ( int i = 0; i < count; i++ ) {
		if ( arr[i].valid && arr[i].seq_num_start == seq_num ) {
			return arr[i].phi_bb.intval;
		}
	}	
	return 0;
}
int get_newest_wiwi_complete_index(t_clientAnchor_selfWiWiData * arr, int count)
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

