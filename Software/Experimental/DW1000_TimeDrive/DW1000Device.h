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
 * @file DW1000Device.h
 * Arduino global library (header file) working with the DW1000 library
 * for the Decawave DW1000 UWB transceiver IC.
 * 
 * @todo complete this class
 */


#define INACTIVITY_TIME 1000

#ifndef _DW1000Device_H_INCLUDED
#define _DW1000Device_H_INCLUDED

#include "DW1000Time.h"
#include "DW1000Mac.h"

class DW1000Mac;

class DW1000Device;

class DW1000Device {
public:
	//Constructor and destructor
	DW1000Device();
	DW1000Device(byte address[], byte shortAddress[]);
	DW1000Device(byte address[], boolean shortOne = false);
	~DW1000Device();
	
	//setters:
	void setReplyTime(uint16_t replyDelayTimeUs);
	void setAddress(char address[]);
	void setAddress(byte* address);
	void setShortAddress(byte address[]);
	
	void setRange(float range);
	void setRXPower(float power);
	void setFPPower(float power);
	void setQuality(float quality);
	
	void setReplyDelayTime(uint16_t time) { _replyDelayTimeUS = time; }
	
	void setIndex(int8_t index) { _index = index; }
	
	//getters
	uint16_t getReplyTime() { return _replyDelayTimeUS; }
	
	byte* getByteAddress();
	
	int8_t getIndex() { return _index; }
	
	//String getAddress();
	byte* getByteShortAddress();
	uint16_t getShortAddress();
	//String getShortAddress();
	
	float getRange();
	float getRXPower();
	float getFPPower();
	float getQuality();
	
	boolean isAddressEqual(DW1000Device* device);
	boolean isShortAddressEqual(DW1000Device* device);
	
	//functions which contains the date: (easier to put as public)
	// timestamps to remember
	DW1000Time timePollSent;
	DW1000Time timePollReceived;
	DW1000Time timePollAckSent;
	DW1000Time timePollAckReceived;
	DW1000Time timeRangeSent;
	DW1000Time timeRangeReceived;
	
	void    noteActivity();
	boolean isInactive();


private:
	//device ID
	byte         _ownAddress[8];
	byte         _shortAddress[2];
	int32_t      _activity;
	uint16_t     _replyDelayTimeUS;
	int8_t       _index; // not used
	
	int16_t _range;
	int16_t _RXPower;
	int16_t _FPPower;
	int16_t _quality;
	
	void randomShortAddress();
	
};


#endif
