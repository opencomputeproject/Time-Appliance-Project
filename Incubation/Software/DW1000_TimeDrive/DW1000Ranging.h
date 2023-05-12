/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net> and Leopold Sayous <leosayous@gmail.com>
 * Decawave DW1000 library for arduino.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file DW1000Ranging.h
 * Arduino global library (header file) working with the DW1000 library
 * for the Decawave DW1000 UWB transceiver IC.
 *
 * @TODO
 * - remove or debugmode for Serial.print
 * - move strings to flash to reduce ram usage
 * - do not safe duplicate of pin settings
 * - maybe other object structure
 * - use enums instead of preprocessor constants
 */

#include "DW1000.h"
#include "DW1000Time.h"
#include "DW1000Device.h" 
#include "DW1000Mac.h"

// messages used in the ranging protocol
#define POLL 0
#define POLL_ACK 1
#define RANGE 2
#define RANGE_REPORT 3
#define RANGE_FAILED 255
#define BLINK 4
#define RANGING_INIT 5

#define LEN_DATA 90

//Max devices we put in the networkDevices array ! Each DW1000Device is 74 Bytes in SRAM memory for now.
#define MAX_DEVICES 4

//Default Pin for module:
#define DEFAULT_RST_PIN 9
#define DEFAULT_SPI_SS_PIN 10

//Default value
//in ms
#define DEFAULT_RESET_PERIOD 200
//in us
#define DEFAULT_REPLY_DELAY_TIME 7000

//sketch type (anchor or tag)
#define TAG 0
#define ANCHOR 1

//default timer delay
#define DEFAULT_TIMER_DELAY 80

//debug mode
#ifndef DEBUG
#define DEBUG false
#endif


class DW1000RangingClass {
public:
	//variables
	// data buffer
	static byte data[LEN_DATA];
	
	//initialisation
	static void    initCommunication(uint8_t myRST = DEFAULT_RST_PIN, uint8_t mySS = DEFAULT_SPI_SS_PIN, uint8_t myIRQ = 2);
	static void    configureNetwork(uint16_t deviceAddress, uint16_t networkId, const byte mode[]);
	static void    generalStart();
	static void    startAsAnchor(char address[], const byte mode[], const bool randomShortAddress = true);
	static void    startAsTag(char address[], const byte mode[], const bool randomShortAddress = true);
	static boolean addNetworkDevices(DW1000Device* device, boolean shortAddress);
	static boolean addNetworkDevices(DW1000Device* device);
	static void    removeNetworkDevices(int16_t index);
	
	//setters
	static void setReplyTime(uint16_t replyDelayTimeUs);
	static void setResetPeriod(uint32_t resetPeriod);
	
	//getters
	static byte* getCurrentAddress() { return _currentAddress; };
	
	static byte* getCurrentShortAddress() { return _currentShortAddress; };
	
	static uint8_t getNetworkDevicesNumber() { return _networkDevicesNumber; };
	
	//ranging functions
	static int16_t detectMessageType(byte datas[]); // TODO check return type
	static void loop();
	static void useRangeFilter(boolean enabled);
	// Used for the smoothing algorithm (Exponential Moving Average). newValue must be >= 2. Default 15.
	static void setRangeFilterValue(uint16_t newValue);
	
	//Handlers:
	static void attachNewRange(void (* handleNewRange)(void)) { _handleNewRange = handleNewRange; };
	
	static void attachBlinkDevice(void (* handleBlinkDevice)(DW1000Device*)) { _handleBlinkDevice = handleBlinkDevice; };
	
	static void attachNewDevice(void (* handleNewDevice)(DW1000Device*)) { _handleNewDevice = handleNewDevice; };
	
	static void attachInactiveDevice(void (* handleInactiveDevice)(DW1000Device*)) { _handleInactiveDevice = handleInactiveDevice; };
	
	
	
	static DW1000Device* getDistantDevice();
	static DW1000Device* searchDistantDevice(byte shortAddress[]);
	
	//FOR DEBUGGING
	static void visualizeDatas(byte datas[]);


private:
	//other devices in the network
	static DW1000Device _networkDevices[MAX_DEVICES];
	static volatile uint8_t _networkDevicesNumber;
	static int16_t      _lastDistantDevice;
	static byte         _currentAddress[8];
	static byte         _currentShortAddress[2];
	static byte         _lastSentToShortAddress[2];
	static DW1000Mac    _globalMac;
	static int32_t      timer;
	static int16_t      counterForBlink;
	
	//Handlers:
	static void (* _handleNewRange)(void);
	static void (* _handleBlinkDevice)(DW1000Device*);
	static void (* _handleNewDevice)(DW1000Device*);
	static void (* _handleInactiveDevice)(DW1000Device*);
	
	//sketch type (tag or anchor)
	static int16_t          _type; //0 for tag and 1 for anchor
	// TODO check type, maybe enum?
	// message flow state
	static volatile byte    _expectedMsgId;
	// message sent/received state
	static volatile boolean _sentAck;
	static volatile boolean _receivedAck;
	// protocol error state
	static boolean          _protocolFailed;
	// reset line to the chip
	static uint8_t     _RST;
	static uint8_t     _SS;
	// watchdog and reset period
	static uint32_t    _lastActivity;
	static uint32_t    _resetPeriod;
	// reply times (same on both sides for symm. ranging)
	static uint16_t     _replyDelayTimeUS;
	//timer Tick delay
	static uint16_t     _timerDelay;
	// ranging counter (per second)
	static uint16_t     _successRangingCount;
	static uint32_t    _rangingCountPeriod;
	//ranging filter
	static volatile boolean _useRangeFilter;
	static uint16_t         _rangeFilterValue;
	//_bias correction
	static char  _bias_RSL[17]; // TODO remove or use
	//17*2=34 bytes in SRAM
	static int16_t _bias_PRF_16[17]; // TODO remove or use
	//17 bytes in SRAM
	static char  _bias_PRF_64[17]; // TODO remove or use
	
	
	//methods
	static void handleSent();
	static void handleReceived();
	static void noteActivity();
	static void resetInactive();
	
	//global functions:
	static void checkForReset();
	static void checkForInactiveDevices();
	static void copyShortAddress(byte address1[], byte address2[]);
	
	//for ranging protocole (ANCHOR)
	static void transmitInit();
	static void transmit(byte datas[]);
	static void transmit(byte datas[], DW1000Time time);
	static void transmitBlink();
	static void transmitRangingInit(DW1000Device* myDistantDevice);
	static void transmitPollAck(DW1000Device* myDistantDevice);
	static void transmitRangeReport(DW1000Device* myDistantDevice);
	static void transmitRangeFailed(DW1000Device* myDistantDevice);
	static void receiver();
	
	//for ranging protocole (TAG)
	static void transmitPoll(DW1000Device* myDistantDevice);
	static void transmitRange(DW1000Device* myDistantDevice);
	
	//methods for range computation
	static void computeRangeAsymmetric(DW1000Device* myDistantDevice, DW1000Time* myTOF);
	
	static void timerTick();
	
	//Utils
	static float filterValue(float value, float previousValue, uint16_t numberOfElements);
};

extern DW1000RangingClass DW1000Ranging;
