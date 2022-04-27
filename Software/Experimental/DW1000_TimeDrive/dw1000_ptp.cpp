#include "dw1000_ptp.h"
#include <DW1000.h>
#include <DW1000Time.h>
#include <DW1000Constants.h>
#include "dw1000_regs.h"
#include <require_cpp11.h>
#include <Arduino.h>
#include <SPI.h>









uint8_t seq_num = 0;


// software data buffers
#define BUF_LEN_DATA 512
uint16_t data_len;
byte rx_data[BUF_LEN_DATA];
byte tx_data[BUF_LEN_DATA];
DW1000Time rxTimeStamp;
DW1000Time txTimeStamp; 
DW1000Time * store_tx = 0; 

volatile boolean sentPkt = false;
volatile boolean received = false;
volatile boolean error = false;

uint8_t fsm_state = TD_IDLE;



uint32_t time_since_last_sync = 0;
uint8_t sync_num = 0;
DW1000Time PollTXTime;
DW1000Time PollRXTime;
DW1000Time FinalRXTime;
DW1000Time ResponseTXTime;
DW1000Time CalculatedTOFDelay;

uint32_t last_time_rangefound = 0;

uint8_t num_gugs_found = 0;



// Time stick only needs to parse up to three sync+followup packets per burst 
// Need to see packet 0 , record my RX timestamp 0
// See packet 1, record RX timestamp 1
// See packet 2, record RX timestamp 2
// Use followup info from packet 1 for packet zero
// Use followup info from packet 2 for packet 1
// The bursts allow me to miss some packets , as long as I see three I'm ok
// Seeing 3 in a single burst lets me do frequency / time correction for that burst
// State machine: 
#define TIMESTICK_SYNC_STATE_IDLE 0
#define TIMESTICK_SYNC_STATE_GOTONE 1 
#define TIMESTICK_SYNC_STATE_GOTTWO 2
struct uwb_ptp_sync_followup_pkt timestick_seen_follow_ups[2];
DW1000Time sync_followup_rx_time[2]; // just need to store first one, second rx time is processed along with the packet incoming
uint32_t time_last_sync_seen = 0; // simple millis, just to differentiate between bursts, this + sequence number of packet
uint8_t timestick_sync_state = TIMESTICK_SYNC_STATE_IDLE;

/* 
Pseudo single socket supporting retransmit
Send a packet to a specific address, wait for a non-sync broadcast reply
 */

uint16_t tx_to_addr = 0; 
uint16_t rx_my_addr = 0;
uint16_t last_pkt_size = 0;
bool socket_open = false; 
const uint32_t retrans_timer = 250; // time in millis between retransmit
const uint8_t retrans_timeout = 10; // how many times to retransmit
uint8_t retrans_count = 0;
uint32_t time_sent_req; // just use millis for retransmit
DW1000Time * last_store_txTS; 






void print_pkt(byte data[], int len) {

  for ( int i = 0; i < len; i++ ) {
    SerialUSB.print("0x"); SerialUSB.print( data[i] , HEX); SerialUSB.print(" ");
  }
  SerialUSB.println("");
}



void handleReceived() {
  // status change on reception success
  received = true;
}


void handleError() {
  error = true;
}

void handleSent() {
  // status change on sent success
  sentPkt = true;
}

void receiver() {
  DW1000.newReceive();
  DW1000.setDefaults();
  // so we don't need to restart the receiver manually
  DW1000.receivePermanently(true);
  DW1000.startReceive();
}

void initTransmit() {
  DW1000.newTransmit();
  DW1000.setDefaults();
}


void decawave_ptp_init() {
  if ( is_gug ) 
    SerialUSB.println(F("### DW1000-arduino-ranging-anchor ###"));
  else
    SerialUSB.println(F("### DW1000-arduino-ranging-tag ###"));

  char msg[128];



  // EDIT DW1000 LIBRARY TO ALLOW FOR DIFFERENT SPI 
  while ( 1 ) {
    // initialize the driver
    DW1000.begin_newspi(PIN_DW_IRQ, PIN_DW_RST, &mySPI);
    DW1000.select(PIN_DW_SS);
    SerialUSB.println(F("DW1000 initialized ..."));
    // general configuration
    DW1000.newConfiguration();
    DW1000.setDefaults();
  if ( is_gug ) 
    DW1000.setDeviceAddress(1);
  else
    DW1000.setDeviceAddress(2);
    DW1000.setNetworkId(10);
    DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_ACCURACY);
    DW1000.commitConfiguration();
    SerialUSB.println(F("Committed configuration ..."));
    // DEBUG chip info and registers pretty printed
    
    DW1000.getPrintableDeviceIdentifier(msg);
    SerialUSB.print("Device ID: "); SerialUSB.println(msg);
    if ( msg[0] != 'D' && msg[1] != 'E' &&
      msg[2] != 'C' && msg[3] != 'A' ) {
      SerialUSB.println("Didn't get DECA, waiting for DECA!");
      delay(1000);
      DW1000.reset();
    } else {
      break;
    }
  }
  DW1000.getPrintableExtendedUniqueIdentifier(msg);
  SerialUSB.print("Unique ID: "); SerialUSB.println(msg);
  DW1000.getPrintableNetworkIdAndShortAddress(msg);
  SerialUSB.print("Network ID & Device Address: "); SerialUSB.println(msg);
  DW1000.getPrintableDeviceMode(msg);
  SerialUSB.print("Device mode: "); SerialUSB.println(msg);
  // attach callback for (successfully) sent and received messages
  DW1000.attachSentHandler(handleSent);
  DW1000.attachReceivedHandler(handleReceived);
  DW1000.attachErrorHandler(handleError);

  // start reception
  receiver();
}


void deca_tx(byte data[], uint16_t n, DW1000Time * txTS) {

  if ( is_gug ) 
    SerialUSB.print("GUG ");
  else
    SerialUSB.print("TimeStick ");
  SerialUSB.print(" deca_tx "); SerialUSB.print(n); SerialUSB.println(" bytes");
  initTransmit();
  DW1000.setData( data, n );
  if ( txTS != NULL ) {
    store_tx = txTS;
  } else {
    store_tx = &txTimeStamp;
  }
  DW1000.startTransmit();
}

bool deca_tx_done() {
  return (store_tx == 0);
}
bool deca_rx_has_data() {
  return (data_len > 0);
}

void deca_loop() {
  /* Basic receiver */
  if (received) {
    received = false; 
    // get data as bytes and number of bytes received 
    data_len = DW1000.getDataLength(); 
    DW1000.getData( rx_data, BUF_LEN_DATA );
    DW1000.getReceiveTimestamp(rxTimeStamp);

    /*
    if ( is_gug )
      SerialUSB.print("GUG ");
    else
      SerialUSB.print("TimeStick ");
    SerialUSB.print("Received Data is ... "); print_pkt(rx_data, data_len);
    */
    //SerialUSB.print("FP power is [dBm] ... "); SerialUSB.println(DW1000.getFirstPathPower());
    //SerialUSB.print("RX power is [dBm] ... "); SerialUSB.println(DW1000.getReceivePower());
    //SerialUSB.print("Signal quality is ... "); SerialUSB.println(DW1000.getReceiveQuality());
  } else {
    data_len = 0; 
  }  
  if ( sentPkt ) {
    // store the timestamp
    sentPkt = false;
    DW1000.getTransmitTimestamp(txTimeStamp);
    if ( store_tx != 0 ) {
      *store_tx = txTimeStamp;
      store_tx = 0;
    }
    receiver(); // enable receiving 
  
    if ( is_gug ) 
      SerialUSB.print("GUG ");
    else
      SerialUSB.print("TimeStick ");

    SerialUSB.print("Got TX timestamp "); SerialUSB.println(txTimeStamp.getAsMicroSeconds());
  }
  if (error) {
    SerialUSB.println("Error receiving a message");
    error = false;
    //DW1000.getData(0);
    //SerialUSB.println("Error data is ... "); 
  }  
}



void open_pseudo_socket(uint16_t remote, uint16_t local) {
  if ( socket_open ) return;
  
  tx_to_addr = remote;
  rx_my_addr = local;
  socket_open = true; 
  time_sent_req = 0;
  retrans_count = 0;
}
bool pseudo_socket_is_open() {
  return socket_open;
}

void close_pseudo_socket() {
  tx_to_addr = 0;
  rx_my_addr = 0;
  socket_open = false;
  time_sent_req = 0;
  retrans_count = 0;
}

void pseudo_socket_send(uint16_t n, DW1000Time * txTS) {
  if ( !deca_tx_done() ) return; // last TX hasn't finished
  deca_tx( tx_data, n, txTS );
  time_sent_req = millis();
  last_pkt_size = n;
  last_store_txTS = txTS;
  retrans_count = 0;
  SerialUSB.print("Pseudo socket send "); SerialUSB.println(last_pkt_size);
}

void psuedo_socket_resend() {
  if ( !deca_tx_done() ) return; // last TX hasn't finished 
  deca_tx( tx_data, last_pkt_size, last_store_txTS );  
  time_sent_req = millis();
  retrans_count++;
  SerialUSB.print("Pseudo socket resend "); SerialUSB.println(last_pkt_size);
}

bool pseudo_socket_need_retx() {
  return ( socket_open && retrans_count < retrans_timeout &&
    (( millis() - time_sent_req) > retrans_timer ) );
}

bool pseudo_socket_retransmit_timedout() {
  return retrans_count >= retrans_timeout;
}

// use global rx_data buffer
bool is_rx_packet_for_socket() {
  struct uwb_ptp_hdr * hdr = (struct uwb_ptp_hdr *) &rx_data;
  if ( socket_open && hdr->frame_control == 0x4188 && 
    hdr->dest_addr == rx_my_addr &&
    hdr->src_addr == tx_to_addr && 
    hdr->function_code != SYNC_FOLLOWUP ) 
    return true;
  return false;
}


/*
GUG state machine
TD_IDLE -> go to listen 
Broadcast -> broadcast sync+followup messages without listening inbetween
Listen -> Listen for range finding , jump to Broadcast after a while
  GUGs will NOT retransmit range finding messages 
  HOWEVER, GUG should track multiple range finding sessions and respond appropriately
  INCLUDING PROPER SEQUENCE NUMBERS PER SESSION
  This will be implemented in the future, bench testing is hard coding for single
  and ignoring sequence numbers basically
*/


void Send_Sync_Followup() {
  struct uwb_ptp_sync_followup_pkt * syncpkt;
  syncpkt = (struct uwb_ptp_sync_followup_pkt *) &tx_data;
  
  syncpkt->hdr.seq_num = seq_num;  
  if ( sync_num == 0 ) { // need to fill in software TX data buffer
    syncpkt->hdr.frame_control = 0x4188;
    syncpkt->hdr.pan_id = 0xcade;
    syncpkt->hdr.dest_addr = 0x2; // time stick, hard code for now
    syncpkt->hdr.src_addr = 0x1; // my GUG address, hard code for now
    syncpkt->hdr.function_code = SYNC_FOLLOWUP;
  } 
  syncpkt->sync_num = sync_num;
  
  syncpkt->num_syncs_sending = NUM_SYNC_RETRANSMITS;
  // ignoring GPS time for now
  
  // filling in follow up information , assuming txTimeStamp is the last packet sent
  if ( sync_num > 0 ) // store follow-up
    txTimeStamp.getTimestamp(syncpkt->followups[sync_num-1]);

  deca_tx( tx_data, SYNCFOLLOWUP_PKTSIZE, 0 );
  SerialUSB.print("Sending sync followup #"); SerialUSB.print(sync_num); 
  SerialUSB.print(" Followup val: "); SerialUSB.println(txTimeStamp.getAsMicroSeconds());
  
  sync_num++;
  seq_num = ( seq_num + 1 ) % 256;
  print_pkt(tx_data, SYNCFOLLOWUP_PKTSIZE);
}

void Send_Poll() {
  struct uwb_poll_pkt * pollpkt;
  pollpkt = (struct uwb_poll_pkt *) &tx_data;
  
  pollpkt->hdr.seq_num = seq_num;
  pollpkt->hdr.frame_control = 0x4188;
  pollpkt->hdr.pan_id = 0xcade;
  pollpkt->hdr.dest_addr = 0x2; 
  pollpkt->hdr.src_addr = 0x1;
  pollpkt->hdr.function_code = POLL_MESSAGE;
  deca_tx( tx_data, POLL_PKTSIZE, &PollTXTime );
  seq_num = ( seq_num + 1 ) % 256;
  SerialUSB.println("GUG Send Poll");
  print_pkt(tx_data, POLL_PKTSIZE);
}

void Send_Final() {
  struct uwb_final_message_pkt * finalpkt;
  finalpkt = (struct uwb_final_message_pkt *) &tx_data;
  
  finalpkt->hdr.seq_num = seq_num;
  finalpkt->hdr.frame_control = 0x4188;
  finalpkt->hdr.pan_id = 0xcade;
  finalpkt->hdr.dest_addr = 0x2; 
  finalpkt->hdr.src_addr = 0x1;
  finalpkt->hdr.function_code = FINAL_MESSAGE;
  /* Since both ends are aligned , use absolute times. Send timestamp I received Response message */
  byte storeTS[5];
  rxTimeStamp.getTimestamp(storeTS);

  
  finalpkt->respRxTimeSubPollTX = storeTS[0] + (storeTS[1] << 8) +
    (storeTS[2] << 16) + (storeTS[3] << 24);
  finalpkt->finalTxTimeSubRespRX = storeTS[4];
  deca_tx( tx_data, FINALMSG_PKTSIZE, 0 );
  seq_num = ( seq_num + 1 ) % 256;
  SerialUSB.println("GUG Send Final");
  print_pkt(tx_data, FINALMSG_PKTSIZE );
}

void gug_respond() {
  struct uwb_ptp_hdr * hdr = (struct uwb_ptp_hdr *) &rx_data;
  if ( hdr->frame_control == 0x4188 && 
    hdr->dest_addr == 0x1 ) { // GUG address, hard code for now 
    if ( hdr->function_code == RANGING_INIT_MESSAGE ) {
      // I got ranging init request, send poll
      Send_Poll();
    } else if ( hdr->function_code == RESPONSE_MESSAGE ) {
      // I got response message, send final message
      Send_Final();
    }
  } else {
    SerialUSB.print("GUG respond not proper request: frame_control:0x");
    SerialUSB.print(hdr->frame_control, HEX);
    SerialUSB.print(", dest_addr:0x");
    SerialUSB.println(hdr->dest_addr, HEX);
  }
}

void GUGFSM() {
  // state machine
  //SerialUSB.print("GUG FSM "); SerialUSB.println(fsm_state);
  if ( fsm_state == TD_IDLE ) {
    receiver();
    fsm_state = LISTEN;
    time_since_last_sync = 0;
    SerialUSB.println("GUG TD_IDLE -> LISTEN");
  } else if ( fsm_state == LISTEN ) {
    if ( deca_rx_has_data() ) {
      SerialUSB.print("GUG in listen state got packet len "); SerialUSB.println(data_len);  
      gug_respond();    
    }
    if (( millis() - time_since_last_sync ) > TIME_BETWEEN_BURSTS ) {
      fsm_state = BROADCAST;
      sync_num = 0;
      SerialUSB.println("#####################GUG LISTEN -> BROADCAST#################################");
    }
  } else if ( fsm_state == BROADCAST ) {
    //SerialUSB.print( millis() - time_since_last_sync ); SerialUSB.print(" "); 
    //SerialUSB.print((unsigned int) store_tx); SerialUSB.print(" "); SerialUSB.println(deca_tx_done());
    if  ( (( millis() - time_since_last_sync ) > TIME_BETWEEN_SYNCS_MSEC) 
        && deca_tx_done() && sync_num <= NUM_SYNC_RETRANSMITS ) {
      //SerialUSB.print("Send sync followup debug:");
      //SerialUSB.print("Store_Tx="); SerialUSB.println( (unsigned int) store_tx);
      // send out sync+followup
      Send_Sync_Followup();
      time_since_last_sync = millis();
    }    
    // sent out last sync+followup, go to listen now
    if ( sync_num > NUM_SYNC_RETRANSMITS && deca_tx_done() ) {
      receiver(); // enable RX mode 
      fsm_state = LISTEN;  
      SerialUSB.println("######################GUG BROADCAST -> LISTEN################################");
    }   
  } else { 
    SerialUSB.println("GUG UNKNOWN -> TD_IDLE");
    fsm_state = TD_IDLE;
  }  
}

/* Time stick FSM
TD_IDLE -> go to listen
Listen -> Listen for GUGs and sync+followups, store GUG data internally
Range-find -> Try to range find with any GUGs found during listening 
    Doesn't need to be done often, maybe every 10 seconds or something
    Only matters if devices are moving around 
    Time stick does retransmits after a timeout as needed
*/


void Send_Ranging_Init() {
  struct uwb_poll_pkt * pollpkt = (struct uwb_poll_pkt *) &tx_data;
  pollpkt->hdr.frame_control = 0x4188;
  pollpkt->hdr.seq_num = seq_num; 
  pollpkt->hdr.pan_id = 0xcade;
  pollpkt->hdr.dest_addr = tx_to_addr; // GUG address, hard code for now 
  pollpkt->hdr.src_addr = rx_my_addr; // time stick, hard code for now
  pollpkt->hdr.function_code = RANGING_INIT_MESSAGE; 
  pseudo_socket_send( RANGE_INIT_PKTSIZE, &txTimeStamp );
  
  seq_num = ( seq_num + 1 ) % 256;
  SerialUSB.println("Time stick send ranging init");
  print_pkt(tx_data, RANGE_INIT_PKTSIZE );
  
}

void Send_Response() {
  struct uwb_response_pkt * resppkt = (struct uwb_response_pkt *) &tx_data;
  resppkt->hdr.frame_control = 0x4188;
  resppkt->hdr.seq_num = seq_num; 
  resppkt->hdr.pan_id = 0xcade;
  resppkt->hdr.dest_addr = tx_to_addr; // GUG address, hard code for now 
  resppkt->hdr.src_addr = rx_my_addr; // time stick, hard code for now
  resppkt->hdr.function_code = RESPONSE_MESSAGE; 
  pseudo_socket_send( RESPONSE_PKTSIZE, &ResponseTXTime );
  
  seq_num = ( seq_num + 1 ) % 256;  
  SerialUSB.println("Time stick send response"); 
  print_pkt(tx_data, RESPONSE_PKTSIZE);
}

void calculate_delay() {
  SerialUSB.println("Time stick calculate delay:");
  SerialUSB.print("ResponseTXTime:"); SerialUSB.println(ResponseTXTime.getAsMicroSeconds());

  // look at received packet contents , final message
  
  struct uwb_final_message_pkt * finalpkt = (struct uwb_final_message_pkt *) &rx_data;
  byte finaltime[5];
  DW1000Time GotResponseTime; 
  finaltime[0] = finalpkt->respRxTimeSubPollTX & 0xff;
  finaltime[1] = (finalpkt->respRxTimeSubPollTX >> 8) & 0xff;
  finaltime[2] = (finalpkt->respRxTimeSubPollTX >> 16) & 0xff;
  finaltime[3] = (finalpkt->respRxTimeSubPollTX >> 24) & 0xff;
  finaltime[4] = finalpkt->finalTxTimeSubRespRX & 0xff;
  CalculatedTOFDelay.setTimestamp(finaltime);

  SerialUSB.print("GUG GotResponseTime:"); SerialUSB.println(CalculatedTOFDelay.getAsMicroSeconds());
  for ( int i = 0; i < 5; i++ ) {
    SerialUSB.print(" 0x"); SerialUSB.print(finaltime[i], HEX);
  }
  SerialUSB.println("");
  CalculatedTOFDelay = CalculatedTOFDelay - ResponseTXTime;
  SerialUSB.print("Calculated delay:"); SerialUSB.println(CalculatedTOFDelay.getAsMicroSeconds());
}

void TimeStickStoreFirstSync() {
  SerialUSB.print("TimeStickStoreFirstSync "); SerialUSB.println(rxTimeStamp.getAsMicroSeconds());
  memcpy( timestick_seen_follow_ups, rx_data, SYNCFOLLOWUP_PKTSIZE );
  time_last_sync_seen = millis();
  sync_followup_rx_time[0] = rxTimeStamp; 
}

void TimeStickStoreSecondSync() {
  SerialUSB.print("TimeStickStoreSecondSync "); SerialUSB.println(rxTimeStamp.getAsMicroSeconds());
  memcpy( &(timestick_seen_follow_ups[1]), rx_data, SYNCFOLLOWUP_PKTSIZE );
  time_last_sync_seen = millis();
  sync_followup_rx_time[1] = rxTimeStamp; 
}

bool isRxInSameBurst() {
  struct uwb_ptp_sync_followup_pkt * rx_sync_followup = (struct uwb_ptp_sync_followup_pkt*) &rx_data;
  
  // sanity, is TD_IDLE 
  if ( timestick_sync_state == TIMESTICK_SYNC_STATE_IDLE ) {
    return true; 
  }
  
  // has it been too long since last sync seen 
  if ( (millis() - time_last_sync_seen) > TIME_BETWEEN_BURSTS/2) return false;
  
  // check if this incoming packet is in the same burst 
  // compute the sequence number I expect based on previously seen sync 
  uint8_t expect_seq = 0;
  expect_seq = timestick_seen_follow_ups[0].hdr.seq_num + 
    (rx_sync_followup->sync_num - timestick_seen_follow_ups[0].sync_num);
  expect_seq = expect_seq % 256;
  if ( rx_sync_followup->hdr.seq_num != expect_seq ) return false;  
  return true; 
}

bool ExpectMoreFollowups() {
  struct uwb_ptp_sync_followup_pkt * rx_sync_followup = (struct uwb_ptp_sync_followup_pkt*) &rx_data;
  return ( rx_sync_followup->sync_num < rx_sync_followup->num_syncs_sending );   
}


extern int64_t picosecond_offset; // absolute offset 
extern double frequency_ratio; // ratio of remote frequency / local frequency 
extern bool update_dpll;  // PTP will return offset and frequency to adjust, 
// other algorithm (servo) determines whether to do a phase jump or frequency adjust

void ComputeThreePointCorrection() {
  // got three packets in a single burst
  // more recent one is current rx_data
  // other two are stored
  // compute the delta to adjust from this data set
  // ALSO compute using the TOF estimate 
    
  DW1000Time time1;
  DW1000Time time2;
  DW1000Time time3;  
  struct uwb_ptp_sync_followup_pkt * rx_sync_followup = (struct uwb_ptp_sync_followup_pkt*) &rx_data;
  
  update_dpll = true;

  // figure out which follow-up to look at in the follow-up list
  // just look at current RX_data for the follow-ups 

  SerialUSB.println("////////////ComputeThreePointCorrection");
  SerialUSB.print("NEED TO ADD IN CalculatedTOF:"); SerialUSB.println(CalculatedTOFDelay.getAsMicroSeconds());
  
  // compute offset first, easier. Just need info from one packet 
  time1.setTimestamp( rx_sync_followup->followups[ timestick_seen_follow_ups[0].sync_num ] );
  SerialUSB.print("First followup value:"); SerialUSB.println(time1.getAsMicroSeconds());  
  time1 = time1 - sync_followup_rx_time[0]; 
  SerialUSB.print("RX time:"); SerialUSB.println(sync_followup_rx_time[0].getAsMicroSeconds());
  SerialUSB.print("Calculated offset in decawave units:"); SerialUSB.println(time1.getAsMicroSeconds());
  
  picosecond_offset = (int64_t)(((double)time1.getTimestamp()) * 15.65);  // approx 15.65 ps per tick in decawave
  // rounding to nearest picosecond is fine, won't ever get that precise anyways

  SerialUSB.print("picosecond_offset "); //SerialUSB.println(picosecond_offset);

  // compute frequency ratio of local clock versus remote clock
  time1.setTimestamp(rx_sync_followup->followups[ timestick_seen_follow_ups[0].sync_num ]);
  time2.setTimestamp(rx_sync_followup->followups[ timestick_seen_follow_ups[1].sync_num ]);
  SerialUSB.print("Remote time1:"); SerialUSB.println(time1.getAsMicroSeconds());
  SerialUSB.print("Remote time2:"); SerialUSB.println(time2.getAsMicroSeconds());
  time1 = time2 - time1; // remote time difference
  SerialUSB.print("Remote time difference:"); SerialUSB.println(time1.getAsMicroSeconds(), 9);

  
  time3 = sync_followup_rx_time[1] - sync_followup_rx_time[0]; // local time difference 
  SerialUSB.print("Local time0:"); SerialUSB.println(sync_followup_rx_time[0].getAsMicroSeconds());
  SerialUSB.print("Local time1:"); SerialUSB.println(sync_followup_rx_time[1].getAsMicroSeconds());
  SerialUSB.print("Local time difference:"); SerialUSB.println(time3.getAsMicroSeconds(), 9);

  // these are in absolute time , compute the ratio in frequency
  frequency_ratio = ( (double) time1.getTimestamp() ) / ( (double) time3.getTimestamp() );
  
  SerialUSB.print("////////////ComputeThreePointCorrection frequency_ratio:"); SerialUSB.println(frequency_ratio, 15);
}

void TimeStickHandleSyncFollowup() {
  struct uwb_ptp_sync_followup_pkt * rx_sync_followup = (struct uwb_ptp_sync_followup_pkt*) &rx_data;
  
  if ( timestick_sync_state == TIMESTICK_SYNC_STATE_IDLE ) {
    // check if I can expect more packets in this burst     
    if ( ExpectMoreFollowups() ) { 
      // if not last one, can expect more followups
      TimeStickStoreFirstSync();
      timestick_sync_state = TIMESTICK_SYNC_STATE_GOTONE;
      return;
    }
  } else if ( timestick_sync_state == TIMESTICK_SYNC_STATE_GOTONE ) {    
    if ( !isRxInSameBurst() ) {
      if ( ExpectMoreFollowups() ) {  
        // I can expect more in this burst , consider this my new first one 
        TimeStickStoreFirstSync();     
        return;
      } else {
        // Don't expect more in this burst, go back to idle 
        timestick_sync_state = TIMESTICK_SYNC_STATE_IDLE;
        return;
      }
    }     
    // Ok, at this point , this sync packet is a followup to one I saw before
    TimeStickStoreSecondSync();
    timestick_sync_state = TIMESTICK_SYNC_STATE_GOTTWO;
  } else if ( timestick_sync_state == TIMESTICK_SYNC_STATE_GOTTWO ) {
    // I got two in a single burst previously
    // check if this is a third one in the same burst or part of a new burst
    if ( !isRxInSameBurst() ) { // part of a new burst 
      if ( ExpectMoreFollowups() ) {
        TimeStickStoreFirstSync();
        timestick_sync_state = TIMESTICK_SYNC_STATE_GOTONE;
        return; 
      } else { 
        timestick_sync_state = TIMESTICK_SYNC_STATE_IDLE;
        return;
      }      
    } else { 
      ComputeThreePointCorrection(); 
      // got all info I need from this burst for now
      // maybe could optimize this code path, but assume good enough
      timestick_sync_state = TIMESTICK_SYNC_STATE_IDLE;
      return;
    }    
  } else {
    // bad, unknown, go back to idle
    timestick_sync_state = TIMESTICK_SYNC_STATE_IDLE;
  }  
}

void TimeStickListenGotPkt() {
  struct uwb_ptp_hdr * hdr = (struct uwb_ptp_hdr *) &rx_data;
  if ( hdr->frame_control == 0x4188 &&
    hdr->src_addr == 0x1 && hdr->function_code == SYNC_FOLLOWUP ) {      
      // GUG address, hard code for now
      // listen for anybody broadcasting sync_followup
      if ( num_gugs_found == 0 ) {
        SerialUSB.println("Time stick got GUG packet!");
        num_gugs_found = 1;
        // SHOULD BE SMARTER THAN THIS, just hacky for now  
      }   
      
      if ( data_len != SYNCFOLLOWUP_PKTSIZE ) {
        SerialUSB.print("Packet size not sync / followup size:");
        SerialUSB.print(data_len); SerialUSB.print(" ");
        SerialUSB.println(SYNCFOLLOWUP_PKTSIZE);
        return; // don't care
      }
      TimeStickHandleSyncFollowup();      
  } else {
    SerialUSB.println("Time stick Got packet but not from GUG");
  }    
}



void TimeStickFSM() {
  // state machine
  if ( fsm_state == TD_IDLE ) {
    receiver();
    fsm_state = LISTEN;
    SerialUSB.println("###################Time stick TD_IDLE -> LISTEN############################");
  } else if ( fsm_state == LISTEN ) {
    if ( deca_rx_has_data() ) {
      //SerialUSB.println("Time stick LISTEN got packet");
      TimeStickListenGotPkt();
    }
    if ( ( (millis() - last_time_rangefound) > TIME_BETWEEN_RANGEFIND_MSEC ) &&
      num_gugs_found > 0 ) {
      fsm_state = RANGEFIND;
      SerialUSB.println("##############Time stick LISTEN -> RANGEFIND###################");
    }
  } else if ( fsm_state == RANGEFIND ) {    
    if ( !pseudo_socket_is_open() ) {
      open_pseudo_socket( 0x1, 0x2 ); // hard code , in future should be dynamic
      // actually send the range finding request!
      Send_Ranging_Init();
    }
    // check receive path first
    if ( deca_rx_has_data() ) {
      if ( is_rx_packet_for_socket() ) {
        // got a response back , parse it and send a reply 
        struct uwb_ptp_hdr * hdr = (struct uwb_ptp_hdr *) &rx_data;
        struct uwb_ptp_hdr * senthdr = (struct uwb_ptp_hdr *) &tx_data;
        if ( hdr->function_code == POLL_MESSAGE &&
            senthdr->function_code == RANGING_INIT_MESSAGE ) {
          // got a poll message, need to send response message 
          SerialUSB.println("Time stick range find got poll");
          PollRXTime = rxTimeStamp; 
          Send_Response();
        } else if ( hdr->function_code == FINAL_MESSAGE &&
            senthdr->function_code == RESPONSE_MESSAGE ) {
          // got final message, done
          SerialUSB.println("Time stick range find got final");
          FinalRXTime = rxTimeStamp;
          close_pseudo_socket(); 
          calculate_delay();
          fsm_state = LISTEN;
          last_time_rangefound = millis();
          SerialUSB.println("###############Time stick Rangefind -> LISTEN after final message####################");
        }
      }      
    }    
    // check if I need to retransmit 
    if ( deca_tx_done() && pseudo_socket_need_retx() ) {
      psuedo_socket_resend();
    } else if ( pseudo_socket_retransmit_timedout() ) {
      // failed to get a response by retransmitting
      // go back to listening
      // if multiple GUGs, should try to rangefind with each of them
      // hard coding like this for bench setup with 1 time stick / 1 gug
      close_pseudo_socket();
      fsm_state = LISTEN;
      last_time_rangefound = millis();
      SerialUSB.println("################Time stick Rangefind -> LISTEN due to retransmit timeout##################");
    } 
  } else {
    SerialUSB.println("################Time stick UNKNOWN -> TD_IDLE####################");
    fsm_state = TD_IDLE;
  }
}

// called every loop 
void TopLevelFSM() {
  deca_loop(); // handle basic TX and RX 
  if ( is_gug ) {
    GUGFSM();
  } else {
    TimeStickFSM();
  } 
}
