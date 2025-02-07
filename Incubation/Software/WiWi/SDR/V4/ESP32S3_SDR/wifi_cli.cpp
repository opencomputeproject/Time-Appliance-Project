

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

void wifi_client_disconnect_operation(EmbeddedCli *cli, char *args, void *context);
static Node wifi_client_disconnect_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"disconnect",
    "Disconnect from WiFi if previously connected",
    true,
    nullptr,
    wifi_client_disconnect_operation}
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

void wifi_ping_operation(EmbeddedCli *cli, char *args, void *context);
static Node wifi_ping_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"ping",
    "Ping an IPv4 address provided",
    true,
    nullptr,
    wifi_ping_operation}
};





void rx_add_peer(EmbeddedCli *cli, char *args, void *context);
static Node rx_add_peer_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"rx_add_peer",
    "Add a peer MAC to listen for",
    true,
    nullptr,
    rx_add_peer}
};

void rx_remove_peer(EmbeddedCli *cli, char *args, void *context);
static Node rx_remove_peer_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"rx_remove_peer",
    "Remove a peer MAC to listen for",
    true,
    nullptr,
    rx_remove_peer}
};

void rx_read_packets(EmbeddedCli *cli, char *args, void *context);
static Node rx_remove_peer_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"rx_read_packets",
    "Read from promiscuous packet queue human readable, optional pass number of packets to read, 1 by default",
    true,
    nullptr,
    rx_read_packets}
};



// Define all files

static Node * wifi_files[] = { &wifi_scan_node, &wifi_get_client_info_node,
  &wifi_client_join_node, &wifi_config_packet_dump_node, &wifi_dump_packets_node,
  &wifi_dump_wifi_counters_node, &wifi_udp_test_node, &wifi_client_disconnect_node, &wifi_ping_node,
  &wifi_wiwi_add_v4_peer_node };



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


/************************* WiWi over WiFi stuff
Requires that you can hear the WiFi packets directly over the air from the peer
*******************/

void wifi_wiwi_add_v4_peer(EmbeddedCli *cli, char *args, void *context)
{

}
void wifi_wiwi_remove_v4_peer(EmbeddedCli *cli, char *args, void *context)
{

}
void wifi_wiwi_list_v4_peers(EmbeddedCli *cli, char *args, void *context)
{

}
void wifi_wiwi_set_v4_master(EmbeddedCli *cli, char *args, void *context)
{

}
void wifi_wiwi_start(EmbeddedCli *cli, char *args, void *context)
{

}
void wifi_wiwi_stop(EmbeddedCli *cli, char *args, void *context)
{

}


















/********************** General WiFi stuff **************************/

void wifi_ping_operation(EmbeddedCli *cli, char *args, void *context)
{
  // example: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/protocols/icmp_echo.html
  if ( embeddedCliGetTokenCount(args) != 1 ) {
    SERIAL_PRINTLN("Ping needs 1 argument, IPv4 address!");
    return;
  } 

  ip_addr_t target_addr;
  memset(&target_addr, 0, sizeof(target_addr));
  if ( ipaddr_aton(embeddedCliGetToken(args, 1), &target_addr) != 1 ) {
    SERIAL_PRINTLN("Failed to parse IPv4 address");
    return;
  }
  start_ping(&target_addr);



}

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

  uint64_t efuseMac = ESP.getEfuseMac();
  //sprintf(print_buffer, "Efuse MAC address: %016" PRIx64, efuseMac);
  // efuse Mac is the wifi MAC with extra zeroes
  sprintf(print_buffer, "Efuse MAC address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
    (uint8_t) (efuseMac & 0xff),
    (uint8_t) ((efuseMac>>8) & 0xff),
    (uint8_t) ((efuseMac>>16) & 0xff),
    (uint8_t) ((efuseMac>>24) & 0xff),
    (uint8_t) ((efuseMac>>32) & 0xff),
    (uint8_t) ((efuseMac>>40) & 0xff));
  SERIAL_PRINTLN(print_buffer);

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
    wifi_phy_mode_t phymode;
    esp_wifi_sta_get_negotiated_phymode(&phymode);
    if ( phymode == WIFI_PHY_MODE_LR ) {
      SERIAL_PRINTLN("Phy Mode: LR");
    } else if ( phymode == WIFI_PHY_MODE_11B) {
      SERIAL_PRINTLN("Phy Mode: 11B");
    } else if ( phymode == WIFI_PHY_MODE_11G ) {
      SERIAL_PRINTLN("Phy Mode: 11G");
    } else if ( phymode == WIFI_PHY_MODE_HT20 ) {
      SERIAL_PRINTLN("Phy Mode: HT20");
    } else if ( phymode == WIFI_PHY_MODE_HT40 ) {
      SERIAL_PRINTLN("Phy Mode: HT40");
    } else if ( phymode == WIFI_PHY_MODE_HE20 ) {
      SERIAL_PRINTLN("Phy Mode: HE20");
    } else {
      SERIAL_PRINTLN("Phy Mode: Unknown");
    }

    uint8_t primary_channel;
    wifi_second_chan_t second_chan;
    esp_wifi_get_channel(&primary_channel, &second_chan);
    if ( second_chan == WIFI_SECOND_CHAN_NONE ) {
      sprintf(print_buffer, "Primary Chan:%d , Second Chan: None", primary_channel);
      SERIAL_PRINTLN(print_buffer);
    } else if ( second_chan == WIFI_SECOND_CHAN_ABOVE ) {
      sprintf(print_buffer, "Primary Chan:%d , Second Chan: Above", primary_channel);
      SERIAL_PRINTLN(print_buffer);
    } else if ( second_chan == WIFI_SECOND_CHAN_BELOW ) {
      sprintf(print_buffer, "Primary Chan:%d , Second Chan: Below", primary_channel);
      SERIAL_PRINTLN(print_buffer);
    }
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

  setDualAntennaConfig(11,11,WIFI_RX_ANT0,WIFI_TX_ANT1 ); // setup gpios for antenna select -> GPIO11 , ESP_FPGA_SPARE1 is TX transmit signal from ESP

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

void wifi_client_disconnect_operation(EmbeddedCli *cli, char *args, void *context)
{
  if ( WiFi.status() != WL_CONNECTED ) {
    sprintf(print_buffer, "Not Connected\r\n", print_buffer);
    SERIAL_PRINT(print_buffer);
  } else {
    WiFi.disconnect(false);
    SERIAL_PRINTLN("Disconnected!");
  }
}




static const char *netif_ifkeys[4] = {"WIFI_STA_DEF", "WIFI_AP_DEF", "ETH_DEF", "PPP_DEF"};

static esp_err_t tcpip_adapter_get_netif(int tcpip_if, void **netif) {
  *netif = NULL;
  if (tcpip_if < 4) {
    esp_netif_t *esp_netif = esp_netif_get_handle_from_ifkey(netif_ifkeys[tcpip_if]);
    if (esp_netif == NULL) {
      return ESP_FAIL;
    }
    int netif_index = esp_netif_get_netif_impl_index(esp_netif);
    if (netif_index < 0) {
      return ESP_FAIL;
    }

    *netif = (void *)netif_get_by_index(netif_index);

  } else {
    *netif = netif_default;
  }
  return (*netif != NULL) ? ESP_OK : ESP_FAIL;
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
  ip_protocol = IPPROTO_UDP; //IPPROTO_IP;

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

  //static const char *payload = "Message from ESP32 this is a long payload blah blah blah blah huge payloadasdfffffffffffffffffffffffffffffffffffff";
  static const char *payload = "hi";

  uint32_t start_time = micros();
  int err = sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err < 0) {
    sprintf(print_buffer, "Error occurred during sending: errno %d\r\n", errno);
    SERIAL_PRINT(print_buffer);
    return;
  }
  //LINK_STATS_INC(link.memerr); // this works, lower level lwip stats are accessible directly!


  void * nif;
  struct netif *netif;
  tcpip_adapter_get_netif(0, &nif);
  netif = (struct netif *) nif;
  sprintf(print_buffer, "Loop first = %p", netif);
  SERIAL_PRINTLN(print_buffer);
  sprintf(print_buffer, "Nif IP: 0x%x", netif->ip_addr.u_addr.ip4.addr); // THIS IS THE RIGHT STRUCTURE, CAN GET THE IPv4 address of WIFI direct from netif
  SERIAL_PRINTLN(print_buffer);

  // from here can access the same netif that the low level lwip uses in this code
  // https://github.com/espressif/esp-lwip/blob/392707e5c3a5d8e2531461db77843b9759cd5a43/src/core/netif.c#L1178

  uint32_t end_time = micros();
  SERIAL_PRINTLN("Message sent");
  sprintf(print_buffer, "Payload Size = %d, time in micros = %d\r\n", strlen(payload), end_time - start_time);
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
  if ( temp_num_rx_packets == 0 ) {
    SERIAL_PRINTLN("Listen for zero packets, just disconnect");
    return;
  }
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
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);


  /* Don't use CSI path for now, just use normal promiscuous mode
    //< default config 
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
  */

  SERIAL_PRINTLN("WAITING FOR PACKETS, Use Ctrl+C to end early");
  // read queue

  Packet_t receivedPacket;
  uint8_t read_char;
  while ( temp_num_rx_packets != 0 ) {
    // this is blocking, check for Ctrl+C
    // ESP is annoying, need to check Serial and Serial0
    while ( Serial.available() > 0) {
      // ctrl c is ascii 0x3
      read_char = Serial.read();
      if ( read_char == 0x3 ) {
        SERIAL_PRINTLN("Got ctrl+C, end packet capture!");
        temp_num_rx_packets = 0;
        WiFi.disconnect();
        return;
      }
    }
    while ( Serial0.available() > 0) {
      // ctrl c is ascii 0x3
      read_char = Serial0.read();
      if ( read_char == 0x3 ) {
        SERIAL_PRINTLN("Got ctrl+C, end packet capture!");
        temp_num_rx_packets = 0;
        WiFi.disconnect();
        return;
      }
    }

    // check for queued packet from receiver thread
    if (xQueueReceive(xPacketQueue, &receivedPacket, 0) == pdPASS)
    {
      printPacket(&receivedPacket);
      temp_num_rx_packets--;
      yield(); // make sure to give scheduler some time
    } else {
      delay(1); // let scheduler do something
    }
  }

  SERIAL_PRINTLN("Got all packets, end capture");
  //WiFi.disconnect();
  esp_wifi_set_promiscuous(false); // stop promiscuous mode 

  // clear the queue as well, clean-up
  while ( xQueueReceive(xPacketQueue, &receivedPacket, 0) == pdPASS )
  {
    yield();
  }
  return;
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