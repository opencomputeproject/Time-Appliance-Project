#ifndef WIFI_CODE_H
#define WIFI_CODE_H


#include "menu_cli.h"

#include <WiFi.h>
#include "esp_wifi.h"


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "esp_netif.h"
#include "lwip/netif.h"
#include "esp_vfs_l2tap.h"

#include "esp_mac.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_now.h"


bool parse_mac_address(const char *mac_str, uint8_t *mac_out);
bool is_valid_channel(int channel);





/********* Promicious config and counters ******/

extern uint32_t total_packets_received;   // Total packets received
extern uint32_t total_packets_filtered;   // Packets discarded by software filters
extern uint32_t total_packets_mac_filtered;
extern uint32_t total_packets_type_filtered;
extern uint32_t total_packets_processed;  // Packets that pass all filters


// Global variables for filters
extern uint8_t target_mac[6]; // Default: Match all MAC addresses
extern uint8_t target_channel;  // Default: Match all channels (0 = any)
extern uint32_t target_channel_mask; // Match all channels 
extern bool filter_mgmt;     // Management packets filter
extern bool filter_data;     // Data packets filter
extern bool filter_ctrl;    // Control packets filter

// limit how many packets dumped potentially
extern int num_rx_packets; 


// these callbacks needs to be light weight, i.e. no printing
// passes data through queues to rx task to handle based on config
// not using promicuous cb, csi cb provides the same functionality and more
void wifi_csi_cb(void *ctx, wifi_csi_info_t *data);

void wifi_rx_task(void * parameter); 

void init_wifi_code();


#endif