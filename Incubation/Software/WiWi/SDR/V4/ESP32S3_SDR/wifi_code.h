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
#include "ping/ping_sock.h"


// ---- lwIP protocol headers (adjust include paths as needed) ----
#include "lwip/prot/ethernet.h"
#include "lwip/prot/etharp.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip.h"
#include "lwip/prot/udp.h"
#include "lwip/prot/tcp.h"
#include "lwip/def.h"
#include "lwip/ip4_addr.h"



bool parse_mac_address(const char *mac_str, uint8_t *mac_out);
bool is_valid_channel(int channel);


typedef struct {
  unsigned frame_ctrl:16;
  unsigned duration_id:16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl:16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;



/********* RTOS queue to pass packets ********/
/* Define a packet structure. Modify it as needed for your data */

#define PACKET_QUEUE_DATA_SIZE 512
typedef struct {
  wifi_pkt_rx_ctrl_t rx_ctrl; // meta data
  uint8_t data[PACKET_QUEUE_DATA_SIZE]; // packet content
} Packet_t;

#define PACKET_QUEUE_DEPTH 16

extern QueueHandle_t xPacketQueue;


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

bool setDualAntennaConfig(uint8_t gpio_ant1, uint8_t gpio_ant2, wifi_rx_ant_t rx_mode, wifi_tx_ant_t tx_mode);

void start_ping(ip_addr_t * addr);

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type);

void printPacket(Packet_t * pkt);

// these callbacks needs to be light weight, i.e. no printing
// passes data through queues to rx task to handle based on config
// not using promicuous cb, csi cb provides the same functionality and more
void wifi_csi_cb(void *ctx, wifi_csi_info_t *data);

void wifi_rx_task(void * parameter); 

void init_wifi_code();


#endif