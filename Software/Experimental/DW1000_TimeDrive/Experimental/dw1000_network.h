
/******
Pseudo network stack to support retransmit and local RX buffering 
*******/
#define MAX_PKT_SIZE sizeof(uwb_ptp_sync_followup_pkt)
#define MAX_UWB_SOCKETS 64
#define UWB_SOCKET_INACTIVE_TIMEOUT 2000 // in millis 


struct uwb_sock {
  uint16_t remote_addr;
  uint16_t local_addr;
  
  uint8_t pktSend[MAX_PKT_SIZE];
  uint16_t pktSendLen;
  uint32_t pktSendTime; // in millis , just for simple retransmit 
  DW1000Time pktSendTimestamp; // Decawave timestamp 
  
  uint8_t pktRcvd[MAX_PKT_SIZE];
  uint16_t pktRcvdLen;
  uint32_t pktRcvdTime; // in millis, for inactivity tracking 
  DW1000Time pktRcvdTimestamp;  // Decawave timestamp 
  
  struct uwb_sock * next; 
};



