#ifndef ESP_32_H
#define ESP_32_H


#include <Arduino.h>
#include "WWVB_Arduino.h"
#include <stm32h7xx_hal_spi.h>
#include "menu_cli.h"
#include <stm32h7xx_hal_uart.h>


// Define a structure to hold the parsed SSID information
typedef struct {
    int number;
    char ssid[32];
    int rssi;
    int channel;
    char encryption[16];
} WiFiNetwork;


int parse_wifi_networks(const char *rx_buffer, WiFiNetwork *networks, int max_networks);
int scan_wifi_networks(WiFiNetwork *networks, int max_networks);

void init_esp32();

/************ Top level init and CLI ***********/

void init_esp32_cli();



#endif