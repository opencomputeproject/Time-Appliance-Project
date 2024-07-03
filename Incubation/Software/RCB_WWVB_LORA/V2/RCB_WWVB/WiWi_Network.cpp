

#include "WiWi_Network.h"


/******** Local WiWi network variables **********/
uint8_t wiwi_network_mode = WIWI_MODE_CLIENT;
uint8_t wiwi_state = WIWI_START;


/************************* Packet Reception Logic ************/
unsigned long client_timeout_interval = 5000000; // hacking timeouts to be incredibly long for debug purposes
unsigned long master_timeout_interval = 1000000; //millis value

#define HACK_IMMEDIATE_REPLY


unsigned long master_request_interval = 500; // millis counter 

unsigned long last_master_request_time = 0; // millis value


unsigned long announce_interval = 2000; // millis counter, adjustable threshold
unsigned long last_announce_time = 0; // millis value








float twophi_C_history=0;
float twophi_C_history_raw = 0;
int uwcount_c_prev = 0;

float twophi_D_history=0;
float twophi_D_history_raw = 0;
int uwcount_d_prev = 0;
void wiwi_use_phi_c(phaseUnion phi_aa, phaseUnion phi_ab, phaseUnion phi_ba, phaseUnion phi_bb) {
  // not included in this release
  return;  
}



// a single delay req / delay resp, I need a second transaction
int valid_delay_req_resp_sequences = 0;
// consider myself as client as a, master is b
phaseUnion phi_aa= {.intval=0};
phaseUnion phi_ab={.intval=0}; 
phaseUnion phi_ba={.intval=0};
phaseUnion phi_bb={.intval=0};
void wiwi_sent_delayreq_got_delayresp(packet * delay_req, packet * delay_resp) {
  wiwi_pkt_delay * delay_req_pkt = (wiwi_pkt_delay *) delay_req->data;
  wiwi_pkt_delay * delay_resp_pkt = (wiwi_pkt_delay *) delay_resp->data;

  
  sprintf(print_buffer, "Valid delayreq/delayresp data: phase_sent=0x%lx phase_received=0x%lx phase_farreceive=0x%lx phase_farsent=0x%lx\r\n",
      delay_req->phase.intval, delay_resp->phase.intval, delay_resp_pkt->previous_rx_iq.intval, delay_resp_pkt->previous_tx_iq.intval);
  Serial.print(print_buffer);


  if ( valid_delay_req_resp_sequences == 0 ) { // first sequence don't have full data, but need to store data
    // store
    phi_aa = delay_req->phase; // store my latest phase sent

    phi_ba = delay_resp->phase; //store phase of received phase from response

    phi_ab = delay_resp_pkt->previous_rx_iq; // store phase far side saw from my request

    uwcount_c_prev = 0;
    uwcount_d_prev = 0;
    twophi_C_history = 0;
    twophi_D_history = 0;

  } else {
    // only need phi_bb from this request, rest of data came from previous exchange
    phi_bb = delay_resp_pkt->previous_tx_iq; 

    // now have full data about previous delay req / resp sequence 
    sprintf(print_buffer, "Compute wiwi, phi_aa=0x%lx %f phi_ab=0x%lx %f phi_ba=0x%lx %f phi_bb=0x%lx %f\r\n",
      phi_aa.intval, phi_aa.value, phi_ab.intval, phi_ab.value, phi_ba.intval, phi_ba.value, phi_bb.intval, phi_bb.value);
    Serial.print(print_buffer);

    // compute phases
    wiwi_use_phi_c(phi_aa, phi_ab, phi_ba, phi_bb);

    // store for next
    phi_aa = delay_req->phase; // store my latest phase sent
    phi_ba = delay_resp->phase;
    phi_ab = delay_resp_pkt->previous_rx_iq;
  }



  valid_delay_req_resp_sequences++;
}








void wiwi_client_send_master_request(bool retransmit) {  
  sprintf(print_buffer, "Wiwi client send master request start retransmit %d seq=%d ack=%d\r\n", retransmit,
    clientmanager.master.sequenceNumber, clientmanager.master.ackNumber);
  Serial.print(print_buffer);
	if ( tx_packet_list.size() >= PACKET_BUFFER_SIZE ) {
    Serial.print("TX packet list full!\r\n");
		return; // packet buffer full
	}
  if ( free_packet_list.size() == 0 ) {
    Serial.print("Free packet buffer list empty!\r\n");
    return; // no free packet buffer
  }
  if ( retransmit ) {
    valid_delay_req_resp_sequences = 0;
    uwcount_c_prev = 0;
    uwcount_d_prev = 0;
    twophi_C_history = 0;
    twophi_D_history = 0;
  }
  int single_packet_index = free_packet_list.shift(); // get a free packet buffer
  sprintf(print_buffer, "client send master request free packet index %d\r\n", single_packet_index);
  Serial.print(print_buffer);
  packet * single_packet = &packet_buffer[single_packet_index];
	wiwi_pkt_delay * wiwi_pkt = 
		(wiwi_pkt_delay*)single_packet->data;

  //
  //packet * single_packet = &clientmanager.master.last_tx_pkt[0];
	// keep track of this packet for retransmission

	
	wiwi_pkt->hdr.wiwi_id = htonl(0x77697769);
	wiwi_pkt->hdr.mac_src = wiwi_mac_addr;
	wiwi_pkt->hdr.mac_dest = clientmanager.master.clientMac;
	wiwi_pkt->hdr.pkt_type = WIWI_PKT_DELAY_REQ;
  if ( retransmit ) {
    wiwi_pkt->hdr.seq_num = clientmanager.master.sequenceNumber;
    clientmanager.master.retransmit_count++;
  } else {
    wiwi_pkt->hdr.seq_num = clientmanager.master.sequenceNumber; // INCREMENT THIS WHEN GET ACK
    clientmanager.master.retransmit_count = 0;
  }
	
	wiwi_pkt->hdr.ack_num = clientmanager.master.ackNumber;
	wiwi_pkt->flags = 3; // NEED TO FIX THIS, SHOULD NOT BE CONST 
	wiwi_pkt->previous_tx_ts = clientmanager.master.last_tx_pkt[0].timestamp;
	wiwi_pkt->previous_tx_iq = clientmanager.master.last_tx_pkt[0].phase;
	wiwi_pkt->previous_rx_ts = clientmanager.master.last_rx_delay_pkts[0].timestamp;
	wiwi_pkt->previous_rx_iq = clientmanager.master.last_rx_delay_pkts[0].phase; 

  wiwi_pkt->checksum = 0;
  for ( int i = 0; i < sizeof(wiwi_pkt_hdr) -1; i++ ) {
    wiwi_pkt->checksum ^= ((uint8_t*)wiwi_pkt)[i];
  }

  single_packet->pkt_len = WIWI_PKT_DELAY_LEN;
  single_packet->phase.intval = 0;
  single_packet->timestamp = 0;
  sprintf(print_buffer, "wiwi_client_send_master_request Before add tx packet index %d list %d\r\n",single_packet_index,WIWI_PKT_DELAY_LEN);
  Serial.print(print_buffer);

  	// push this packet into linked list to send out 
  tx_packet_list.add(single_packet_index);

  // IQ and timestamp will be filled in by receive path
  memcpy(&clientmanager.master.last_tx_pkt[1],&clientmanager.master.last_tx_pkt[0],sizeof(packet));
  memcpy(&clientmanager.master.last_tx_pkt[0], single_packet, sizeof(packet));
	

}


void wiwi_receive_valid_master_announce(packet * pkt, wiwi_pkt_announce * announce_pkt) 
{
  // ok have two packets now, last_rx_pkt and this packet
  // can do frequency comparison

  if ( clientmanager.master.announce_count < 3 ) {
    clientmanager.master.announce_count++; 
    sprintf(print_buffer, "Receive valid master announce count %d\r\n", clientmanager.master.announce_count);
    Serial.print(print_buffer);
  } else {
    // need to see at least 3 announce messages to do frequency stuff
    // seen three here, 
    Serial.print("Received at least 3 announces, try to do freq\r\n"); 
    wiwi_pkt_announce * first_annc = (wiwi_pkt_announce *) clientmanager.master.last_rx_announce_pkts[1].data;
    wiwi_pkt_announce * second_annc = (wiwi_pkt_announce *) clientmanager.master.last_rx_announce_pkts[0].data;
    wiwi_pkt_announce * third_annc = announce_pkt;

    // do a sequence check in case I missed packets, they all need to be sequential
    if ( second_annc->hdr.seq_num != (first_annc->hdr.seq_num+1)) {
      sprintf(print_buffer, "Freq correction not possible, first seq %d second seq %d\n",
        first_annc->hdr.seq_num, second_annc->hdr.seq_num);
      Serial.print(print_buffer);
      return;
    }
    if ( third_annc->hdr.seq_num != (second_annc->hdr.seq_num+1)) {
      sprintf(print_buffer, "Freq correction not possible, second seq %d third seq %d\n",
        second_annc->hdr.seq_num, third_annc->hdr.seq_num);
      Serial.print(print_buffer);
      return;
    }

    // setup the timestamps locally versus remote
    uint32_t local_timestamp_first = clientmanager.master.last_rx_announce_pkts[1].timestamp;
    uint32_t local_timestamp_second = clientmanager.master.last_rx_announce_pkts[0].timestamp; 

    uint32_t remote_timestamp_first = second_annc->previous_tx_time;
    uint32_t remote_timestamp_second = third_annc->previous_tx_time;

    uint32_t local_diff = local_timestamp_second - local_timestamp_first;
    uint32_t remote_diff = remote_timestamp_second - remote_timestamp_first;

    /* Timestamps not good for my purposes at the moment
    Keep this code, but not used at the moment
    if ( remote_diff > local_diff ) {
      Serial.printf("Client freq follow, LOCAL SLOWER local difference 0x%lx, remote difference 0x%lx\n",
        local_diff, remote_diff);
    } else if ( remote_diff < local_diff ) {
      Serial.printf("Client freq follow, LOCAL FASTER local difference 0x%lx, remote difference 0x%lx\n",
        local_diff, remote_diff);
    } else {
      Serial.printf("Client freq follow exact same frequency somehow????? local diff 0x%lx remote 0x%lx\n",
        local_diff, remote_diff);
    }
    */
  }

  memcpy(&clientmanager.master.last_rx_announce_pkts[1], &clientmanager.master.last_rx_announce_pkts[0],
    sizeof(packet)); // push it back
  memcpy(&clientmanager.master.last_rx_announce_pkts[0], pkt, sizeof(packet)); // store this packet as new last rx packet
}

void wiwi_receive_potential_master_announce(packet * pkt, wiwi_pkt_announce * announce_pkt)
{
  // stupid heuristic, is their mac address lower than mine, if so follow them
  if ( wiwi_mac_addr > announce_pkt->hdr.mac_src )
  {
    clientmanager.master.isValid = true;
    clientmanager.master.clientMac = announce_pkt->hdr.mac_src;
    clientmanager.master.last_receive_time = millis(); // just local timestamp is fine
    clientmanager.master.last_transmit_time = 0; // havent sent anything yet
    clientmanager.master.retransmit_count = 0;
    clientmanager.master.ackNumber = 0;
    clientmanager.master.sequenceNumber = 0;
    clientmanager.master.announce_count = 1; // seen 1

    wiwi_state = WIWI_RUNNING; // found my master, off and running

    memcpy(&clientmanager.master.last_rx_announce_pkts[0], pkt, sizeof(pkt));
    sprintf(print_buffer, "Following new master! MAC 0x%x\r\n", clientmanager.master.clientMac);
    Serial.print(print_buffer);
  }
}


void wiwi_send_delay_resp(ClientConnection * client, packet * pkt_delay_req, wiwi_pkt_delay * delay_req) {
  sprintf(print_buffer, "Wiwi master send delay response request start, rcvd pkt src=0x%x dest=0x%x seq=%d ack=%d\r\n",
    delay_req->hdr.mac_src, delay_req->hdr.mac_dest, delay_req->hdr.seq_num, delay_req->hdr.ack_num);
  Serial.print(print_buffer);
	if ( tx_packet_list.size() >= PACKET_BUFFER_SIZE ) {
    Serial.print("wiwi send delay resp tx packet list full!\r\n");
		return; // packet buffer full
	}
  if ( free_packet_list.size() == 0 ) {
    Serial.print("wiwi send delay resp free packet buffer list empty!\r\n");
    return; // no free packet buffer
  }

  int single_packet_index = free_packet_list.shift(); // get a free packet buffer
  sprintf(print_buffer, "wiwi_send_delay_resp Free packet list get index %d\r\n", single_packet_index);
  Serial.print(print_buffer);
  packet * single_packet = &packet_buffer[single_packet_index];
	wiwi_pkt_delay * wiwi_pkt = 
		(wiwi_pkt_delay*)single_packet->data;

	
	wiwi_pkt->hdr.wiwi_id = htonl(0x77697769);
	wiwi_pkt->hdr.mac_src = wiwi_mac_addr;
	wiwi_pkt->hdr.mac_dest = delay_req->hdr.mac_src;
	wiwi_pkt->hdr.pkt_type = WIWI_PKT_DELAY_RESP;
	wiwi_pkt->hdr.seq_num = client->sequenceNumber++;
	wiwi_pkt->hdr.ack_num = client->ackNumber;
  sprintf(print_buffer, "WiWi master send delay response packet src=0x%x dest=0x%x seq_num=%d, ack_num=%d prev_iq=0x%lx\r\n",
    wiwi_pkt->hdr.mac_src, wiwi_pkt->hdr.mac_dest, wiwi_pkt->hdr.seq_num, wiwi_pkt->hdr.ack_num, client->last_tx_pkt[0].phase.intval);
  Serial.print(print_buffer);
	wiwi_pkt->flags = 3; // NEED TO FIX THIS, SHOULD NOT BE CONST 
	wiwi_pkt->previous_tx_ts = client->last_tx_pkt[0].timestamp;
	wiwi_pkt->previous_tx_iq = client->last_tx_pkt[0].phase;

  // send back what just got
	wiwi_pkt->previous_rx_ts = pkt_delay_req->timestamp;
	wiwi_pkt->previous_rx_iq = pkt_delay_req->phase;

  single_packet->pkt_len = WIWI_PKT_DELAY_LEN;
  single_packet->phase.intval = 0;
  single_packet->timestamp = 0;

  wiwi_pkt->checksum = 0;
  for ( int i = 0; i < sizeof(wiwi_pkt_hdr) -1; i++ ) {
    wiwi_pkt->checksum ^= ((uint8_t*)wiwi_pkt)[i];
  }
	
	// push this packet into linked list to send out 
  sprintf(print_buffer, "wiwi_send_delay_resp before tx packet list add %d index %d\r\n", WIWI_PKT_DELAY_LEN, single_packet_index);
  Serial.print(print_buffer);
  tx_packet_list.add(single_packet_index);
  memcpy(&client->last_tx_pkt[1],&client->last_tx_pkt[0],sizeof(packet));
  memcpy(&client->last_tx_pkt[0], single_packet, sizeof(packet));


  // hack logic, if I'm sending delay responses, I dont need announce anymore
  announce_interval = 500000;
  
}

void wiwi_receive_delay_req(packet * pkt) {
  // try to find the client in client manager
  wiwi_pkt_delay * rcvd_delay_pkt = (wiwi_pkt_delay *) pkt->data;

  ClientConnection* client = 0;
  client = ConnectionManager_GetClient(&clientmanager, rcvd_delay_pkt->hdr.mac_src);
  if ( client == NULL  ) {
    // client not currently known, add it if manager not full
    sprintf(print_buffer, "Received Delay Request, adding client 0x%x\r\n", rcvd_delay_pkt->hdr.mac_src);
    Serial.print(print_buffer);
    if ( !ConnectionManager_AddClient(&clientmanager, rcvd_delay_pkt->hdr.mac_src) ) {
      Serial.print("Failed to add client, end handle delay req\r\n");
      return;
    }
    // successfully added, get pointer to new client connection
    client = ConnectionManager_GetClient(&clientmanager, rcvd_delay_pkt->hdr.mac_src);
  }
  client->ackNumber = rcvd_delay_pkt->hdr.seq_num; // acknowledge which sequence I've seen
  client->last_receive_time = millis(); 


  // send response
  wiwi_send_delay_resp(client, pkt, rcvd_delay_pkt );


  // push back this response
  memcpy(&client->last_rx_delay_pkts[1], &client->last_rx_delay_pkts[0], sizeof(packet) );
  memcpy(&client->last_rx_delay_pkts[0], pkt, sizeof(packet) );

}

void wiwi_receive_potential_delay_resp(packet * pkt) {
  // for now, assume potential resp is only with master
  // look at both this received packet and my previous tx packet 
  wiwi_pkt_delay * rcvd_delay_pkt = (wiwi_pkt_delay *) pkt->data;
  wiwi_pkt_delay * sent_delay_pkt = (wiwi_pkt_delay *) clientmanager.master.last_tx_pkt[0].data;

  if ( rcvd_delay_pkt->hdr.ack_num != sent_delay_pkt->hdr.seq_num ) 
  {
    // out of order, ignore
    sprintf(print_buffer, "Delay resp ack num != seq num, ignore! %d, %d\r\n",
      rcvd_delay_pkt->hdr.ack_num, sent_delay_pkt->hdr.seq_num);
    Serial.print(print_buffer);
    return;
  }
  if ( !clientmanager.master.isValid ) {
    // master replied too late , I'm done with them
    Serial.print("Got delay resp but master is invalid!\r\n");
    return;
  }
  // add in this logic later, I don't know the math as of today how to handle this
  clientmanager.master.ackNumber = rcvd_delay_pkt->hdr.seq_num;

  // higher level code, 
  wiwi_sent_delayreq_got_delayresp(&clientmanager.master.last_tx_pkt[0], pkt);
  clientmanager.master.sequenceNumber++; // move to next sequence number

}


void wiwi_receive_one_packet() {

	packet * single_packet;
  int single_packet_index;
	wiwi_pkt_hdr * single_hdr;
	wiwi_pkt_delay * delay_pkt;
	wiwi_pkt_announce * announce_pkt;
	
	// parse the packet , not most optimal but whatever
  
  single_packet_index = rx_packet_list.shift();

  sprintf(print_buffer, "Wiwi receive one packet start index %d\r\n", single_packet_index);
  Serial.print(print_buffer);

  

  single_packet = &packet_buffer[single_packet_index];
	
	single_hdr = (wiwi_pkt_hdr *)single_packet->data;
  for ( int i = 0; i < single_packet->pkt_len; i++ ) {
    sprintf(print_buffer, "0x%x ", single_packet->data[i]);
    Serial.print(print_buffer);
  }
  Serial.println("");

	
	// WiWi packet check, packet contents are raw, need htonlz if sent as uint32_t 
	if ( single_hdr->wiwi_id != htonl(0x77697769) ) {
    Serial.print("WiWi id check fail, end receive one packet!\r\n");
    free_packet_list.add(single_packet_index); // make sure this is freed back
		return;
	}	

	// broadcast or unicast packet to me check
	if ( single_hdr->mac_dest != 0xff &&
		single_hdr->mac_dest != wiwi_mac_addr && 
    single_hdr->mac_src != wiwi_mac_addr ) {
      sprintf(print_buffer, "WiWi mac dest not matching, end early! 0x%x 0x%x\r\n",
        single_hdr->mac_dest, wiwi_mac_addr);
      Serial.print(print_buffer);
      free_packet_list.add(single_packet_index); // make sure this is freed back
      return; 
	}			

	// WiWi board is tricky, you'll receive your own transmission
  sprintf(print_buffer, "Got valid WiWi pkt type %d srcmac 0x%x destmac 0x%x seq=0x%x ack=0x%x\r\n",
    single_hdr->pkt_type, single_hdr->mac_src, single_hdr->mac_dest, 
    single_hdr->seq_num, single_hdr->ack_num);
  Serial.print(print_buffer);

  // checksum check
  uint8_t rcvd_checksum = 0;
  uint8_t calc_checksum = 0;
  if ( single_hdr->pkt_type == WIWI_PKT_DELAY_REQ ||
        single_hdr->pkt_type == WIWI_PKT_DELAY_RESP ) 
  {
    delay_pkt = (wiwi_pkt_delay *) (single_hdr);
    rcvd_checksum = delay_pkt->checksum;
    for ( int i =0; i < sizeof(wiwi_pkt_hdr)-1; i++ ) {
      calc_checksum ^= single_packet->data[i];
    }
  }
  else if ( single_hdr->pkt_type == WIWI_PKT_ANNOUNCE ) {
    announce_pkt = (wiwi_pkt_announce *) (single_hdr);
    rcvd_checksum = announce_pkt->checksum;
    for ( int i =0; i < sizeof(wiwi_pkt_hdr)-1; i++ ) {
      calc_checksum ^= single_packet->data[i];
    }
  }



  if ( calc_checksum != rcvd_checksum ) {
    sprintf(print_buffer, "Network stack received one packet with invalid checksum, got 0x%x, expected 0x%x\r\n",
      rcvd_checksum, calc_checksum);
    Serial.print(print_buffer);
    free_packet_list.add(single_packet_index); // make sure this is freed back
		return; 

  }

  ///////////// TOP LEVEL OF NETWORK STACK FOR RECEIVE

  if ( single_hdr->mac_src == wiwi_mac_addr ) {
    // it's my own packet I sent out
    // WiWi will see this on purpose, so need to record IQ and timestamp 
    if ( single_hdr->pkt_type == WIWI_PKT_ANNOUNCE ) { // my last announce
      clientmanager.last_announce_timestamp = single_packet->timestamp;
      clientmanager.last_announce_phase = single_packet->phase;
      Serial.println("Network stack saw my own announce packet");
    } else if ( single_hdr->pkt_type == WIWI_PKT_DELAY_REQ ) { // I sent out delay request
      Serial.print("See my delay request go out on RX chipset\r\n");
      clientmanager.master.last_transmit_time = millis();
      clientmanager.master.last_tx_pkt[0].phase = single_packet->phase;
      clientmanager.master.last_tx_pkt[0].timestamp = single_packet->timestamp;
    } else if ( single_hdr->pkt_type == WIWI_PKT_DELAY_RESP ) {
      // I sent out a delay response
      ClientConnection* client;
      client = ConnectionManager_GetClient(&clientmanager, single_hdr->mac_dest);
      if ( client == NULL ) {
        sprintf(print_buffer, "I sent delay response, no client found! 0x%x\r\n", single_hdr->mac_dest);
        Serial.print(print_buffer);
        free_packet_list.add(single_packet_index); // make sure this is freed
        return;
      } 
      sprintf(print_buffer, "Sent delay response, store iq 0x%lx\r\n", single_packet->phase.intval);
      Serial.print(print_buffer);
      client->last_transmit_time = millis();
      client->last_tx_pkt[0].phase = single_packet->phase;
      client->last_tx_pkt[0].timestamp = single_packet->timestamp;

    }
  } else {
    // it's a packet from someone else!
    if ( single_hdr->pkt_type == WIWI_PKT_ANNOUNCE ) { // someone else's announce
      Serial.print("See someone else's announce!\r\n");
      announce_pkt = (wiwi_pkt_announce*)single_packet->data;
      if ( clientmanager.master.isValid &&
            clientmanager.master.clientMac == single_hdr->mac_src ) {
        // this is my current master        
        wiwi_receive_valid_master_announce(single_packet, announce_pkt);
      } else if ( !clientmanager.master.isValid ) {
        // I don't have a master, maybe I want to follow this guy
        wiwi_receive_potential_master_announce(single_packet, announce_pkt);
      }
    } else if ( single_hdr->pkt_type == WIWI_PKT_DELAY_RESP ) {
      Serial.print("See someone else's delay response!\r\n");
      wiwi_receive_potential_delay_resp(single_packet);
      // HACK, IMMEDIATELY SEND ANOTHER REQUEST
#ifdef HACK_IMMEDIATE_REPLY
      wiwi_client_send_master_request(0);
      last_master_request_time = millis();
#endif
    } else if ( single_hdr->pkt_type == WIWI_PKT_DELAY_REQ ) {
      Serial.print("See someone else's delay request!\r\n");
      wiwi_receive_delay_req(single_packet);
    }
  }

/*
	if ( single_hdr->pkt_type == WIWI_PKT_DELAY_REQ ) {
		delay_pkt = (wiwi_pkt_delay *)single_packet->data;
		
		// if it's my own packet, fill in clientmanager last_tx_pkt 
		// timestamp and iq 
	}
	else if (single_hdr->pkt_type == WIWI_PKT_DELAY_RESP ) {
		delay_pkt = (wiwi_pkt_delay *)single_packet->data;
	}
	else if (single_hdr->pkt_type == WIWI_PKT_ANNOUNCE ) {
		announce_pkt = (wiwi_pkt_announce*)single_packet->data;
	}
  */

  // I'm done processing this packet, free it back
  //Serial.printf("Receive path done, free index %d\r\n", single_packet_index);
  free_packet_list.add(single_packet_index);

  return;
}

void wiwi_receive_packets() {
  Serial.println("wiwi receive packets start");
	while ( rx_packet_list.size() > 0 ) {
		// loop through each packet
		wiwi_receive_one_packet();
    Serial.print("\r\n");	
	}
}


void wiwi_clean_client_list() {
	ClientConnection * client;
	for (int i = 0; i < MAX_CONNECTIONS; i++ ) {
		client = (ClientConnection*)(&clientmanager.clients[i]);
		if (client->isValid) {
			// currently flagged as valid, check if still true
			if ( client->sequenceNumber == client->ackNumber ) {
				// everything I've sent has been acknowledged, good
				continue;
			}
			if ( client->last_transmit_time > client->last_receive_time ) {
				// I transmitted something and didn't get a reply
				if ( client->last_transmit_time - client->last_receive_time >
					client_timeout_interval ) {
					client->isValid = false;
				}
			}
		}
	}
}

/***************************** Packet Transmission logic ************/



void send_announce_message() {	
	if ( tx_packet_list.size() >= PACKET_BUFFER_SIZE ) {
    Serial.println("Send announce message, tx packet list full");
		return; // packet buffer full
	}
  if ( free_packet_list.size() == 0 ){
    Serial.println("Send announce message, free packet list empty");
    return;
  } // no free buffers
	Serial.print("Send announce message start\r\n"); Serial.flush();
  
  int single_packet_index = free_packet_list.shift(); // get a free packet buffer
  //Serial.printf("send_announce_message free_packet_index %d\r\n", single_packet_index);
	packet * single_packet = &packet_buffer[single_packet_index];
	wiwi_pkt_announce * wiwi_pkt = (wiwi_pkt_announce*)single_packet->data;
	
	wiwi_pkt->hdr.wiwi_id = htonl(0x77697769);
	wiwi_pkt->hdr.mac_src = wiwi_mac_addr;
	wiwi_pkt->hdr.mac_dest = 0xff; // broadcast
	wiwi_pkt->hdr.pkt_type = WIWI_PKT_ANNOUNCE;
	wiwi_pkt->hdr.seq_num = clientmanager.announce_seq_num++;
	wiwi_pkt->hdr.ack_num = 0;
	if ( wiwi_network_mode == WIWI_MODE_MASTER ) 
		wiwi_pkt->flags = 1;
	else 
		wiwi_pkt->flags = 0;
	wiwi_pkt->previous_tx_time = clientmanager.last_announce_timestamp;
	wiwi_pkt->previous_iq = clientmanager.last_announce_phase;

  wiwi_pkt->checksum = 0;
  for ( int i = 0; i < sizeof(wiwi_pkt_hdr) -1; i++ ) {
    wiwi_pkt->checksum ^= ((uint8_t*)wiwi_pkt)[i];
  }

  single_packet->pkt_len = WIWI_PKT_ANNOUNCE_LEN;
  single_packet->phase.intval = 0;
  single_packet->timestamp = 0;
	
  sprintf(print_buffer, "send_announce_message before buffer add %d index %d\r\n", WIWI_PKT_ANNOUNCE_LEN, single_packet_index);
	Serial.print(print_buffer); Serial.flush();
	// push this packet into linked list to send out 
  tx_packet_list.add(single_packet_index);
}


void run_wiwi_network_client() {
	// Top level state machine
	if ( wiwi_state == WIWI_START ) {
    clientmanager.master.announce_count = 0; // start at zero
		wiwi_state = WIWI_SEARCH;
#ifdef DEBUG_PRINT_WIWI_NETWORK
    Serial.print("Client wiwi network goto search\r\n");
#endif
	} else if ( wiwi_state == WIWI_SEARCH ) {
		
		if ( (millis() - last_announce_time) > announce_interval &&
      !clientmanager.master.isValid ) { // don't have a master yet
#ifdef DEBUG_PRINT_WIWI_NETWORK
      Serial.print("Client wiwi network send announce packet!\r\n");
#endif
			send_announce_message();
			last_announce_time = millis();
		}
		if ( rx_packet_list.size() > 0 ) {
			// have RX packets
			wiwi_receive_packets(); 			
		}
		wiwi_clean_client_list();		
	} else if ( wiwi_state == WIWI_RUNNING ) {
		if ( rx_packet_list.size() > 0 ) {
			// have RX packets
			wiwi_receive_packets(); 			
		}
		// found a master , is master valid

		if ( (millis() - clientmanager.master.last_receive_time ) >
			master_timeout_interval ) {
			// master hasn't replied in some time, not good
      Serial.print("Master hasn't replied in timeout interval, make invalid!\r\n");
			clientmanager.master.isValid = false;
			wiwi_state = WIWI_SEARCH;
		} else if ( (clientmanager.master.ackNumber == 
				clientmanager.master.sequenceNumber-1) && 
			(millis() - last_master_request_time ) >
				master_request_interval ) {
			// master has acknowledged my previous transmission
			// and it's been long enough between requests
      Serial.print("Client send master request because interval timeout!\r\n");
      
			wiwi_client_send_master_request(0);
      last_master_request_time = millis();
      
		} else if ( (millis() - last_master_request_time) > master_request_interval ) {
      // master hasn't acknowledged my last reply, retransmit
      Serial.print("Client send master request retransmit because no acknowledge\r\n");
			wiwi_client_send_master_request(1);
			last_master_request_time = millis();
    }
		wiwi_clean_client_list();	
	}
}



void run_wiwi_network_master() {
	if ( wiwi_state == WIWI_START ) {
		wiwi_state = WIWI_RUNNING;

	} else if ( wiwi_state == WIWI_RUNNING ) {
		if ( (millis() - last_announce_time ) > announce_interval ) {
			send_announce_message();
			last_announce_time = millis();
		}
		if ( rx_packet_list.size() > 0 ) {
			// have RX packets
			wiwi_receive_packets(); 			
		}
		wiwi_clean_client_list();	
	}
}


void run_wiwi_network() {
  if ( ENABLE_WIWI_STACK ) {
    if ( wiwi_network_mode == WIWI_MODE_CLIENT ) {
      // run client code
      run_wiwi_network_client();
    } else if ( wiwi_network_mode == WIWI_MODE_MASTER ) {
      // run master code 
      run_wiwi_network_master();
    }
  } 
}

void wiwi_network_setup() {
  Serial.println("Wiwi network setup start");
  for ( int i = 0; i < PACKET_BUFFER_SIZE; i++ ) {
    free_packet_list.add(i); // all packets are free, add to free list
  }



}