/*
 * Copyright (c) 2015 by Leopold Sayous <leosayous@gmail.com> and Thomas Trojer <thomas@trojer.net>
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
 * @file DW1000Mac.h
 * Arduino global library (header file) working with the DW1000 library
 * for the Decawave DW1000 UWB transceiver IC. This class has the purpose
 * to generate the mac layer
 * 
 * @todo everything, this class is only a prototype
 */

#define FC_1 0x41
#define FC_1_BLINK 0xC5
#define FC_2 0x8C
#define FC_2_SHORT 0x88

#define PAN_ID_1 0xCA
#define PAN_ID_2 0xDE

#define SHORT_MAC_LEN 9
#define LONG_MAC_LEN 15


#ifndef _DW1000MAC_H_INCLUDED
#define _DW1000MAC_H_INCLUDED

#include <Arduino.h>
#include "DW1000Device.h" 

class DW1000Device;

class DW1000Mac {
public:
	//Constructor and destructor
	DW1000Mac(DW1000Device* parent);
	DW1000Mac();
	~DW1000Mac();
	
	
	//setters
	void setDestinationAddress(byte* destinationAddress);
	void setDestinationAddressShort(byte* shortDestinationAddress);
	void setSourceAddress(byte* sourceAddress);
	void setSourceAddressShort(byte* shortSourceAddress);
	
	
	//for poll message we use just 2 bytes address
	//total=12 bytes
	void generateBlinkFrame(byte frame[], byte sourceAddress[], byte sourceShortAddress[]);
	
	//the short fram usually for Resp, Final, or Report
	//2 bytes for Desination Address and 2 bytes for Source Address
	//total=9 bytes
	void generateShortMACFrame(byte frame[], byte sourceShortAddress[], byte destinationShortAddress[]);
	
	//the long frame for Ranging init
	//8 bytes for Destination Address and 2 bytes for Source Address
	//total of
	void generateLongMACFrame(byte frame[], byte sourceShortAddress[], byte destinationAddress[]);
	
	//in order to decode the frame and save source Address!
	void decodeBlinkFrame(byte frame[], byte address[], byte shortAddress[]);
	void decodeShortMACFrame(byte frame[], byte address[]);
	void decodeLongMACFrame(byte frame[], byte address[]);
	
	void incrementSeqNumber();


private:
	uint8_t _seqNumber = 0;
	void reverseArray(byte to[], byte from[], int16_t size);
	
};


#endif

