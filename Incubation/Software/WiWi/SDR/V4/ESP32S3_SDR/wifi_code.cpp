

#include "wifi_code.h"


bool parse_mac_address(const char *mac_str, uint8_t *mac_out) {
  return sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &mac_out[0], &mac_out[1], &mac_out[2],
                &mac_out[3], &mac_out[4], &mac_out[5])
         == 6;
}
bool is_valid_channel(int channel) {
  return channel >= 1 && channel <= 13;  // Common 2.4GHz Wi-Fi channels
}



/******************** Code for callback for promicious operation ********/
// Global counters
uint32_t total_packets_received = 0;  // Total packets received
uint32_t total_packets_filtered = 0;  // Packets discarded by software filters
uint32_t total_packets_mac_filtered = 0;
uint32_t total_packets_type_filtered = 0;
uint32_t total_packets_processed = 0;  // Packets that pass all filters


// Global variables for filters
uint8_t target_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  // Default: Match all MAC addresses
uint8_t target_channel = 0;                                      // Default: Match all channels (0 = any)
uint32_t target_channel_mask = 0xFFFFFFFF;                       // Match all channels
bool filter_mgmt = true;                                         // Management packets filter
bool filter_data = true;                                         // Data packets filter
bool filter_ctrl = true;                                         // Control packets filter

// limit how many packets dumped potentially
int num_rx_packets = 0;



/* 
 * Static memory allocation for the queue:
 *
 * 1. A StaticQueue_t structure to hold the queue control information.
 * 2. An array to hold the actual data items.
 */
static StaticQueue_t xQueueBuffer;
static uint8_t ucQueueStorageArea[PACKET_QUEUE_DEPTH * sizeof(Packet_t)];

/* Global queue handle */
QueueHandle_t xPacketQueue = NULL;


const char * wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
  switch(type) {
  case WIFI_PKT_MGMT: return "MGMT";
  case WIFI_PKT_DATA: return "DATA";
  default:  
  case WIFI_PKT_MISC: return "MISC";
  }
}

// interrupt handler for promiscuous, don't do printing, just push to queue
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
  /*
  if ( total_packets_received > 3 ) {
    return;    
  }
  */
  static Packet_t rcv_pkt;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;

  // copy meta data
  memcpy(&rcv_pkt.rx_ctrl, &ppkt->rx_ctrl, sizeof(wifi_pkt_rx_ctrl_t) );

  // copy packet payload
  memcpy(&rcv_pkt.data, &ppkt->payload[0], ppkt->rx_ctrl.sig_len);

  xQueueSend(xPacketQueue, &rcv_pkt, 0 ); // if queue is full, packet is dropped

  total_packets_received++;


  return;  
}

void printPacket(Packet_t * pkt)
{

  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)pkt->data;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  total_packets_received++;


  if (memcmp(hdr->addr1, target_mac, 6) != 0 && memcmp(target_mac, "\xFF\xFF\xFF\xFF\xFF\xFF", 6) != 0 &&
      memcmp(hdr->addr2, target_mac, 6) != 0 ) {    
    total_packets_filtered++;
    total_packets_mac_filtered++;
    return;
  }
  /* add this back later, need to parse from header
  // Filter by packet type
  if ((type == WIFI_PKT_MGMT && !filter_mgmt) || (type == WIFI_PKT_DATA && !filter_data) || (type == WIFI_PKT_CTRL && !filter_ctrl)) {
    total_packets_filtered++;
    total_packets_type_filtered++;
    return;
  }
  */

  SERIAL_PRINTLN("====Start of frame=====");

  printf("CHAN=%02d, RSSI=%02d,"
    " FRAMECTRL=%04x,"
    " DURATION_ID=%04x,"
    " ADDR1=%02x:%02x:%02x:%02x:%02x:%02x,"
    " ADDR2=%02x:%02x:%02x:%02x:%02x:%02x,"
    " ADDR3=%02x:%02x:%02x:%02x:%02x:%02x\n",
    pkt->rx_ctrl.channel,
    pkt->rx_ctrl.rssi,
    /* FrameCtrl */
    hdr->frame_ctrl,
    /* Duration ID */
    hdr->duration_id,
    /* ADDR1 */
    hdr->addr1[0],hdr->addr1[1],hdr->addr1[2],
    hdr->addr1[3],hdr->addr1[4],hdr->addr1[5],
    /* ADDR2 */
    hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],
    hdr->addr2[3],hdr->addr2[4],hdr->addr2[5],
    /* ADDR3 */
    hdr->addr3[0],hdr->addr3[1],hdr->addr3[2],
    hdr->addr3[3],hdr->addr3[4],hdr->addr3[5]
  ); 

}





void wifi_csi_cb(void *ctx, wifi_csi_info_t *data) {

  if (num_rx_packets == 0) 
  {
    return;
  }

  if ( !data || !data->buf ) {
    SERIAL_PRINTLN(" wifi_csi_cb no data!");
    return;
  }

  total_packets_received++;
  const uint8_t *hdr = data->hdr;
  const uint8_t *payload = data->payload;
  const wifi_pkt_rx_ctrl_t *meta = &data->rx_ctrl;
  // Parse Frame Control field
  uint16_t frame_control = hdr[0] | (hdr[1] << 8);
  uint8_t frame_type = (frame_control >> 2) & 0x03;
  uint8_t frame_subtype = (frame_control >> 4) & 0x0F;

  const uint8_t *dest_mac = data->dmac;  // Destination MAC (offset 4)
  const uint8_t *src_mac = data->mac;  // Source MAC (offset 10)




  // Filter by MAC address (compare first 6 bytes of payload for simplicity)
  // check dmac too, want every packet associated with this mac
  if (memcmp(data->mac, target_mac, 6) != 0 && memcmp(target_mac, "\xFF\xFF\xFF\xFF\xFF\xFF", 6) != 0 &&
      memcmp(data->dmac, target_mac, 6) != 0 ) {    
    total_packets_filtered++;
    total_packets_mac_filtered++;
    return;
  }
  // Filter by packet type
  if ((frame_type == WIFI_PKT_MGMT && !filter_mgmt) || (frame_type == WIFI_PKT_DATA && !filter_data) || (frame_type == WIFI_PKT_CTRL && !filter_ctrl)) {
    total_packets_filtered++;
    total_packets_type_filtered++;
    return;
  }



  SERIAL_PRINTLN("====Start of frame=====");

  // Print metadata using the buffer
  sprintf(print_buffer, "Packet Metadata:\r\n");
  SERIAL_PRINT(print_buffer);

  sprintf(print_buffer, "  Channel: %d\r\n", meta->channel);
  SERIAL_PRINT(print_buffer);

  sprintf(print_buffer, "  RSSI: %d dBm\r\n", meta->rssi);
  SERIAL_PRINT(print_buffer);

  sprintf(print_buffer, "  Phy Rate: %d\r\n", meta->rate);
  SERIAL_PRINT(print_buffer);

  sprintf(print_buffer, "  Packet Length: %d bytes\r\n", meta->sig_len);
  SERIAL_PRINT(print_buffer);

  sprintf(print_buffer, "  Timestamp: %u\r\n", meta->timestamp);
  SERIAL_PRINT(print_buffer);


  // Determine Frame Type and Subtype (Human Readable)
  const char *frame_type_str;
  const char *frame_subtype_str;

  switch (frame_type) {
    case 0:  // Management Frames
      frame_type_str = "Management";
      switch (frame_subtype) {
        case 0: frame_subtype_str = "Association Request"; break;
        case 1: frame_subtype_str = "Association Response"; break;
        case 2: frame_subtype_str = "Reassociation Request"; break;
        case 3: frame_subtype_str = "Reassociation Response"; break;
        case 4: frame_subtype_str = "Probe Request"; break;
        case 5: frame_subtype_str = "Probe Response"; break;
        case 8: frame_subtype_str = "Beacon"; break;
        case 10: frame_subtype_str = "Disassociation"; break;
        case 11: frame_subtype_str = "Authentication"; break;
        case 12: frame_subtype_str = "Deauthentication"; break;
        default: frame_subtype_str = "Unknown Management Subtype"; break;
      }
      break;
    case 1:  // Control Frames
      frame_type_str = "Control";
      switch (frame_subtype) {
        case 10: frame_subtype_str = "Power Save Poll"; break;
        case 11: frame_subtype_str = "RTS"; break;
        case 12: frame_subtype_str = "CTS"; break;
        case 13: frame_subtype_str = "ACK"; break;
        case 14: frame_subtype_str = "CF-End"; break;
        case 15: frame_subtype_str = "CF-End + CF-Ack"; break;
        default: frame_subtype_str = "Unknown Control Subtype"; break;
      }
      break;
    case 2:  // Data Frames
      frame_type_str = "Data";
      switch (frame_subtype) {
        case 0: frame_subtype_str = "Data"; break;
        case 4: frame_subtype_str = "Null"; break;
        case 8: frame_subtype_str = "QoS Data"; break;
        default: frame_subtype_str = "Unknown Data Subtype"; break;
      }
      break;
    default:
      frame_type_str = "Unknown Type";
      frame_subtype_str = "Unknown Subtype";
      break;
  }

  // Print Frame Type and Subtype
  sprintf(print_buffer, "Frame Type: %s, Subtype: %s\r\n", frame_type_str, frame_subtype_str);
  SERIAL_PRINT(print_buffer);



  // Print Source and Destination MAC addresses
  sprintf(print_buffer, "Dest MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
          dest_mac[0], dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4], dest_mac[5]);
  SERIAL_PRINT(print_buffer);

  sprintf(print_buffer, "Src MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
          src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
  SERIAL_PRINT(print_buffer);


  // Print first 20 bytes of the packet payload
  sprintf(print_buffer, "Packet Payload: ");
  SERIAL_PRINT(print_buffer);

  for (int i = 0;  i < meta->sig_len; i++) {
    sprintf(print_buffer, "%02X ", payload[i]);
    SERIAL_PRINT(print_buffer);
  }

  if (num_rx_packets != -1) {
    num_rx_packets--;
    if (num_rx_packets == 0) {
      // disable listening
      SERIAL_PRINTLN("Done listening!");
      //esp_wifi_set_promiscuous(false);
    }
  }
  total_packets_processed++;

  

  SERIAL_PRINTLN("\r\n====END OF FRAME ====");
  SERIAL_PRINTLN("");


}




static void test_on_ping_success(esp_ping_handle_t hdl, void *args)
{
    // optionally, get callback arguments
    // const char* str = (const char*) args;
    // printf("%s\r\n", str); // "foo"
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));

    sprintf(print_buffer,"%d bytes from %s icmp_seq=%d ttl=%d time=%d ms\r\n",
           recv_len, inet_ntoa(target_addr.u_addr.ip4), seqno, ttl, elapsed_time);
    SERIAL_PRINT(print_buffer);
}

static void test_on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    sprintf(print_buffer, "From %s icmp_seq=%d timeout\r\n", inet_ntoa(target_addr.u_addr.ip4), seqno);
    SERIAL_PRINT(print_buffer);
}

static void test_on_ping_end(esp_ping_handle_t hdl, void *args)
{
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    sprintf(print_buffer,"%d packets transmitted, %d received, time %dms\r\n", transmitted, received, total_time_ms);
    SERIAL_PRINT(print_buffer);
}


void start_ping(ip_addr_t * addr)
{
  esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
  ping_config.target_addr = *addr;          // target IP address
  ping_config.count = 4;    // just do fixed length ping


  /* set callback functions */
  //SERIAL_PRINTLN("Start ping start");
  esp_ping_callbacks_t cbs;
  cbs.on_ping_success = test_on_ping_success;
  cbs.on_ping_timeout = test_on_ping_timeout;
  cbs.on_ping_end = test_on_ping_end;
  cbs.cb_args = NULL;

  esp_ping_handle_t ping;
  esp_ping_new_session(&ping_config, &cbs, &ping);
  esp_ping_start(ping);
  SERIAL_PRINTLN("Ping start");
}

/**
 * Configure Dual antenna. copied source from https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiGeneric.cpp
 * https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_wifi.html
 * @param gpio_ant1 Configure the GPIO number for the antenna 1 connected to the RF switch (default GPIO2 on ESP32-WROOM-DA)
 * @param gpio_ant2 Configure the GPIO number for the antenna 2 connected to the RF switch (default GPIO25 on ESP32-WROOM-DA)
 * @param rx_mode Set the RX antenna mode. See wifi_rx_ant_t for the options.
 * @param tx_mode Set the TX antenna mode. See wifi_tx_ant_t for the options.
 * @return true on success
 */
bool setDualAntennaConfig(uint8_t gpio_ant1, uint8_t gpio_ant2, wifi_rx_ant_t rx_mode, wifi_tx_ant_t tx_mode) {

  wifi_ant_gpio_config_t wifi_ant_io;
  SERIAL_PRINTLN("setDualAntennaConfig start");

  
  if (ESP_OK != esp_wifi_get_ant_gpio(&wifi_ant_io)) {
    SERIAL_PRINTLN("Failed to get antenna configuration");
    return false;
  }
  

  wifi_ant_io.gpio_cfg[0].gpio_num = gpio_ant1;
  wifi_ant_io.gpio_cfg[0].gpio_select = 1;
  wifi_ant_io.gpio_cfg[1].gpio_num = gpio_ant2;
  wifi_ant_io.gpio_cfg[1].gpio_select = 0;

  if (ESP_OK != esp_wifi_set_ant_gpio(&wifi_ant_io)) {
    SERIAL_PRINTLN("Failed to set antenna GPIO configuration");
    return false;
  }

  // Set antenna default configuration
  wifi_ant_config_t ant_config = {
    .rx_ant_mode = WIFI_ANT_MODE_ANT0,
    .rx_ant_default = WIFI_ANT_ANT0,  // Ignored in AUTO mode
    .tx_ant_mode = WIFI_ANT_MODE_ANT1,
    .enabled_ant0 = 0,
    .enabled_ant1 = 1,
  };

  switch (rx_mode) {
    case WIFI_RX_ANT0: ant_config.rx_ant_mode = WIFI_ANT_MODE_ANT0; break;
    case WIFI_RX_ANT1: ant_config.rx_ant_mode = WIFI_ANT_MODE_ANT1; break;
    case WIFI_RX_ANT_AUTO:
      SERIAL_PRINTLN("TX Antenna will be automatically selected");
      ant_config.rx_ant_default = WIFI_ANT_ANT0;
      ant_config.rx_ant_mode = WIFI_ANT_MODE_AUTO;
      // Force TX for AUTO if RX is AUTO
      ant_config.tx_ant_mode = WIFI_ANT_MODE_AUTO;
      goto set_ant;
      break;
    default:
      SERIAL_PRINTLN("Invalid default antenna! Falling back to AUTO");
      ant_config.rx_ant_mode = WIFI_ANT_MODE_AUTO;
      break;
  }

  switch (tx_mode) {
    case WIFI_TX_ANT0: ant_config.tx_ant_mode = WIFI_ANT_MODE_ANT0; break;
    case WIFI_TX_ANT1: ant_config.tx_ant_mode = WIFI_ANT_MODE_ANT1; break;
    case WIFI_TX_ANT_AUTO:
      SERIAL_PRINTLN("RX Antenna will be automatically selected");
      ant_config.rx_ant_default = WIFI_ANT_ANT0;
      ant_config.tx_ant_mode = WIFI_ANT_MODE_AUTO;
      // Force RX for AUTO if RX is AUTO
      ant_config.rx_ant_mode = WIFI_ANT_MODE_AUTO;
      break;
    default:
      SERIAL_PRINTLN("Invalid default antenna! Falling back to AUTO");
      ant_config.rx_ant_default = WIFI_ANT_ANT0;
      ant_config.tx_ant_mode = WIFI_ANT_MODE_AUTO;
      break;
  }

set_ant:
  if (ESP_OK != esp_wifi_set_ant(&ant_config)) {
    SERIAL_PRINTLN("Failed to set antenna configuration");
    return false;
  }
  SERIAL_PRINTLN("Set antenna configuration!");
  return true;
}


void init_wifi_code() {
  // init RX packet queue, promiscuous packets get posted to this
    xPacketQueue = xQueueCreateStatic(PACKET_QUEUE_DEPTH, sizeof(Packet_t),
      ucQueueStorageArea, &xQueueBuffer);

}
