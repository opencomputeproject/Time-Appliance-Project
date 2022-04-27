

#include <DW1000.h>
#include <DW1000Constants.h>
#include "dw1000_regs.h"
#include "dw1000_ptp.h"
#include "dw1000_gpio_leds.h"
#include <require_cpp11.h>
#include <SPI.h>
#include "TimeDrive_AD9546_RX.h"
#include "wiring_private.h"

/*
extern "C" {
#include <hardware/watchdog.h>
};
*/


#include "DPLL_AD9546.h"





// DPLL Pins 
extern const uint8_t PIN_DPLL_SS;
extern const uint8_t PIN_DPLL_SCK;
extern const uint8_t PIN_DPLL_DAT;
extern const uint8_t PIN_DPLL_RESETB;
extern const uint8_t PIN_DPLL_M5;

const uint8_t PIN_DPLL_SCK = 14; // PA02
const uint8_t PIN_DPLL_DAT = 42; // PA03
const uint8_t PIN_DPLL_SS = 17; // PA04
const uint8_t PIN_DPLL_RESETB = 18; // PA05
const uint8_t PIN_DPLL_M5 = 8; // PA06
extern bool debug_dpll_print;

// Decawave pins 
const uint8_t PIN_DW_IRQ = 22; // PA12
const uint8_t PIN_DW_FORCEON = 38; // PA13
const uint8_t PIN_DW_WAKEUP = 2; //PA14
const uint8_t PIN_DW_RST = 5; // PA15
const uint8_t PIN_DW_MOSI = 11; // PA16 , SERCOM1/3 PAD[0]
const uint8_t PIN_DW_SCK = 13; // PA17 , SERCOM1/3 PAD[1]
const uint8_t PIN_DW_SS = 10;  // PA18
const uint8_t PIN_DW_MISO = 12; // PA19 , SERCOM1/3 PAD[3]

// create a new SPI using sercom1 on SAMD , doesn't seem to work as of 4/25/2022
// https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/creating-a-new-spi
SPIClass mySPI (&sercom1, PIN_DW_MISO, PIN_DW_SCK, PIN_DW_MOSI,
  SPI_PAD_0_SCK_1 , SERCOM_RX_PAD_3);


// Using SoftSPI instead
// https://wiki.seeedstudio.com/Software-SPI/
// SoftSPI mySPI(11, 12, 13);


// Time Drive LED pin
extern const uint8_t PIN_LED_1;
extern const uint8_t PIN_LED_2;

const uint8_t PIN_LED_1 = 30; // PB22
const uint8_t PIN_LED_2 = 31;// PB23





#define LED_BLINK_INTERVAL 500
uint32_t led_counter = 0; // just to make it blink without delays
bool led_val = HIGH;

bool is_gug = false; // detect if is GUG via a strap GPIO. Strap to GND to make a GUG
//#define GUG_DETECT_PIN 5




// the setup function runs once when you press reset or power the board
void setup() {
  SerialUSB.begin(9600);


  bool toggleme = false;

  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_LED_2, OUTPUT);
  digitalWrite(PIN_LED_1, 0);
  digitalWrite(PIN_LED_2, 0);


  //pinMode(GUG_DETECT_PIN, INPUT_PULLUP);  
  //delay(10);

  
  delay(5000); // just adding this delay so I can watch serial log from beginning
  // real use case shouldn't need this , just for debugging / development



  //mySPI.begin();
  //pinPeripheral(PIN_DW_MISO, PIO_SERCOM);
  //pinPeripheral(PIN_DW_MOSI, PIO_SERCOM);
  //pinPeripheral(PIN_DW_SCK, PIO_SERCOM);
  /*
  int i = 0;
  while ( 1 ) {
    SerialUSB.println("Debug writing SPI something!");

    delay(200);
    
    mySPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    mySPI.transfer(i++);
    mySPI.endTransaction();
    
    
    digitalWrite(PIN_DW_SS, HIGH);
    digitalWrite(PIN_DW_MOSI, HIGH);
    digitalWrite(PIN_DW_SCK, HIGH);

    delay(200);
  }
  */

  //is_gug = !digitalRead(GUG_DETECT_PIN); 

  if ( is_gug ) SerialUSB.println("Detected GUG!");
  else SerialUSB.println("Detected Time Stick!");

/*
  if (watchdog_caused_reboot()) {
      printf("Rebooted by Watchdog!\n");
  } else {
      printf("Clean boot\n");
  }
*/
 




  dpll_init(TimeDrive_AD9546_RX_dpll_reg_count,TimeDrive_AD9546_RX_dpll_regs, TimeDrive_AD9546_RX_dpll_vals);
  // do some basic dpll health checks

  /*
  while ( 1 ) {
      
      digitalWrite(PIN_LED_1, 1);
      digitalWrite(PIN_LED_2, 0);
      delay(500);
      digitalWrite(PIN_LED_1, 0);
      digitalWrite(PIN_LED_2, 1);    
      delay(500);
  }
  */ 
  
  decawave_ptp_init();
  deca_setup_gpio(); 
  /* 
  if (is_gug) 
    decawave_led_setmode(DECA_LEDMODE_COUNTUP);
  else
    decawave_led_setmode(DECA_LEDMODE_COUNTDOWN);
  */

  //watchdog_enable(3000, 1); // make it quite long, but still enable it 
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
    SerialUSB.print("DPLL health check fail:0x"); SerialUSB.println(val);
    debug_dpll_print = true; 
    return false;
  }
  debug_dpll_print = true;

  // check I can still access the decawave
  DW1000.getPrintableDeviceIdentifier(msg);
  if ( msg[0] != 'D' && msg[1] != 'E' &&
    msg[2] != 'C' && msg[3] != 'A' ) {
      SerialUSB.print("Decawave Health check fail:"); SerialUSB.println(msg);
      return false;
  }
  return true;
}


void loop() {
  /*
 if ( health_check() ) 
  watchdog_update();
  */
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
