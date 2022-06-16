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
 * @file DW1000Device.cpp
 * Arduino global library (source file) working with the DW1000 library
 * for the Decawave DW1000 UWB transceiver IC.
 * 
 * @todo complete this class
 */

#include "DW1000Device.h"
#include "DW1000.h"


//Constructor and destructor
DW1000Device::DW1000Device() {
	randomShortAddress();
}

DW1000Device::DW1000Device(byte deviceAddress[], boolean shortOne) {
	if(!shortOne) {
		//we have a 8 bytes address
		setAddress(deviceAddress);
		randomShortAddress();
	}
	else {
		//we have a short address (2 bytes)
		setShortAddress(deviceAddress);
	}
}

DW1000Device::DW1000Device(byte deviceAddress[], byte shortAddress[]) {
	//we have a 8 bytes address
	setAddress(deviceAddress);
	//we set the 2 bytes address
	setShortAddress(shortAddress);
}

DW1000Device::~DW1000Device() {
}

//setters:
void DW1000Device::setReplyTime(uint16_t replyDelayTimeUs) { _replyDelayTimeUS = replyDelayTimeUs; }

void DW1000Device::setAddress(char deviceAddress[]) { DW1000.convertToByte(deviceAddress, _ownAddress); }

void DW1000Device::setAddress(byte* deviceAddress) {
	memcpy(_ownAddress, deviceAddress, 8);
}

void DW1000Device::setShortAddress(byte deviceAddress[]) {
	memcpy(_shortAddress, deviceAddress, 2);
}


void DW1000Device::setRange(float range) { _range = round(range*100); }

void DW1000Device::setRXPower(float RXPower) { _RXPower = round(RXPower*100); }

void DW1000Device::setFPPower(float FPPower) { _FPPower = round(FPPower*100); }

void DW1000Device::setQuality(float quality) { _quality = round(quality*100); }


byte* DW1000Device::getByteAddress() {
	return _ownAddress;
}

/*
String DW1000Device::getAddress(){
    char string[25];
    sprintf(string, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
            _ownAddress[0], _ownAddress[1], _ownAddress[2], _ownAddress[3], _ownAddress[4], _ownAddress[5], _ownAddress[6], _ownAddress[7]);
    return String(string);
}*/

byte* DW1000Device::getByteShortAddress() {
	return _shortAddress;
}

/*
String DW1000Device::getShortAddress(){
    char string[6];
    sprintf(string, "%02X:%02X",
            _shortAddress[0], _shortAddress[1]);
    return String(string);
}
*/

uint16_t DW1000Device::getShortAddress() {
	return _shortAddress[1]*256+_shortAddress[0];
}


boolean DW1000Device::isAddressEqual(DW1000Device* device) {
	return memcmp(this->getByteAddress(), device->getByteAddress(), 8) == 0;
}

boolean DW1000Device::isShortAddressEqual(DW1000Device* device) {
	return memcmp(this->getByteShortAddress(), device->getByteShortAddress(), 2) == 0;
}


float DW1000Device::getRange() { return float(_range)/100.0f; }

float DW1000Device::getRXPower() { return float(_RXPower)/100.0f; }

float DW1000Device::getFPPower() { return float(_FPPower)/100.0f; }

float DW1000Device::getQuality() { return float(_quality)/100.0f; }


void DW1000Device::randomShortAddress() {
	_shortAddress[0] = random(0, 256);
	_shortAddress[1] = random(0, 256);
}

void DW1000Device::noteActivity() {
	_activity = millis();
}


boolean DW1000Device::isInactive() {
	//One second of inactivity
	if(millis()-_activity > INACTIVITY_TIME) {
		_activity = millis();
		return true;
	}
	return false;
}
