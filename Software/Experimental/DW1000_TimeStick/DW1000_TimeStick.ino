

#include <DW1000.h>
#include <DW1000Constants.h>
#include "dw1000_regs.h"
#include "dw1000_ptp.h"
#include "dw1000_gpio_leds.h"
#include <require_cpp11.h>
#include <SPI.h>

#include "UWB_AD9546_TX_ver2.h"
#include "UWB_AD9546_TX.h"
#include "UWB_AD9546_RX_ver2j.h"

extern "C" {
#include <hardware/watchdog.h>
};


#include "DPLL_AD9546.h"





// DPLL Pins 
extern const uint8_t PIN_DPLL_SS;
extern const uint8_t PIN_DPLL_SCK;
extern const uint8_t PIN_DPLL_DAT;
extern const uint8_t PIN_DPLL_RESETB;
const uint8_t PIN_DPLL_SS = 15;
const uint8_t PIN_DPLL_DAT = 14;
const uint8_t PIN_DPLL_SCK = 13;
const uint8_t PIN_DPLL_RESETB = 12;
extern bool debug_dpll_print;

// Decawave pins 
const uint8_t PIN_RST = 20;
const uint8_t PIN_IRQ = 21;
const uint8_t PIN_SS = 17; 








#define LED_BLINK_INTERVAL 500
uint32_t led_counter = 0; // just to make it blink without delays
bool led_val = HIGH;

bool is_gug = false; // detect if is GUG via a strap GPIO. Strap to GND to make a GUG
#define GUG_DETECT_PIN 5



// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize digital pin LED_BUILTIN as an output.

  pinMode(GUG_DETECT_PIN, INPUT_PULLUP);  
  delay(10);

  
  delay(5000); // just adding this delay so I can watch serial log from beginning
  // real use case shouldn't need this , just for debugging / development

  is_gug = !digitalRead(GUG_DETECT_PIN); 

  if ( is_gug ) Serial.println("Detected GUG!");
  else Serial.println("Detected Time Stick!");

  if (watchdog_caused_reboot()) {
      printf("Rebooted by Watchdog!\n");
  } else {
      printf("Clean boot\n");
  }
  if ( is_gug ) 
    dpll_init(UWB_AD9546_TX_dpll_reg_count,UWB_AD9546_TX_dpll_regs, UWB_AD9546_TX_dpll_vals);
  else
    dpll_init(UWB_AD9546_RX_ver2j_dpll_reg_count,UWB_AD9546_RX_ver2j_dpll_regs, UWB_AD9546_RX_ver2j_dpll_vals);
  decawave_ptp_init();
  deca_setup_gpio();  
  if (is_gug) 
    decawave_led_setmode(DECA_LEDMODE_COUNTUP);
  else
    decawave_led_setmode(DECA_LEDMODE_COUNTDOWN);

  watchdog_enable(3000, 1); // make it quite long, but still enable it 
}




int64_t picosecond_offset = 0; // absolute offset
double frequency_ratio = 0; // ratio of (remote frequency / local frequency) 
bool update_dpll = false;  // PTP will return offset and frequency to adjust, 
// other algorithm (servo) determines whether to do a phase jump or frequency adjust
uint64_t time_last_updated_dpll = 0; // simple millis, estimate 1 second

bool health_check() {
  return true;
  char msg[128];
  // check I can still access the DPLL
  debug_dpll_print = false;
  byte val; 
  val = dpll_read_register(DEVICE_CODE_0);
  if ( val != 0x21 ){
    Serial.print("DPLL health check fail:0x"); Serial.println(val);
    debug_dpll_print = true; 
    return false;
  }
  debug_dpll_print = true;

  // check I can still access the decawave
  DW1000.getPrintableDeviceIdentifier(msg);
  if ( msg[0] != 'D' && msg[1] != 'E' &&
    msg[2] != 'C' && msg[3] != 'A' ) {
      Serial.print("Decawave Health check fail:"); Serial.println(msg);
      return false;
  }
  return true;
}


void loop() {
 if ( health_check() ) 
  watchdog_update();
 // sanity arduino LED blink 
 if ( (millis() - led_counter) >= LED_BLINK_INTERVAL ) {
    led_counter = millis();    
    digitalWrite(LED_BUILTIN, led_val);
    led_val = !led_val;    
  }
  
  decawave_led_loop();
  update_dpll = false; 
  TopLevelFSM();

  if ( update_dpll && ((millis() - time_last_updated_dpll) > 3000) ) {
    if ( dpll_adjust_error() ) {
      time_last_updated_dpll = millis();
    }
    
  }
  
}
