
// pre-allocate sockets 
struct uwb_sock uwb_sockets[MAX_UWB_SOCKETS];
// singly linked lists 
struct uwb_sock * active_sockets_head = 0;
struct uwb_sock * inactive_sockets_head = 0;


void init_uwb_socket_stack() {
  for ( int i = 0; i < MAX_UWB_SOCKETS; i++ ) {
    uwb_sockets[i].remote_addr = 0;
    uwb_sockets[i].local_addr = 0;
    
    uwb_sockets[i].pktSendLen = 0;
    uwb_sockets[i].pktSendTimestamp.setTime(0);
    uwb_sockets[i].pktSendTime = 0;
    
    uwb_sockets[i].pktRcvdLen = 0;
    uwb_sockets[i].pktRcvdTimestamp.setTime(0);
    
    uwb_sockets[i].next = 0;
  }
  active_sockets_head = 0;
  inactive_sockets_head = 0;
}

int uwb_socket_init( uint16_t remote_addr, uint16_t local_addr ) {
  // check if a socket already exists
  for ( int i = 0; i < MAX_UWB_SOCKETS; i++ ) {
  }
  // socket doesn't exist, pull from inactive and put in active 
  struct uwb_sock * temp;
  //temp = linkedListRemove(inactive_sockets_head, inactive_sockets_head);
  if ( temp == NULL ) {
    return -1; 
  }
  temp->remote_addr = remote_addr;
  temp->local_addr = local_addr;
  //linkedListPush(active_sockets_head, temp);
  return 0;
}

int uwb_socket_stack_process() {
  struct uwb_sock * temp;
  
  temp = active_sockets_head;
  while ( temp != NULL ) {
    // process timeout , if haven't send or received a packet in 
    // some timeframe, move to inactive
    if ( (( millis() - temp->pktSendTime ) > UWB_SOCKET_INACTIVE_TIMEOUT) &&
      (( millis() - temp->pktRcvdTime ) > UWB_SOCKET_INACTIVE_TIMEOUT ) ) {
      struct uwb_sock * to_remove = temp;
      temp = temp->next;
      //linkedListRemove(active_sockets_head, to_remove);
      //linkedListPush( inactive_sockets_head, to_remove );      
    } else {
      temp = temp->next;
    }   
  }
  return 0;
}

int uwb_socket_send( struct uwb_sock * sock, uint8_t data[], uint16_t len ) {
  uint16_t bufLen;
  if ( len > MAX_PKT_SIZE ) {
    Serial.print("*******UWB_SOCKET_SEND ERROR, PKT TOO BIG "); Serial.println(len);
  }
  bufLen = MAX_PKT_SIZE > len ? len : MAX_PKT_SIZE;
  memcpy( sock->pktSend, data, bufLen );
  
  deca_tx(data, bufLen, &sock->pktSendTimestamp);  
  return bufLen;
}

int uwb_socket_rcv( struct uwb_sock * sock, uint8_t data[], uint16_t len ) { 
  if ( sock->pktRcvdLen > 0 ) {
    uint16_t copylen;
    copylen = (sock->pktRcvdLen > len) ? len : sock->pktRcvdLen;
    memcpy( sock->pktRcvd, data, copylen );
    return copylen;
  } 
  return 0;
}

