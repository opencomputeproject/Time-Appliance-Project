

#include "wifi_cli.h"








void wifi_scan_operation(EmbeddedCli *cli, char *args, void *context);
static Node wifi_scan_node = { .name = "scan", 
  .type = MY_FILE, 
  .cliBinding = {"scan",
    "WiFi Scan",
    true,
    nullptr,
    wifi_scan_operation}
};

void wifi_get_client_info_operation(EmbeddedCli *cli, char *args, void *context);
static Node wifi_get_client_info_node = { .name = "client_info", 
  .type = MY_FILE, 
  .cliBinding = {"get_client_info",
    "Prints WiFi Client Info, MAC address, Connection status. If connected SSID / IP address ",
    true,
    nullptr,
    wifi_get_client_info_operation}
};

void wifi_client_join_operation(EmbeddedCli *cli, char *args, void *context);
static Node wifi_client_join_node = { .name = "client_join", 
  .type = MY_FILE, 
  .cliBinding = {"client_join",
    "Join a WiFi network as a client, pass SSID / password. ex. client_join MyWiFISSID Password123",
    true,
    nullptr,
    wifi_client_join_operation}
};

void wifi_configure_packet_dump_operation(EmbeddedCli *cli, char *args, void *context);
static Node wifi_config_packet_dump_node = { .name = "configure_packet_dump", 
  .type = MY_FILE, 
  .cliBinding = {"config_packet_dump",
    "Configure packet listen parameters. Options: mac (ex. ff:ff:ff:ff:ff:ff) , channel (1-13) , filter_mgmt / filter_data / filter_ctrl (true or false)",
    true,
    nullptr,
    wifi_configure_packet_dump_operation}
};

void wifi_dump_packets_operation(EmbeddedCli *cli, char *args, void *context);
static Node wifi_dump_packets_node = { .name = "listen", 
  .type = MY_FILE, 
  .cliBinding = {"listen",
    "Dump every WiFi Packet as configured, pass number of packets to dump, -1 for infinite until you send enter",
    true,
    nullptr,
    wifi_dump_packets_operation}
};

void wifi_dump_wifi_counters(EmbeddedCli *cli, char *args, void *context);
static Node wifi_dump_wifi_counters_node = { .name = "counters", 
  .type = MY_FILE, 
  .cliBinding = {"counters",
    "Prints out WiFi counters",
    true,
    nullptr,
    wifi_dump_wifi_counters}
};


void wifi_udp_test(EmbeddedCli *cli, char *args, void *context);
static Node wifi_udp_test_node = { .name = "udp_test", 
  .type = MY_FILE, 
  .cliBinding = {"udp_test",
    "Debug function for testing UDP",
    true,
    nullptr,
    wifi_udp_test}
};

// Define all files

static Node * wifi_files[] = { &wifi_scan_node, &wifi_get_client_info_node,
  &wifi_client_join_node, &wifi_config_packet_dump_node, &wifi_dump_packets_node,
  &wifi_dump_wifi_counters_node, &wifi_udp_test_node };



void wifi_dir_operation(EmbeddedCli *cli, char *args, void *context);
// Define the SPI directory
static Node wifi_dir = {
    .name = "wifi",
    .type = MY_DIRECTORY,
    .cliBinding = {"wifi",
          "WiFi operations",
          true,
          nullptr,
          wifi_dir_operation},
    .parent = 0,
    .children = wifi_files,
    .num_children = sizeof(wifi_files) / sizeof(wifi_files[0])
};



void wifi_scan_operation(EmbeddedCli *cli, char *args, void *context) {
  SERIAL_PRINTLN("WiFi Scan start");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // WiFi.scanNetworks will return the number of networks found.
  int n = WiFi.scanNetworks();
  SERIAL_PRINTLN("Scan done");
  if (n == 0) {
    SERIAL_PRINTLN("no networks found");
  } else {
    SERIAL_PRINT(n);
    SERIAL_PRINTLN(" networks found");
    SERIAL_PRINTLN("Nr | SSID                             | RSSI | CH | Encryption");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      sprintf(print_buffer, "%2d", i + 1); SERIAL_PRINT(print_buffer);
      SERIAL_PRINT(" | ");
      sprintf(print_buffer, "%-32.32s", WiFi.SSID(i).c_str()); SERIAL_PRINT(print_buffer);
      SERIAL_PRINT(" | ");
      sprintf(print_buffer, "%4ld", WiFi.RSSI(i)); SERIAL_PRINT(print_buffer);
      SERIAL_PRINT(" | ");
      sprintf(print_buffer, "%2ld", WiFi.channel(i)); SERIAL_PRINT(print_buffer);
      SERIAL_PRINT(" | ");
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN:            SERIAL_PRINT("open"); break;
        case WIFI_AUTH_WEP:             SERIAL_PRINT("WEP"); break;
        case WIFI_AUTH_WPA_PSK:         SERIAL_PRINT("WPA"); break;
        case WIFI_AUTH_WPA2_PSK:        SERIAL_PRINT("WPA2"); break;
        case WIFI_AUTH_WPA_WPA2_PSK:    SERIAL_PRINT("WPA+WPA2"); break;
        case WIFI_AUTH_WPA2_ENTERPRISE: SERIAL_PRINT("WPA2-EAP"); break;
        case WIFI_AUTH_WPA3_PSK:        SERIAL_PRINT("WPA3"); break;
        case WIFI_AUTH_WPA2_WPA3_PSK:   SERIAL_PRINT("WPA2+WPA3"); break;
        case WIFI_AUTH_WAPI_PSK:        SERIAL_PRINT("WAPI"); break;
        default:                        SERIAL_PRINT("unknown");
      }
      SERIAL_PRINTLN();
      delay(10);
    }
  }
  SERIAL_PRINTLN("");

  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();
}


void wifi_get_client_info_operation(EmbeddedCli *cli, char *args, void *context)
{
  SERIAL_PRINTLN("Wifi Client Info");
  String macAddress = WiFi.macAddress();
  sprintf(print_buffer, "MAC address: %s\r\n", macAddress.c_str());
  SERIAL_PRINT(print_buffer);

  sprintf(print_buffer, "WiFi Status:");
  if ( WiFi.status() != WL_CONNECTED ) {
    sprintf(print_buffer, "%sNot Connected\r\n", print_buffer);
    SERIAL_PRINT(print_buffer);
  } else {
    sprintf(print_buffer, "%sConnected\r\n", print_buffer);
    SERIAL_PRINT(print_buffer);
    sprintf(print_buffer, "IP address:%s\r\n", WiFi.localIP().toString().c_str() );
    SERIAL_PRINT(print_buffer);
    sprintf(print_buffer, "SSID:%s\r\n", WiFi.SSID().c_str() );
    SERIAL_PRINT(print_buffer);
  }
}

void wifi_client_join_operation(EmbeddedCli *cli, char *args, void *context)
{
  if ( embeddedCliGetTokenCount(args) != 2 ) {
    Serial.println("Client join needs 2 arguments, SSID and Password!");
    return;
  } 
  sprintf(print_buffer, "Connecting to SSID %s\r\n", embeddedCliGetToken(args, 1) );
  SERIAL_PRINT(print_buffer);

  WiFi.disconnect(false);

  WiFi.mode(WIFI_STA);

  WiFi.begin( embeddedCliGetToken(args, 1), embeddedCliGetToken(args, 2) );

  // wait for some time
  uint32_t start_time = millis();
  while ( (millis() - start_time) < 5000 ) 
  {
    if ( WiFi.status() == WL_CONNECTED ) {
      break;
    }
    SERIAL_PRINT(".");
    delay(250);
  }

  if ( WiFi.status() != WL_CONNECTED ) {
    WiFi.disconnect(false);
    SERIAL_PRINTLN("Failed to connect");
    return;
  } else {
    SERIAL_PRINTLN("Connected!");
  }
  
}





void wifi_udp_test(EmbeddedCli *cli, char *args, void *context)
{
  if ( WiFi.status() != WL_CONNECTED ) {
    SERIAL_PRINTLN("WiFi not connected!");
    return;
  } else {
    SERIAL_PRINTLN("Connected!");
  }
  IPAddress gateway = WiFi.gatewayIP(); 
  sprintf(print_buffer, "Gateway IP: %s\r\n", gateway.toString().c_str() );
  SERIAL_PRINT(print_buffer);

  //esp_err_t ret = esp_vfs_l2tap_intf_register(NULL);

  // make UDP socket to gateway just for test
  struct sockaddr_in dest_addr;
  uint16_t port = 80;
  int addr_family = 0;
  int ip_protocol = 0;


  dest_addr.sin_addr.s_addr = inet_addr( gateway.toString().c_str() );
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(port);
  addr_family = AF_INET;
  ip_protocol = IPPROTO_IP;

  int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
  if (sock < 0) {
    sprintf(print_buffer, "Unable to create socket: errno %d\r\n", errno);
    SERIAL_PRINT(print_buffer);
    return;
  }

  // Set timeout
  struct timeval timeout;
  timeout.tv_sec = 3;
  timeout.tv_usec = 0;
  setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

  sprintf(print_buffer, "Socket created, sending to %s:%d\r\n", gateway.toString().c_str(), port);
  SERIAL_PRINT(print_buffer);

  static const char *payload = "Message from ESP32 ";

  uint32_t start_time = millis();
  int err = sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err < 0) {
    sprintf(print_buffer, "Error occurred during sending: errno %d\r\n", errno);
    SERIAL_PRINT(print_buffer);
    return;
  }
  uint32_t end_time = millis();
  SERIAL_PRINTLN("Message sent");
  sprintf(print_buffer, "Start time before sendto: %d, endtime after sendto: %d\r\n", start_time, end_time);
  SERIAL_PRINT(print_buffer);

  SERIAL_PRINTLN("Shutting down socket...");
  shutdown(sock, 0);
  close(sock);
}

void wifi_configure_packet_dump_operation(EmbeddedCli *cli, char *args, void *context)
{
  if ( embeddedCliGetTokenCount(args) != 2 ) {
    Serial.println("Configure packet dump needs 2 arguments, field and value!");
    return;
  } 

  const char *field = embeddedCliGetToken(args, 1);
  const char *value = embeddedCliGetToken(args, 2);

  if (strcmp(field, "mac") == 0) {
    uint8_t mac[6];
    if (parse_mac_address(value, mac)) {
        memcpy(target_mac, mac, 6);
        sprintf(print_buffer, "MAC address filter updated: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        SERIAL_PRINT(print_buffer);
    } else {
        sprintf(print_buffer, "Invalid MAC address format. Use XX:XX:XX:XX:XX:XX\r\n");
        SERIAL_PRINT(print_buffer);
    }
  } else if (strcmp(field, "channel") == 0) {
    int channel = atoi(value);
    if (is_valid_channel(channel)) {
        target_channel = (uint8_t)channel;
        sprintf(print_buffer, "Channel filter updated: %d\r\n", channel);
        SERIAL_PRINT(print_buffer);
    } else {
        sprintf(print_buffer, "Invalid channel. Valid range: 1-13\r\n");
        SERIAL_PRINT(print_buffer);
    }  
  } else if (strcmp(field, "filter_mgmt") == 0) {
    filter_mgmt = (strcmp(value, "true") == 0);
    sprintf(print_buffer, "Management packet filter: %s\r\n", filter_mgmt ? "Enabled" : "Disabled");
    SERIAL_PRINT(print_buffer);
  } else if (strcmp(field, "filter_data") == 0) {
    filter_data = (strcmp(value, "true") == 0);
    sprintf(print_buffer, "Data packet filter: %s\r\n", filter_data ? "Enabled" : "Disabled");
    SERIAL_PRINT(print_buffer);
  } else if (strcmp(field, "filter_ctrl") == 0) {
    filter_ctrl = (strcmp(value, "true") == 0);
    sprintf(print_buffer, "Control packet filter: %s\r\n", filter_ctrl ? "Enabled" : "Disabled");
    SERIAL_PRINT(print_buffer);
  } else {
    sprintf(print_buffer, "Unknown field: %s\r\n", field);
    SERIAL_PRINT(print_buffer);
    sprintf(print_buffer, "Valid fields: mac, channel, filter_mgmt, filter_data, filter_ctrl\r\n");
    SERIAL_PRINT(print_buffer);
  }
}

void wifi_dump_packets_operation(EmbeddedCli *cli, char *args, void *context)
{
  int temp_num_rx_packets = 0;
  // needs one argument, number of packets
  if ( embeddedCliGetTokenCount(args) != 1 ) {
    SERIAL_PRINTLN("Dump packets needs 1 argument, number of packets, -1 to wait until enter!");
    return;
  } 

  if ( sscanf(embeddedCliGetToken(args, 1) , "%d", &temp_num_rx_packets) != 1 ) {
    SERIAL_PRINTLN("Failed to parse first argument as decimal integer");
    return;
  }

  sprintf(print_buffer,"Setting up to listen for %d packets\r\n", temp_num_rx_packets);
  SERIAL_PRINT(print_buffer);
  // ok good, now set to promicious mode and dump packets
  // Start Wi-Fi in STA mode (station mode required for promiscuous mode)
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  
  delay(50);

  /* from espressif csi example, not sure necessary
  if ( esp_event_loop_create_default() != ESP_OK ) SERIAL_PRINTLN("CREATE DEFAULT EVENT LOOP BAD");
  if ( esp_netif_init() != ESP_OK ) SERIAL_PRINTLN("ESP NETIF INIT BAD");

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  cfg.csi_enable = 1;
  if ( esp_wifi_init(&cfg) != ESP_OK ) SERIAL_PRINTLN("WIFI INIT DEFAULT CONFIG BAD");

  if ( esp_wifi_set_promiscuous(false) != ESP_OK ) SERIAL_PRINTLN("SET PROM FALSE BAD");  // Disable if already enabled
  //WiFi.mode(WIFI_STA);
  if ( esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ) SERIAL_PRINTLN("SET MODE STA BAD"); 
  //if ( esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK ) SERIAL_PRINTLN("SET STORAGE BAD");
  if ( esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20) != ESP_OK ) SERIAL_PRINTLN("SET BW HT20 BAD");
  if ( esp_wifi_start() != ESP_OK ) SERIAL_PRINTLN("ESP WIFI START BAD");
  //esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS0_SGI);
  if ( esp_wifi_set_ps(WIFI_PS_NONE) != ESP_OK ) SERIAL_PRINTLN("SET WIFI PS NONE BAD");
  
  */

  if ( target_channel != 0 ) {
    if ( esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE) != ESP_OK ) SERIAL_PRINTLN("ESP WIFI SET CHANNEL BAD");
  } else {
    if ( esp_wifi_set_channel(5, WIFI_SECOND_CHAN_NONE) != ESP_OK ) SERIAL_PRINTLN("ESP WIFI SET CHANNEL 5 BAD");
  }
  

  // Initialize Wi-Fi
  
  if ( esp_wifi_set_promiscuous(true) != ESP_OK ) SERIAL_PRINTLN("WIFI SET PROMICUOUS TRUE BAD");   // Enable promiscuous mode

  /**< default config */
  wifi_csi_config_t csi_config = {
      .lltf_en           = true,
      .htltf_en          = true,
      .stbc_htltf2_en    = true,
      .ltf_merge_en      = true,
      .channel_filter_en = true,
      .manu_scale        = false,
      .shift             = false,
  };
  if ( esp_wifi_set_csi_config(&csi_config) != ESP_OK ) SERIAL_PRINTLN("WIFI SET CSI CONFIG BAD");
  if ( esp_wifi_set_csi_rx_cb(wifi_csi_cb, NULL) != ESP_OK ) SERIAL_PRINTLN("WIFI SET CSI RX CB BAD");
  if ( esp_wifi_set_csi(true) != ESP_OK ) SERIAL_PRINTLN("SET CSI TRUE BAD");

  num_rx_packets = temp_num_rx_packets;
}

void wifi_dump_wifi_counters(EmbeddedCli *cli, char *args, void *context)
{

  sprintf(print_buffer, "Total Packets Received: %u\r\n", total_packets_received);
  SERIAL_PRINT(print_buffer);

  sprintf(print_buffer, "Total Packets Processed: %u\r\n", total_packets_processed);
  SERIAL_PRINT(print_buffer);

  sprintf(print_buffer, "Total Packets Filtered: %u\r\n", total_packets_filtered);
  SERIAL_PRINT(print_buffer);

  sprintf(print_buffer, "Total Packets Filtered by MAC: %u\r\n", total_packets_mac_filtered);
  SERIAL_PRINT(print_buffer);

  sprintf(print_buffer, "Total Packets Filtered by Type: %u\r\n", total_packets_type_filtered);
  SERIAL_PRINT(print_buffer);

}

























/********************** Boiler plate code for CLI ****************/

void wifi_dir_operation(EmbeddedCli *cli, char *args, void *context)
{
  //SERIAL_PRINTLN("Want to change into SPI mode!");
  change_to_node(&wifi_dir);
}

void wifi_init() {
  for (int i = 0; i < wifi_dir.num_children; i++) {
    wifi_files[i]->parent = &wifi_dir;
  }
  add_root_filesystem(&wifi_dir);
}



void init_wifi_cli()
{
  wifi_init();
  init_wifi_code();
}