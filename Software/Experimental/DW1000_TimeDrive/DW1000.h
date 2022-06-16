/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
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
 * @file DW1000.h
 * Arduino driver library (header file) for the Decawave DW1000 UWB transceiver IC.
 * 
 * @todo
 * - impl: later:
 * - TXBOFFS in TX_FCTRL for offset buffer transmit
 * - TR in TX_FCTRL for flagging for ranging messages
 * - CANSFCS in SYS_CTRL to cancel frame check suppression
 * - HSRBP in SYS_CTRL to determine in double buffered mode from which buffer to read
 */

#ifndef _DW1000_H_INCLUDED
#define _DW1000_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include "DW1000Constants.h"
#include "DW1000Time.h"
#include "SoftSPI.h"



class DW1000Class {
public:
	/* ##### Init ################################################################ */
	/** 
	Initiates and starts a sessions with one or more DW1000. If rst is not set or value 0xff, a soft resets (i.e. command
	triggered) are used and it is assumed that no reset line is wired.
	 
	@param[in] irq The interrupt line/pin that connects the Arduino.
	@param[in] rst The reset line/pin for hard resets of ICs that connect to the Arduino. Value 0xff means soft reset.
	*/
	static void begin(uint8_t irq, uint8_t rst = 0xff);
  static void begin_newspi(uint8_t irq, uint8_t rst = 0xff, SoftSPI * newSPIarg = 0, bool poll_irq = true);
  static void begin_hardspi(uint8_t irq, uint8_t rst = 0xff, SPIClass * hardSPIarg = 0, bool poll_irq = true);
	
	/** 
	Selects a specific DW1000 chip for communication. In case of a single DW1000 chip in use
	this call only needs to be done once at start up, but is still mandatory. Other than a call
	to `reselect()` this function performs an initial setup of the now-selected chip.

	@param[in] ss The chip select line/pin that connects the to-be-selected chip with the
	Arduino.
	*/
	static void select(uint8_t ss);
	
	/** 
	(Re-)selects a specific DW1000 chip for communication. In case of a single DW1000 chip in use
	this call is not needed; only a call to `select()` has to be performed once at start up. Other 
	than a call to `select()` this function does not perform an initial setup of the (again-)selected 
	chips and assumes it to have a valid configuration loaded.

	@param[in] ss The chip select line/pin that connects the to-be-selected chip with the
	Arduino.
	*/
	static void reselect(uint8_t ss);
	
	/** 
	Tells the driver library that no communication to a DW1000 will be required anymore.
	This basically just frees SPI and the previously used pins.
	*/
	static void end();
	
	/** 
	Enable debounce Clock, used to clock the LED blinking
	*/
	static void enableDebounceClock();

	/**
	Enable led blinking feature
	*/
	static void enableLedBlinking();

	/**
	Set GPIO mode
	*/
	static void setGPIOMode(uint8_t msgp, uint8_t mode);

        /**
        Enable deep sleep mode
        */
        static void deepSleep();

        /**
        Wake-up from deep sleep by toggle chip select pin
        */
        static void spiWakeup();

	/**
	Resets all connected or the currently selected DW1000 chip. A hard reset of all chips
	is preferred, although a soft reset of the currently selected one is executed if no 
	reset pin has been specified (when using `begin(int)`, instead of `begin(int, int)`).
	*/
	static void reset();
	
	/** 
	Resets the currently selected DW1000 chip programmatically (via corresponding commands).
	*/
	static void softReset();
	
	/* ##### Print device id, address, etc. ###################################### */
	/** 
	Generates a String representation of the device identifier of the chip. That usually 
	are the letters "DECA" plus the	version and revision numbers of the chip.

	@param[out] msgBuffer The String buffer to be filled with printable device information.
		Provide 128 bytes, this should be sufficient.
	*/
	static void getPrintableDeviceIdentifier(char msgBuffer[]);
	
	/** 
	Generates a String representation of the extended unique identifier (EUI) of the chip.

	@param[out] msgBuffer The String buffer to be filled with printable device information.
		Provide 128 bytes, this should be sufficient.
	*/
	static void getPrintableExtendedUniqueIdentifier(char msgBuffer[]);
	
	/** 
	Generates a String representation of the short address and network identifier currently
	defined for the respective chip.

	@param[out] msgBuffer The String buffer to be filled with printable device information.
		Provide 128 bytes, this should be sufficient.
	*/
	static void getPrintableNetworkIdAndShortAddress(char msgBuffer[]);
	
	/** 
	Generates a String representation of the main operational settings of the chip. This
	includes data rate, pulse repetition frequency, preamble and channel settings.

	@param[out] msgBuffer The String buffer to be filled with printable device information.
		Provide 128 bytes, this should be sufficient.
	*/
	static void getPrintableDeviceMode(char msgBuffer[]);
	
	/* ##### Device address management, filters ################################## */
	/** 
	(Re-)set the network identifier which the selected chip should be associated with. This
	setting is important for certain MAC address filtering rules.

	@param[in] val An arbitrary numeric network identifier.
	*/
	
	
	static void setNetworkId(uint16_t val);
	
	/** 
	(Re-)set the device address (i.e. short address) for the currently selected chip. This
	setting is important for certain MAC address filtering rules.

	@param[in] val An arbitrary numeric device address.
	*/
	static void setDeviceAddress(uint16_t val);
	// TODO MAC and filters
	
	static void setEUI(char eui[]);
	static void setEUI(byte eui[]);
	
	/* ##### General device configuration ######################################## */
	/** 
	Specifies whether the DW1000 chip should, again, turn on its receiver in case that the
	last reception failed. 

	This setting is enabled as part of `setDefaults()` if the device is
	in idle mode.

	@param[in] val `true` to enable, `false` to disable receiver auto-reenable.
	*/
	static void setReceiverAutoReenable(boolean val);
	
	/** 
	Specifies the interrupt polarity of the DW1000 chip. 

	As part of `setDefaults()` if the device is in idle mode, interrupt polarity is set to 
	active high.

	@param[in] val `true` for active high interrupts, `false` for active low interrupts.
	*/
	static void setInterruptPolarity(boolean val);
	
	/** 
	Specifies whether to suppress any frame check measures while sending or receiving messages.
	If suppressed, no 2-byte checksum is appended to the message before sending and this 
	checksum is also not expected at receiver side. Note that when suppressing frame checks, 
	the error event handler	(attached via `attachReceiveErrorHandler()`) will not be triggered 
	if received data is corrupted.

	Frame checks are enabled as part of `setDefaults()` if the device is in idle mode.

	@param[in] val `true` to suppress frame check on sender and receiver side, `false` otherwise.
	*/
	static void suppressFrameCheck(boolean val);
	
	/** 
	Specifies the data transmission rate of the DW1000 chip. One of the values
	- `TRX_RATE_110KBPS` (i.e. 110 kb/s)
	- `TRX_RATE_850KBPS` (i.e. 850 kb/s)
	- `TRX_RATE_6800KBPS` (i.e. 6.8 Mb/s)
	has to be provided.

	See `setDefaults()` and `enableMode()` for additional information on data rate settings.

	@param[in] rate The data transmission rate, encoded by the above defined constants.
	*/
	static void setDataRate(byte rate);
	
	/** 
	Specifies the pulse repetition frequency (PRF) of data transmissions with the DW1000. Either
	- `TX_PULSE_FREQ_16MHZ` (i.e. 16 MHz)
	- `TX_PULSE_FREQ_64MHZ` (i.e. 64 MHz)
	has to be chosen.

	Note that the 16 MHz setting is more power efficient, while the 64 MHz setting requires more
	power, but also delivers slightly better transmission performance (i.e. on communication range and 
	timestamp accuracy) (see DWM1000 User Manual, section 9.3). 

	See `setDefaults()` and `enableMode()` for additional information on PRF settings.

	@param[in] freq The PRF, encoded by the above defined constants.
	*/
	static void setPulseFrequency(byte freq);
	static byte getPulseFrequency();
	static void setPreambleLength(byte prealen);
	static void setChannel(byte channel);
	static void setPreambleCode(byte preacode);
	static void useSmartPower(boolean smartPower);
	
	/* transmit and receive configuration. */
	static DW1000Time   setDelay(const DW1000Time& delay);
	static void         receivePermanently(boolean val);
	static void         setData(byte data[], uint16_t n);
	static void         setData(const String& data);
	static void         getData(byte data[], uint16_t n);
	static void         getData(String& data);
	static uint16_t     getDataLength();
	static void         getTransmitTimestamp(DW1000Time& time);
	static void         getReceiveTimestamp(DW1000Time& time);
	static void         getSystemTimestamp(DW1000Time& time);
	static void         getTransmitTimestamp(byte data[]);
	static void         getReceiveTimestamp(byte data[]);
	static void         getSystemTimestamp(byte data[]);
	
	/* receive quality information. */
	static float getReceivePower();
	static float getFirstPathPower();
	static float getReceiveQuality();
	
	/* interrupt management. */
	static void interruptOnSent(boolean val);
	static void interruptOnReceived(boolean val);
	static void interruptOnReceiveFailed(boolean val);
	static void interruptOnReceiveTimeout(boolean val);
	static void interruptOnReceiveTimestampAvailable(boolean val);
	static void interruptOnAutomaticAcknowledgeTrigger(boolean val);

	/* Antenna delay calibration */
	static void setAntennaDelay(const uint16_t value);
	static uint16_t getAntennaDelay();

	/* callback handler management. */
	static void attachErrorHandler(void (* handleError)(void)) {
		_handleError = handleError;
	}
	
	static void attachSentHandler(void (* handleSent)(void)) {
		_handleSent = handleSent;
	}
	
	static void attachReceivedHandler(void (* handleReceived)(void)) {
		_handleReceived = handleReceived;
	}
	
	static void attachReceiveFailedHandler(void (* handleReceiveFailed)(void)) {
		_handleReceiveFailed = handleReceiveFailed;
	}
	
	static void attachReceiveTimeoutHandler(void (* handleReceiveTimeout)(void)) {
		_handleReceiveTimeout = handleReceiveTimeout;
	}
	
	static void attachReceiveTimestampAvailableHandler(void (* handleReceiveTimestampAvailable)(void)) {
		_handleReceiveTimestampAvailable = handleReceiveTimestampAvailable;
	}
	
	/* device state management. */
	// idle state
	static void idle();
	
	// general configuration state
	static void newConfiguration();
	static void commitConfiguration();
	
	// reception state
	static void newReceive();
	static void startReceive();
	
	// transmission state
	static void newTransmit();
	static void startTransmit();
	
	/* ##### Operation mode selection ############################################ */
	/** 
	Specifies the mode of operation for the DW1000. Modes of operation are pre-defined
	combinations of data rate, pulse repetition frequency, preamble and channel settings
	that properly go together. If you simply want the chips to work, choosing a mode is 
	preferred over manual configuration.

	The following modes are pre-configured and one of them needs to be chosen:
	- `MODE_LONGDATA_RANGE_LOWPOWER` (basically this is 110 kb/s data rate, 16 MHz PRF and long preambles)
	- `MODE_SHORTDATA_FAST_LOWPOWER` (basically this is 6.8 Mb/s data rate, 16 MHz PRF and short preambles)
	- `MODE_LONGDATA_FAST_LOWPOWER` (basically this is 6.8 Mb/s data rate, 16 MHz PRF and long preambles)
	- `MODE_SHORTDATA_FAST_ACCURACY` (basically this is 6.8 Mb/s data rate, 64 MHz PRF and short preambles)
	- `MODE_LONGDATA_FAST_ACCURACY` (basically this is 6.8 Mb/s data rate, 64 MHz PRF and long preambles)
	- `MODE_LONGDATA_RANGE_ACCURACY` (basically this is 110 kb/s data rate, 64 MHz PRF and long preambles)

	Note that LOWPOWER and ACCURACY refers to the better power efficiency and improved transmission performance
	of 16 MHZ and 64 MHZ PRF respectively (see `setPulseFrequency()`).

	The default setting that is selected by `setDefaults()` is MODE_LONGDATA_RANGE_LOWPOWER.

	@param[in] mode The mode of operation, encoded by the above defined constants.
	*/
	static void enableMode(const byte mode[]);
	
	// use RX/TX specific and general default settings
	static void setDefaults();
	
	/* debug pretty print registers. */
	static void getPrettyBytes(byte cmd, uint16_t offset, char msgBuffer[], uint16_t n);
	static void getPrettyBytes(byte data[], char msgBuffer[], uint16_t n);
	
	//convert from char to 4 bits (hexadecimal)
	static uint8_t nibbleFromChar(char c);
	static void convertToByte(char string[], byte* eui_byte);
	
	// host-initiated reading of temperature and battery voltage
	static void getTempAndVbat(float& temp, float& vbat);
	
	// transmission/reception bit rate
	static constexpr byte TRX_RATE_110KBPS  = 0x00;
	static constexpr byte TRX_RATE_850KBPS  = 0x01;
	static constexpr byte TRX_RATE_6800KBPS = 0x02;
	
	// transmission pulse frequency
	// 0x00 is 4MHZ, but receiver in DW1000 does not support it (!??)
	static constexpr byte TX_PULSE_FREQ_16MHZ = 0x01;
	static constexpr byte TX_PULSE_FREQ_64MHZ = 0x02;
	
	// preamble length (PE + TXPSR bits)
	static constexpr byte TX_PREAMBLE_LEN_64   = 0x01;
	static constexpr byte TX_PREAMBLE_LEN_128  = 0x05;
	static constexpr byte TX_PREAMBLE_LEN_256  = 0x09;
	static constexpr byte TX_PREAMBLE_LEN_512  = 0x0D;
	static constexpr byte TX_PREAMBLE_LEN_1024 = 0x02;
	static constexpr byte TX_PREAMBLE_LEN_1536 = 0x06;
	static constexpr byte TX_PREAMBLE_LEN_2048 = 0x0A;
	static constexpr byte TX_PREAMBLE_LEN_4096 = 0x03;
	
	// PAC size. */
	static constexpr byte PAC_SIZE_8  = 8;
	static constexpr byte PAC_SIZE_16 = 16;
	static constexpr byte PAC_SIZE_32 = 32;
	static constexpr byte PAC_SIZE_64 = 64;
	
	/* channel of operation. */
	static constexpr byte CHANNEL_1 = 1;
	static constexpr byte CHANNEL_2 = 2;
	static constexpr byte CHANNEL_3 = 3;
	static constexpr byte CHANNEL_4 = 4;
	static constexpr byte CHANNEL_5 = 5;
	static constexpr byte CHANNEL_7 = 7;
	
	/* preamble codes. */
	static constexpr byte PREAMBLE_CODE_16MHZ_1  = 1;
	static constexpr byte PREAMBLE_CODE_16MHZ_2  = 2;
	static constexpr byte PREAMBLE_CODE_16MHZ_3  = 3;
	static constexpr byte PREAMBLE_CODE_16MHZ_4  = 4;
	static constexpr byte PREAMBLE_CODE_16MHZ_5  = 5;
	static constexpr byte PREAMBLE_CODE_16MHZ_6  = 6;
	static constexpr byte PREAMBLE_CODE_16MHZ_7  = 7;
	static constexpr byte PREAMBLE_CODE_16MHZ_8  = 8;
	static constexpr byte PREAMBLE_CODE_64MHZ_9  = 9;
	static constexpr byte PREAMBLE_CODE_64MHZ_10 = 10;
	static constexpr byte PREAMBLE_CODE_64MHZ_11 = 11;
	static constexpr byte PREAMBLE_CODE_64MHZ_12 = 12;
	static constexpr byte PREAMBLE_CODE_64MHZ_17 = 17;
	static constexpr byte PREAMBLE_CODE_64MHZ_18 = 18;
	static constexpr byte PREAMBLE_CODE_64MHZ_19 = 19;
	static constexpr byte PREAMBLE_CODE_64MHZ_20 = 20;
	
	/* frame length settings. */
	static constexpr byte FRAME_LENGTH_NORMAL   = 0x00;
	static constexpr byte FRAME_LENGTH_EXTENDED = 0x03;
	
	/* pre-defined modes of operation (3 bytes for data rate, pulse frequency and 
	preamble length). */
	static constexpr byte MODE_LONGDATA_RANGE_LOWPOWER[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_2048};
	static constexpr byte MODE_SHORTDATA_FAST_LOWPOWER[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_128};
	static constexpr byte MODE_LONGDATA_FAST_LOWPOWER[]  = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_1024};
	static constexpr byte MODE_SHORTDATA_FAST_ACCURACY[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_128};
	static constexpr byte MODE_LONGDATA_FAST_ACCURACY[]  = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_1024};
	static constexpr byte MODE_LONGDATA_RANGE_ACCURACY[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_2048};

//private:
	/* chip select, reset and interrupt pins. */
	static uint8_t _ss;
	static uint8_t _rst;
	static uint8_t _irq;
  
  static bool useNewSPI;
  static bool useHardSPI;
  static SoftSPI  * newSPI;
  static SPIClass * hardSPI; 
	
	/* callbacks. */
	static void (* _handleError)(void);
	static void (* _handleSent)(void);
	static void (* _handleReceived)(void);
	static void (* _handleReceiveFailed)(void);
	static void (* _handleReceiveTimeout)(void);
	static void (* _handleReceiveTimestampAvailable)(void);
	
	/* register caches. */
	static byte _syscfg[LEN_SYS_CFG];
	static byte _sysctrl[LEN_SYS_CTRL];
	static byte _sysstatus[LEN_SYS_STATUS];
	static byte _txfctrl[LEN_TX_FCTRL];
	static byte _sysmask[LEN_SYS_MASK];
	static byte _chanctrl[LEN_CHAN_CTRL];
	
	/* device status monitoring */
	static byte _vmeas3v3;
	static byte _tmeas23C;

	/* PAN and short address. */
	static byte _networkAndAddress[LEN_PANADR];
	
	/* internal helper that guide tuning the chip. */
	static boolean    _smartPower;
	static byte       _extendedFrameLength;
	static byte       _preambleCode;
	static byte       _channel;
	static byte       _preambleLength;
	static byte       _pulseFrequency;
	static byte       _dataRate;
	static byte       _pacSize;
	static DW1000Time _antennaDelay;
	static boolean    _antennaCalibrated;
	
	/* internal helper to remember how to properly act. */
	static boolean _permanentReceive;
	static boolean _frameCheck;
	
	// whether RX or TX is active
	static uint8_t _deviceMode;

	// whether debounce clock is active
	static boolean _debounceClockEnabled;

	/* Arduino interrupt handler */
	static void handleInterrupt();
	
	/* Allow MAC frame filtering . */
	// TODO auto-acknowledge
	static void setFrameFilter(boolean val);
	static void setFrameFilterBehaveCoordinator(boolean val);
	static void setFrameFilterAllowBeacon(boolean val);
	//data type is used in the FC_1 0x41
	static void setFrameFilterAllowData(boolean val);
	static void setFrameFilterAllowAcknowledgement(boolean val);
	static void setFrameFilterAllowMAC(boolean val);
	//Reserved is used for the Blink message
	static void setFrameFilterAllowReserved(boolean val);
	
	// note: not sure if going to be implemented for now
	static void setDoubleBuffering(boolean val);
	// TODO is implemented, but needs testing
	static void useExtendedFrameLength(boolean val);
	// TODO is implemented, but needs testing
	static void waitForResponse(boolean val);
	
	/* tuning according to mode. */
	static void tune();
	
	/* device status flags */
	static boolean isReceiveTimestampAvailable();
	static boolean isTransmitDone();
	static boolean isReceiveDone();
	static boolean isReceiveFailed();
	static boolean isReceiveTimeout();
	static boolean isClockProblem();
	
	/* interrupt state handling */
	static void clearInterrupts();
	static void clearAllStatus();
	static void clearReceiveStatus();
	static void clearReceiveTimestampAvailableStatus();
	static void clearTransmitStatus();
	
	/* internal helper to read/write system registers. */
	static void readSystemEventStatusRegister();
	static void readSystemConfigurationRegister();
	static void writeSystemConfigurationRegister();
	static void readNetworkIdAndDeviceAddress();
	static void writeNetworkIdAndDeviceAddress();
	static void readSystemEventMaskRegister();
	static void writeSystemEventMaskRegister();
	static void readChannelControlRegister();
	static void writeChannelControlRegister();
	static void readTransmitFrameControlRegister();
	static void writeTransmitFrameControlRegister();
	
	/* clock management. */
	static void enableClock(byte clock);

  /* JULIAN hack , don't use interrupt */
  static void poll_irq();
	
	/* LDE micro-code management. */
	static void manageLDE();
	
	/* timestamp correction. */
	static void correctTimestamp(DW1000Time& timestamp);
	
	/* reading and writing bytes from and to DW1000 module. */
	static void readBytes(byte cmd, uint16_t offset, byte data[], uint16_t n);
	static void readBytesOTP(uint16_t address, byte data[]);
	static void writeByte(byte cmd, uint16_t offset, byte data);
	static void writeBytes(byte cmd, uint16_t offset, byte data[], uint16_t n);
	
	/* writing numeric values to bytes. */
	static void writeValueToBytes(byte data[], int32_t val, uint16_t n);
	
	/* internal helper for bit operations on multi-bytes. */
	static boolean getBit(byte data[], uint16_t n, uint16_t bit);
	static void    setBit(byte data[], uint16_t n, uint16_t bit, boolean val);
	
	/* Register is 6 bit, 7 = write, 6 = sub-adressing, 5-0 = register value
	 * Total header with sub-adressing can be 15 bit. */
	static const byte WRITE      = 0x80; // regular write
	static const byte WRITE_SUB  = 0xC0; // write with sub address
	static const byte READ       = 0x00; // regular read
	static const byte READ_SUB   = 0x40; // read with sub address
	static const byte RW_SUB_EXT = 0x80; // R/W with sub address extension
	
	/* clocks available. */
	static const byte AUTO_CLOCK = 0x00;
	static const byte XTI_CLOCK  = 0x01;
	static const byte PLL_CLOCK  = 0x02;
	
	/* SPI configs. */
	static const SPISettings _fastSPI;
	static const SPISettings _slowSPI;
	static const SPISettings* _currentSPI;
	
	/* range bias tables (500/900 MHz band, 16/64 MHz PRF), -61 to -95 dBm. */
	static const byte BIAS_500_16_ZERO = 10;
	static const byte BIAS_500_64_ZERO = 8;
	static const byte BIAS_900_16_ZERO = 7;
	static const byte BIAS_900_64_ZERO = 7;
	
	// range bias tables (500 MHz in [mm] and 900 MHz in [2mm] - to fit into bytes)
	static constexpr byte BIAS_500_16[] = {198, 187, 179, 163, 143, 127, 109, 84, 59, 31, 0, 36, 65, 84, 97, 106, 110, 112};
	static constexpr byte BIAS_500_64[] = {110, 105, 100, 93, 82, 69, 51, 27, 0, 21, 35, 42, 49, 62, 71, 76, 81, 86};
	static constexpr byte BIAS_900_16[] = {137, 122, 105, 88, 69, 47, 25, 0, 21, 48, 79, 105, 127, 147, 160, 169, 178, 197};
	static constexpr byte BIAS_900_64[] = {147, 133, 117, 99, 75, 50, 29, 0, 24, 45, 63, 76, 87, 98, 116, 122, 132, 142};
	
};

extern DW1000Class DW1000;

#endif
