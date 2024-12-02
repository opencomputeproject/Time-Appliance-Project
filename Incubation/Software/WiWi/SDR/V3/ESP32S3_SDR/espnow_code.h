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