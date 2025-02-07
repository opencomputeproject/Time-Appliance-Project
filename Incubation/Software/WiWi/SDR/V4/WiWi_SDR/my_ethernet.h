#ifndef MY_ETHERNET_H
#define MY_ETHERNET_H

#include <Arduino.h>
#include "WWVB_Arduino.h"
#include <stm32h7xx_hal_eth.h>
#include "menu_cli.h"

/**** 
https://github.com/stm32duino/STM32Ethernet/tree/main
Depends on https://github.com/limbongofficial/STM32_Core-Arduino/tree/master
Doesn't work for me because I'm using Arduino Giga framework
#include <LwIP.h>
#include <STM32Ethernet.h>
*****/


// https://github.com/STMicroelectronics/stm32-lan8742/tree/main
// apparently libmbed.a as part of Arduino Giga has lan8472 source , just need header file
#include "lan8742.h"

void init_ethernet_cli();


#endif