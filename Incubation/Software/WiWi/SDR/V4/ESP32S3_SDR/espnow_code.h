#ifndef ESPNOW_CODE_H
#define ESPNOW_CODE_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "menu_cli.h"
#include "esp_wifi.h"


#define MAX_PEERS 10

typedef struct {
 uint8_t mac[6];
 uint8_t channel;
 bool active;
} PeerInfo;

extern PeerInfo peers[MAX_PEERS];

#define RX_BUFFER_SIZE 10  // Maximum number of buffered packets
#define MAX_DATA_LEN 250   // Maximum data length per packet

typedef struct {
 uint8_t mac[6];
 int rssi;
 int channel;
 size_t data_len;
 uint8_t data[MAX_DATA_LEN];
} ReceivedPacket;

typedef struct {
 ReceivedPacket packets[RX_BUFFER_SIZE];
 volatile int head;
 volatile int tail;
} RingBuffer;










/******* From this code: https://github.com/espressif/esp-idf/blob/v5.4/examples/wifi/espnow/main/espnow_example_main.c ***/

/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

#define ESPNOW_QUEUE_SIZE           6

#define IS_BROADCAST_ADDR(addr) (memcmp(addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

typedef enum {
    EXAMPLE_ESPNOW_SEND_CB,
    EXAMPLE_ESPNOW_RECV_CB,
} example_espnow_event_id_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t status;
} example_espnow_event_send_cb_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} example_espnow_event_recv_cb_t;

typedef union {
    example_espnow_event_send_cb_t send_cb;
    example_espnow_event_recv_cb_t recv_cb;
} example_espnow_event_info_t;

/* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
typedef struct {
    example_espnow_event_id_t id;
    example_espnow_event_info_t info;
} example_espnow_event_t;

enum {
    EXAMPLE_ESPNOW_DATA_BROADCAST,
    EXAMPLE_ESPNOW_DATA_UNICAST,
    EXAMPLE_ESPNOW_DATA_MAX,
};

/* User defined field of ESPNOW data in this example. */
typedef struct {
    uint8_t type;                         //Broadcast or unicast ESPNOW data.
    uint8_t state;                        //Indicate that if has received broadcast ESPNOW data or not.
    uint16_t seq_num;                     //Sequence number of ESPNOW data.
    uint16_t crc;                         //CRC16 value of ESPNOW data.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast ESPNOW data.
    uint8_t payload[0];                   //Real payload of ESPNOW data.
} __attribute__((packed)) example_espnow_data_t;

/* Parameters of sending ESPNOW data. */
typedef struct {
    bool unicast;                         //Send unicast ESPNOW data.
    bool broadcast;                       //Send broadcast ESPNOW data.
    uint8_t state;                        //Indicate that if has received broadcast ESPNOW data or not.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast ESPNOW data.
    uint16_t count;                       //Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                       //Delay between sending two ESPNOW data, unit: ms.
    int len;                              //Length of ESPNOW data to be sent, unit: byte.
    uint8_t *buffer;                      //Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
} example_espnow_send_param_t;















bool addPeer(const uint8_t *peerMac, uint8_t channel, const uint8_t *key = nullptr, size_t keyLength = 0);
bool removePeer(const uint8_t *peerMac);
void listPeers();

bool setChannel(uint8_t channel);
uint8_t getChannel();

bool sendData(const uint8_t *peerMac, const uint8_t *data, size_t dataLength);
bool broadcastData(const uint8_t *data, size_t dataLength);

bool hasReceivedData();
ReceivedPacket* popReceivedData();
void printPacketInfo(const ReceivedPacket *packet);


void onDataReceived(const uint8_t *macAddr, const uint8_t *data, int dataLength);

void init_espnow_code();


#endif