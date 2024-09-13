

#include "WiWi_Network.h"


/******** WiWi network variables **********/
uint8_t wiwi_network_mode = WIWI_MODE_TAG;
uint8_t wiwi_state = WIWI_START;








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
phaseUnion phi_aa={.intval=0};
phaseUnion phi_ab={.intval=0}; 
phaseUnion phi_ba={.intval=0};
phaseUnion phi_bb={.intval=0};
void wiwi_sent_delayreq_got_delayresp(packet * delay_req, packet * delay_resp) {
  wiwi_pkt_delay * delay_req_pkt = (wiwi_pkt_delay *) delay_req->data;
  wiwi_pkt_delay * delay_resp_pkt = (wiwi_pkt_delay *) delay_resp->data;

  Serial.println("wiwi_sent_delayreq_got_delayresp");
  /*
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
    */
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
	if ( single_hdr->wiwi_id != 0x6977 ) {
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
  sprintf(print_buffer, "Got valid WiWi pkt type %d srcmac 0x%x destmac 0x%x seq=0x%x ack=0x%x type=0x%x\r\n",
    single_hdr->pkt_type, single_hdr->mac_src, single_hdr->mac_dest, 
    single_hdr->seq_num, single_hdr->ack_num, single_hdr->pkt_type);
  Serial.print(print_buffer);

  //  header checksum basically 
  uint8_t rcvd_checksum = 0;
  uint8_t calc_checksum = 0;
  rcvd_checksum = single_hdr->checksum;
  for ( int i = 0; i < sizeof(wiwi_pkt_hdr)-2; i++ ) {
	  calc_checksum ^= single_packet->data[i];
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
	// for packets involved in WiWi
    if ( single_hdr->pkt_type == WIWI_PKT_ANNOUNCE ) { // my last announce
	  if ( wiwi_network_mode == WIWI_MODE_MASTER_ANCHOR ) {
		  // announce should only be master
		  Serial.println("Received my own announce message!");
		  masterAnchor_handleSentAnnounce(single_packet);
	  }
      Serial.println("Network stack saw my own announce packet");
    } else if ( single_hdr->pkt_type == WIWI_PKT_DELAY_REQ ) { // I sent out delay request
      Serial.print("See my delay request go out on RX chipset\r\n\r\n");
	  if ( wiwi_network_mode == WIWI_MODE_MASTER_ANCHOR ) {
		  // delay req should only be master
		  masterAnchor_handleSentDelayReq(single_packet);
	  }
    } else if ( single_hdr->pkt_type == WIWI_PKT_DELAY_RESP ) {		
	  // delay Resp will be from client anchors only
	  if ( wiwi_network_mode == WIWI_MODE_SLAVE_ANCHOR ) {
		  clientAnchor_handleSentDelayResp(single_packet); 
	  }
    }
  } else {
      // it's a packet from someone else!
      if ( single_hdr->pkt_type == WIWI_PKT_ANNOUNCE ) { // someone else's announce
        // this can happen for non masters, client anchors or tags
        Serial.print("See someone else's announce!\r\n");
        if ( wiwi_network_mode == WIWI_MODE_SLAVE_ANCHOR ) {
          clientAnchor_handleReceiveAnnounce(single_packet); 
        } else if ( wiwi_network_mode == WIWI_MODE_TAG ) {
          tag_handleReceiveAnnounce(single_packet);
        }
      } else if ( single_hdr->pkt_type == WIWI_PKT_DELAY_RESP ) {
        // delay response will be picked up by everybody 
        // everybody needs to process to handle TDMA controlled by master 
        Serial.print("See someone else's delay response!\r\n");
        if ( wiwi_network_mode == WIWI_MODE_MASTER_ANCHOR ) {
          masterAnchor_handleDelayResp(single_packet);
        } else if ( wiwi_network_mode == WIWI_MODE_SLAVE_ANCHOR ) {
          clientAnchor_handleDelayResp(single_packet);
        } else if ( wiwi_network_mode == WIWI_MODE_TAG ) {
          tag_handleDelayResp(single_packet);
        }
	  } else if ( single_hdr->pkt_type == WIWI_PKT_DELAY_REQ ) {
		// delay request, tag and client anchors need to do something
		if ( wiwi_network_mode == WIWI_MODE_SLAVE_ANCHOR ) {
			clientAnchor_handleDelayReq(single_packet);
		} else if ( wiwi_network_mode == WIWI_MODE_TAG ) {
			tag_handleDelayReq(single_packet);
		}		
      } else if ( single_hdr->pkt_type == WIWI_PKT_ANCHOR_SUBSCRIBE ) {
		Serial.println("Received another nodes anchor subscribe packet");
        if ( wiwi_network_mode == WIWI_MODE_MASTER_ANCHOR ) {
          masterAnchor_handleAnchorSubscribe(single_packet);
        }
      } else if ( single_hdr->pkt_type == WIWI_PKT_TAG_SUBSCRIBE ) {
        if ( wiwi_network_mode == WIWI_MODE_MASTER_ANCHOR ) {
          masterAnchor_handleTagSubscribe(single_packet);
        }
    } else if ( single_hdr->pkt_type == WIWI_PKT_TAG_RESPONSE ) {
      // a tag response to delay req, all anchors need to process
      if ( wiwi_network_mode == WIWI_MODE_MASTER_ANCHOR ) {
        masterAnchor_handleTagResponse(single_packet);
      } else if ( wiwi_network_mode == WIWI_MODE_SLAVE_ANCHOR ) {
        clientAnchor_handleTagResponse(single_packet);
      }
    }
  }

  // I'm done processing this packet, free it back
  sprintf(print_buffer,"Receive path done, free index %d\r\n\r\n", single_packet_index);
  Serial.print(print_buffer);
  
  free_packet_list.add(single_packet_index);

  return;
}

void wiwi_receive_packets() {
  //Serial.println("wiwi receive packets start");
	while ( rx_packet_list.size() > 0 ) {
		// loop through each packet
		wiwi_receive_one_packet();
    //Serial.print("\r\n");	
	}
}





void run_wiwi_network() {
  if ( ENABLE_WIWI_STACK ) {
    wiwi_receive_packets();
    if ( wiwi_network_mode == WIWI_MODE_SLAVE_ANCHOR ) {
      // run client code
      run_wiwi_network_client();
    } else if ( wiwi_network_mode == WIWI_MODE_MASTER_ANCHOR ) {
      // run master code 
      run_masterAnchor();
    } else if ( wiwi_network_mode == WIWI_MODE_TAG ) {
      run_wiwi_network_tag();
    }
  } 
}

void wiwi_network_setup() {
  Serial.println("Wiwi network setup start");
  for ( int i = 0; i < PACKET_BUFFER_SIZE; i++ ) {
    free_packet_list.add(i); // all packets are free, add to free list
  }
}