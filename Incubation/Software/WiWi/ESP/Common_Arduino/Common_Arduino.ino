/// @file    Blink.ino
/// @brief   Blink the first LED of an LED strip
/// @example Blink.ino

#include <FastLED.h>
#include <Wire.h>
#include "WiFi.h"


/****************  LED STUFF ********************/

// How many leds in your strip?
#define NUM_LEDS 1

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI
#define DATA_PIN 4
#define CLOCK_PIN 13

// Define the array of leds
CRGB leds[NUM_LEDS];

/*************** ESP WIWI Board *******************/

#define ID0_PIN 36
#define ID1_PIN 37
#define ESPWIWI_RST 35 // RST pin when management
#define DCTCXO_ADDR 0x60 // arduino uses 7bit address 
#define DCTCXO_SCL 2
#define DCTCXO_SDA 1

char print_buffer[128];
byte led_pin = 0;
byte esp_id = 0xff; 

#define ESPRX_ID 0x1
#define ESPTX_ID 0x0
#define ESPMGMT_ID 0x2



/********** RF Switch path controls

RX Attenuator -> Controlled by ESPTX GPIO1 : default is low (max attenuation) , set high for zero attenuation
SW1 , ESPTX select -> Controlled by ESPTX GPIO7 : high for RF1 path (TX own antenna), low for RF2 path (WiWi path)
SW2 , ESPTX own antenna select -> Controlled by ESPTX GPIO8 / GPIO2: low/low for off , high/low for RF1 (SMA) , low/high for RF2 (Chip antenna on board)
SW3 , WiWi antenna select -> Controlled by ESPTX GPIO4 / GPIO5: low/low for off, high/low for RF1 (SMA) , low/high for RF2 (chip antenna on board)
SW4 , TX to WiWi or RX to WiWi -> Controlled by ESPTX GPIO6: high for output1 (TX), low for output2 (RX)

*********/


/****** WiWi board control functions ********/

void esptx_disconnect_txrx_wiwi() {
  sprintf(print_buffer, "ESP TX disable antenna connections for tx and rx modules\n");
  Serial.write(print_buffer);
  // turn on attenuators 
  digitalWrite(1, LOW);
  // select TX path
  digitalWrite(7, HIGH);
  // disconnect TX antenna
  digitalWrite(8, LOW);
  digitalWrite(2, LOW);

  // disconnect WiWi / RX antenna
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);

  // select RX to WiWi antenna
  digitalWrite(6, LOW);



}

void esptx_set_independent_antenna(bool tx_sma, bool rx_sma) {
  sprintf(print_buffer, "ESP TX set independent antenna, tx_sma=%d, rx_sma=%d\n", tx_sma, rx_sma);
  Serial.write(print_buffer);
  // turn off attenuators 
  digitalWrite(1, HIGH);
  // select TX path
  digitalWrite(7, HIGH);
  // select TX antenna
  if ( tx_sma ) {
    digitalWrite(8, HIGH);
    digitalWrite(2, LOW);
  } else {
    digitalWrite(8, LOW);
    digitalWrite(2, HIGH);
  }
  // select WiWi (in this case RX) antenna
  if ( rx_sma ) {
    digitalWrite(4, HIGH);
    digitalWrite(5, LOW);
  } else {
    digitalWrite(4, LOW);
    digitalWrite(5, HIGH);
  }
  // select RX to WiWi antenna
  digitalWrite(6, LOW);

}




/********************** SETUP *************************/

void esptx_setup()
{
  pinMode(1, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  // default, set TX and RX to their own chip antennas
  esptx_set_independent_antenna(false,false); 

  //esptx_disconnect_txrx_wiwi();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

}



void esprx_setup()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

}

void espmgmt_setup()
{
  pinMode(ESPWIWI_RST, OUTPUT);
  digitalWrite(ESPWIWI_RST, LOW); // hold them in reset
  Wire.begin(DCTCXO_SDA, DCTCXO_SCL);

    // enable DCTCXO , this must succeed, otherwise loop, means dctcxo not installed on the board most likely
  int i2c_status = 0;
  do {
    Wire.beginTransmission(DCTCXO_ADDR);
    Wire.write(0x1); // register address
    Wire.write(1<<2); // Output enable, bit 10
    Wire.write(0x0); // LSB, default to zero
    i2c_status = Wire.endTransmission();
    sprintf(print_buffer, "DCTCXO Enable I2c status: 0x%x\n", i2c_status);
    Serial.write(print_buffer);
    Serial.write("\n");
    delay(500);
  } while (i2c_status != 0);
  // DCTCXO is enabled, let WiWi ESPs out of reset
  digitalWrite(ESPWIWI_RST, HIGH);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}





void setup() { 
  Serial.begin(115200);
  pinMode(36,INPUT);
  pinMode(37,INPUT);
  int i2c_status = 0;



  if ( !digitalRead(36) && !digitalRead(37) ) {
    // both zero , only true for ESP TX
    esp_id = ESPTX_ID;
  } else if ( digitalRead(36) && !digitalRead(37) ) {
    esp_id = ESPRX_ID;
  } else {
    esp_id = ESPMGMT_ID;
  }
   

  if ( esp_id == ESPMGMT_ID ) {
    Serial.write("Found management ID!\n");
    led_pin = 4;
    espmgmt_setup();
  } else if ( esp_id == ESPTX_ID ) {
    Serial.write("Found TX ID!\n");
    led_pin = 38;
    esptx_setup();
  } else if ( esp_id == ESPRX_ID ) {
    Serial.write("Found RX ID!\n");
    led_pin = 21;
    esprx_setup();
  }
  

  switch (led_pin) {
    case 4: // esp management
      FastLED.addLeds<NEOPIXEL, 4>(leds, NUM_LEDS);  // GRB ordering is assumed
      break;
    case 21: // esp rx
      FastLED.addLeds<NEOPIXEL, 21>(leds, NUM_LEDS);  // GRB ordering is assumed
      break;
    case 38: // exp tx
      FastLED.addLeds<NEOPIXEL, 38>(leds, NUM_LEDS);  // GRB ordering is assumed
      break;
  }
  FastLED.setBrightness(10); // my eyes 
  

}

/************************ LOOP ******************************/



void do_wifi_scan() {

  if ( esp_id == ESPMGMT_ID ) {
    Serial.write("Mgmt wifi scan start\n");
  } else if ( esp_id == ESPTX_ID ) {
    Serial.write("TX wifi scan start\n");
  } else if ( esp_id == ESPRX_ID ) {
    Serial.write("RX wifi scan start\n");
  }

  int n = WiFi.scanNetworks();
  if ( n == 0 ) {
    Serial.write("No networks found!\n");
  } else {
    sprintf(print_buffer, "%d networks found\n", n);
    Serial.write(print_buffer);
    for ( int i = 0; i < n; ++i) {
      sprintf(print_buffer, "%d: %s ( RSSI=%d )\n", i+1, WiFi.SSID(i), WiFi.RSSI(i));
      Serial.write(print_buffer);
    }
  }

}


char ssid[] = "S&JHome";
char pass[] = "zxcASD123[]{}";


void do_wifi_connect_test() {
  int connect_count = 0;
  Serial.print("Starting wifi connect test!\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi ..");
  while ( (WiFi.status() != WL_CONNECTED) && (connect_count < 40) ) {
    Serial.print('.');
    delay(250);
    connect_count++;
  }
  if ( WiFi.status() != WL_CONNECTED ) {
    Serial.print("Failed to connect to wifi! ");
    Serial.print(WiFi.status());
    Serial.print("\n");
  } else {
    Serial.println(WiFi.localIP());
    WiFi.disconnect();
  }
  

}



int loop_counter = 0;

void exptx_loop() {
  Serial.write("tx loop5\n");
  leds[0] = CRGB::Red;
  
  FastLED.show();
  delay(500);
  // Now turn the LED off, then pause
  leds[0] = CRGB::White;
  
  FastLED.show();
  delay(500);
  if ( (loop_counter % 10) == 0) {
    do_wifi_scan();   
    do_wifi_connect_test();  
  }
}
void exprx_loop() {
  Serial.write("rx loop5\n");

  leds[0] = CRGB::Yellow;
  
  FastLED.show();
  delay(500);
  // Now turn the LED off, then pause
  leds[0] = CRGB::Pink;
  
  FastLED.show();
  delay(500);
  if ( (loop_counter % 10) == 0) {
    do_wifi_scan();  
    do_wifi_connect_test();  
  }

}
void expmgmt_loop() {
  leds[0] = CRGB::Green;
  
  FastLED.show();
  delay(500);
  // Now turn the LED off, then pause
  leds[0] = CRGB::Blue;
  
  FastLED.show();
  delay(500);

  if ( (loop_counter % 10) == 0) {
    do_wifi_scan();   
    do_wifi_connect_test(); 
  }


}


void loop() { 
  // Turn the LED on, then pause
  

  if ( esp_id == ESPMGMT_ID ) {
    expmgmt_loop();
  } else if ( esp_id == ESPTX_ID ) {
    exptx_loop();
  } else if ( esp_id == ESPRX_ID ) {
    exprx_loop();
  }
  loop_counter++;

}


