

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



// Use the antenna GPIO feature of ESP32 API
// to signal to the board whether I'm transmitting or not
// need to verify this works!
// using neopixel pin to debug
// https://github.com/espressif/esp-idf/blob/7a305c0284b7af7cd8b8f12b48f72e2685d9a363/examples/phy/antenna/main/antenna_switch_example_main.c

void disable_tx_gpio() {
  wifi_ant_gpio_config_t ant_gpio_config;

  ant_gpio_config.gpio_cfg[0].gpio_select = 0;
  ant_gpio_config.gpio_cfg[0].gpio_num = 38;
  ant_gpio_config.gpio_cfg[1].gpio_select = 0;
  ant_gpio_config.gpio_cfg[1].gpio_num = 21;  // not used on board
  ant_gpio_config.gpio_cfg[2].gpio_select = 0;
  ant_gpio_config.gpio_cfg[3].gpio_select = 0;

  esp_err_t err = esp_wifi_set_ant_gpio(&ant_gpio_config);
  if (err == ESP_OK) {
    SERIAL_PRINTLN("\r\nESP WIFI DISABLE ANT GPIO GOOD");
  } else {
    sprintf(print_buffer, "E\r\nSP WIFI DISABLE ANT GPIO BAD %d\r\n", err);
    SERIAL_PRINT(print_buffer);
    return;
  }
}
void setup_tx_gpio() {

  // Configure GPIO for antenna
  // I'm only using one GPIO, set high for TX, low for RX I guess
  wifi_ant_gpio_config_t ant_gpio_config;

  ant_gpio_config.gpio_cfg[0].gpio_select = 1;
  ant_gpio_config.gpio_cfg[0].gpio_num = 38;
  ant_gpio_config.gpio_cfg[1].gpio_select = 1;
  ant_gpio_config.gpio_cfg[1].gpio_num = 21;  // not used on board
  ant_gpio_config.gpio_cfg[2].gpio_select = 0;
  ant_gpio_config.gpio_cfg[3].gpio_select = 0;

  esp_err_t err = esp_wifi_set_ant_gpio(&ant_gpio_config);
  if (err == ESP_OK) {
    SERIAL_PRINTLN("\r\nESP WIFI SET ANT GPIO GOOD");
  } else {
    sprintf(print_buffer, "E\r\nSP WIFI SET ANT GPIO BAD %d\r\n", err);
    SERIAL_PRINT(print_buffer);
    return;
  }

  wifi_ant_config_t ant_config;
  ant_config.rx_ant_mode = WIFI_ANT_MODE_ANT1;
  ant_config.rx_ant_default = WIFI_ANT_ANT1;
  ant_config.tx_ant_mode = WIFI_ANT_MODE_ANT0;
  ant_config.enabled_ant0 = 1;
  ant_config.enabled_ant1 = 2;

  err = esp_wifi_set_ant(&ant_config);
  if (err == ESP_OK) {
    SERIAL_PRINTLN("ESP WIFI SET ANT CONFIG GOOD");
  } else {
    sprintf(print_buffer, "ESP WIFI SET ANT CONFIG BAD %d\r\n", err);
    SERIAL_PRINT(print_buffer);
  }
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

void wifi_csi_cb(void *ctx, wifi_csi_info_t *data) {

  if (num_rx_packets == 0) 
  {
    return;
  }

  if ( !data || !data->buf ) {
    SERIAL_PRINTLN(" wifi_csi_cb no data!");
    return;
  }

  const uint8_t *hdr = data->hdr;
  const uint8_t *payload = data->payload;
  const wifi_pkt_rx_ctrl_t *meta = &data->rx_ctrl;
  // Parse Frame Control field
  uint16_t frame_control = hdr[0] | (hdr[1] << 8);
  uint8_t frame_type = (frame_control >> 2) & 0x03;
  uint8_t frame_subtype = (frame_control >> 4) & 0x0F;

  const uint8_t *dest_mac = data->dmac;  // Destination MAC (offset 4)
  const uint8_t *src_mac = data->mac;  // Source MAC (offset 10)


  total_packets_received++;

  // Filter by MAC address (compare first 6 bytes of payload for simplicity)
  if (memcmp(data->mac, target_mac, 6) != 0 && memcmp(target_mac, "\xFF\xFF\xFF\xFF\xFF\xFF", 6) != 0) {
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
  sprintf(print_buffer, "Packet Payload (first 20 bytes): ");
  SERIAL_PRINT(print_buffer);

  for (int i = 0; i < 20 && i < meta->sig_len; i++) {
    sprintf(print_buffer, "%02X ", payload[i]);
    SERIAL_PRINT(print_buffer);
  }
  SERIAL_PRINT("\r\n\r\n");

  if (num_rx_packets != -1) {
    num_rx_packets--;
    if (num_rx_packets == 0) {
      // disable listening
      SERIAL_PRINTLN("Done listening!");
      //esp_wifi_set_promiscuous(false);
    }
  }
  total_packets_processed++;
}



void wifi_rx_task(void *parameter) {
}

void init_wifi_code() {
}
