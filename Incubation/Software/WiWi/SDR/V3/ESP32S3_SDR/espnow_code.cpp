


#include "espnow_code.h"



PeerInfo peers[MAX_PEERS] = {0};

RingBuffer rx_buffer = { .head = 0, .tail = 0 };


bool addPeer(const uint8_t *peerMac, uint8_t channel, const uint8_t *key, size_t keyLength) {
 for (int i = 0; i < MAX_PEERS; i++) {
  if (peers[i].active && memcmp(peers[i].mac, peerMac, 6) == 0) {
   return true;
  }
 }
 for (int i = 0; i < MAX_PEERS; i++) {
  if (!peers[i].active) {
   memcpy(peers[i].mac, peerMac, 6);
   peers[i].channel = channel;
   peers[i].active = true;

   esp_now_peer_info_t peerInfo = {};
   memcpy(peerInfo.peer_addr, peerMac, 6);
   peerInfo.channel = channel;
   peerInfo.encrypt = (key != nullptr);
   if (key != nullptr && keyLength > 0) {
    memcpy(peerInfo.lmk, key, keyLength);
   }

   if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    return true;
   } else {
    peers[i].active = false;
    return false;
   }
  }
 }
 return false;
}

bool removePeer(const uint8_t *peerMac) {
 for (int i = 0; i < MAX_PEERS; i++) {
  if (peers[i].active && memcmp(peers[i].mac, peerMac, 6) == 0) {
   if (esp_now_del_peer(peerMac) == ESP_OK) {
    peers[i].active = false;
    return true;
   } else {
    return false;
   }
  }
 }
 return false;
}

void listPeers() {
 SERIAL_PRINTLN("List of Active Peers:");
 for (int i = 0; i < MAX_PEERS; i++) {
  if (peers[i].active) {
    sprintf(print_buffer, "Peer %d: MAC=%02X:%02X:%02X:%02X:%02X:%02X, Channel=%d\r\n", i + 1,
                 peers[i].mac[0], peers[i].mac[1], peers[i].mac[2],
                 peers[i].mac[3], peers[i].mac[4], peers[i].mac[5],
                 peers[i].channel);
    SERIAL_PRINT(print_buffer);
  }
 }
}

bool setChannel(uint8_t channel) {
 if (esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE) == ESP_OK) {
  return true;
 }
 return false;
}

uint8_t getChannel() {
 uint8_t channel;
 wifi_second_chan_t secondChannel;
 esp_wifi_get_channel(&channel, &secondChannel);
 return channel;
}

bool sendData(const uint8_t *peerMac, const uint8_t *data, size_t dataLength) {
 if (esp_now_send(peerMac, data, dataLength) == ESP_OK) {
  return true;
 }
 return false;
}

bool broadcastData(const uint8_t *data, size_t dataLength) {
 uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
 return sendData(broadcastMac, data, dataLength);
}


/********* Data reception APIs *******/

bool hasReceivedData() {
  return rx_buffer.head != rx_buffer.tail;
}
ReceivedPacket* popReceivedData() {
  if (!hasReceivedData()) {
      return nullptr; // No data available
  }

  // Get the packet at the tail
  ReceivedPacket *packet = &rx_buffer.packets[rx_buffer.tail];

  // Advance the tail
  rx_buffer.tail = (rx_buffer.tail + 1) % RX_BUFFER_SIZE;

  return packet;
}



void onDataReceived(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
 if (data_len > MAX_DATA_LEN) {
     // Ignore packets that are too large
     return;
 }

 int next_head = (rx_buffer.head + 1) % RX_BUFFER_SIZE;

 if (next_head == rx_buffer.tail) {
     // Buffer is full, drop the packet
     return;
 }

 // Add the packet to the ring buffer
 ReceivedPacket *packet = &rx_buffer.packets[rx_buffer.head];
 memcpy(packet->mac, esp_now_info->src_addr, 6);
 packet->rssi = esp_now_info->rx_ctrl->rssi;
 packet->channel = esp_now_info->rx_ctrl->channel;
 packet->data_len = data_len;
 memcpy(packet->data, data, data_len);

 // Advance the head
 rx_buffer.head = next_head;
}

void init_espnow_code()
{
  return;
 if (esp_now_init() != ESP_OK) {
  SERIAL_PRINTLN("Error initializing ESP-NOW");
  return;
 }
 esp_now_register_recv_cb(onDataReceived);
 SERIAL_PRINTLN("ESP-NOW Initialized");
}