



#include "DW1000.h"
#include "DW1000Constants.h"
#include "dw1000_regs.h"
#include "dw1000_ptp.h"
#include "dw1000_gpio_leds.h"
#include "require_cpp11.h"
#include <SPI.h>
#include "TimeDrive_AD9546_RX.h"
#include "wiring_private.h"
#include "SoftSPI.h"

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


// Sorta DPLL, microcontroller 1PPS Input
extern const uint8_t PIN_1PPS_UC_IN; // PB10
const uint8_t PIN_1PPS_UC_IN = 23; // PB10




// Decawave pins 
// tested these pins on a board without decawave, looks correct
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
//SPIClass mySPI (&sercom1, PIN_DW_MISO, PIN_DW_SCK, PIN_DW_MOSI,
//  SPI_PAD_0_SCK_1 , SERCOM_RX_PAD_3);

SPIClass myHardSPI (&sercom1, PIN_DW_MISO, PIN_DW_SCK, PIN_DW_MOSI, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_3);


// Using SoftSPI instead
// https://wiki.seeedstudio.com/Software-SPI/
SoftSPI mySPI(PIN_DW_MOSI, PIN_DW_MISO, PIN_DW_SCK);


// Time Drive LED pin
extern const uint8_t PIN_LED_1;
extern const uint8_t PIN_LED_2;
extern const uint8_t PIN_LED_3;
extern const uint8_t PIN_LED_4;

const uint8_t PIN_LED_1 = 30; // PB22
const uint8_t PIN_LED_2 = 31;// PB23
const uint8_t PIN_LED_3 = 19; //PB02
const uint8_t PIN_LED_4 = 25; //PB03

// bit define the LED
#define UC_LED_OFF 0
#define UC_LED_GREEN 0x1
#define UC_LED_RED 0x4





#define LED_BLINK_INTERVAL 500
uint32_t led_counter = 0; // just to make it blink without delays
bool led_val = HIGH;

bool is_gug = 1; // detect if is GUG via a strap GPIO. Strap to GND to make a GUG
#define DEBUG_BASIC_TX_RX 0
//#define GUG_DETECT_PIN 5

uint64_t health_check_time = 0;





int64_t picosecond_offset = 0; // absolute offset
double frequency_ratio = 0; // ratio of (remote frequency / local frequency) 
bool update_dpll = false;  // PTP will return offset and frequency to adjust, 
// other algorithm (servo) determines whether to do a phase jump or frequency adjust
uint64_t time_last_updated_dpll = 0; // simple millis, estimate 1 second



void set_uc_led(int val) {
  digitalWrite(PIN_LED_1, val & 0x1);
  digitalWrite(PIN_LED_2, (val & 0x2) > 0);
  digitalWrite(PIN_LED_3, (val & 0x4) > 0);
  digitalWrite(PIN_LED_4, (val & 0x8) > 0);
}






// the setup function runs once when you press reset or power the board
void setup() {
  char past_pin_val = 0;
  SerialUSB.begin(9600);


  bool toggleme = false;
  
  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_LED_2, OUTPUT);
  pinMode(PIN_LED_3, OUTPUT);
  pinMode(PIN_LED_4, OUTPUT);

  delay(5000); // just adding this delay so I can watch serial log from beginning
  // real use case shouldn't need this , just for debugging / development

  myHardSPI.begin();
  pinPeripheral(PIN_DW_MISO, PIO_SERCOM);
  pinPeripheral(PIN_DW_SCK, PIO_SERCOM);
  pinPeripheral(PIN_DW_MOSI, PIO_SERCOM);
  

  // SoftSPI interface!
  //mySPI.begin();
  //mySPI.setClockDivider(SPI_CLOCK_DIV128);

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

  if ( is_gug ) {
    SerialUSB.println("Detected GUG!");
    set_uc_led(UC_LED_GREEN);
  }
  else {
    SerialUSB.println("Detected Time Stick!");
    set_uc_led(UC_LED_RED);
  }

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


  if ( is_gug ) { // HACK FOR MY BOARD, I HAVE TWO BOARDS AS OF 6-20-2022, ONLY 1 HAS FULL WORKING PPS TO UC WHICH IS GUG, DISABLE THIS CHECK FOR TimeDrive side
    pinMode(PIN_1PPS_UC_IN, INPUT);
    past_pin_val = digitalRead(PIN_1PPS_UC_IN);
    SerialUSB.println("Waiting for 1PPS pin to toggle on microcontroller");
    led_counter = millis();
    bool ucled_isgreen = false;
    while ( past_pin_val == digitalRead(PIN_1PPS_UC_IN) ) {
      delay(1);
      // add a visible indication this is the current state
      if ( (millis() - led_counter) > LED_BLINK_INTERVAL ) {
        if ( ucled_isgreen ) {
          set_uc_led(UC_LED_RED);
          ucled_isgreen = false;
        } else {
          set_uc_led(UC_LED_GREEN);
          ucled_isgreen = true;
        }
        led_counter = millis();
      }
    }
  }
  
  
  
  
  
  decawave_ptp_init();
  deca_setup_gpio(); 


  
  
  if (is_gug) {
    decawave_led_setmode(DECA_LEDMODE_COUNTUP);
    SerialUSB.println("Decawave LED mode countup!");
  }
  else {
    decawave_led_setmode(DECA_LEDMODE_COUNTDOWN);
    SerialUSB.println("Decawave LED mode countdown!");
  }

  /*
  if ( digitalRead(PIN_DW_IRQ) ) { 
    // IRQ is asserted, force handle it
    SerialUSB.println("End of Decawave GPIO / LED init, IRQ is asserted, force handle it");
    DW1000Class::handleInterrupt();
    DW1000Class::readSystemEventMaskRegister();
    SerialUSB.print("System event mask:0x"); 
    SerialUSB.print(DW1000Class::_sysmask[3], HEX);
    SerialUSB.print(",0x");
    SerialUSB.print(DW1000Class::_sysmask[2], HEX);
    SerialUSB.print(",0x");
    SerialUSB.print(DW1000Class::_sysmask[1], HEX);
    SerialUSB.print(",0x");
    SerialUSB.print(DW1000Class::_sysmask[0], HEX);
    SerialUSB.println("");
    
    SerialUSB.print("Decawave IRQ after force handle:"); SerialUSB.println(digitalRead(PIN_DW_IRQ));
  }
  */
  

  //watchdog_enable(3000, 1); // make it quite long, but still enable it

  if ( is_gug ) {
    set_uc_led(UC_LED_GREEN);
  }
  else {
    set_uc_led(UC_LED_RED);
  }

  /* hack code, just adjust the phase of the 1pps over and over, for measuring on oscilloscope
  while ( 1 ) {
    SerialUSB.println("========Debug shifting phase 10ms=========");
    dpll_adjust_phase_picoseconds(50000000000);
    delay(5000);
  }
  */

  time_last_updated_dpll = millis(); // only consider from here
}


int health_check_reset_deca_counter = 0;

bool health_check() {
  byte val; 
  char msg[128];
  // check I can still access the DPLL
  debug_dpll_print = false;
  health_check_time = millis();
  
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

  if ( !is_gug ) {
    
    if ((millis() - time_last_updated_dpll) > 30000)  {
      // haven't gotten enough packets to update the DPLL in a while
      // maybe decawave got stuck
      if ( health_check_reset_deca_counter < 4 ) {

        SerialUSB.println("Been over 10 seconds since updated DPLL, try to line up with a GUG, try adjusting the phase");
        dpll_adjust_phase_picoseconds(-100000000000);
        delay(2000); // give DPLL some time to adjust
        

        health_check_reset_deca_counter++;
      } else {   
        
        SerialUSB.println("Shifting phase hasn't helped, try resetting Decawave again");      
        
        decawave_ptp_init();
        deca_setup_gpio(); 
        
        if (is_gug) {
          decawave_led_setmode(DECA_LEDMODE_COUNTUP);
          SerialUSB.println("Decawave LED mode countup!");
        }
        else {
          decawave_led_setmode(DECA_LEDMODE_COUNTDOWN);
          SerialUSB.println("Decawave LED mode countdown!");
        }
        health_check_reset_deca_counter = 0;
      }
      time_last_updated_dpll = millis();
    }
  }
  return true;
}


bool debug_dw_irq_val = 0;

void loop() {
  if ( SerialUSB.available() > 0 ) {
    // user input, for debugging

    // allow millisecond input
    int64_t val = (int64_t) SerialUSB.parseInt();
    SerialUSB.println("Handling user input, manual phase shift!");
  
    dpll_adjust_phase_picoseconds( val * ((int64_t)1000) * ((int64_t)1000) * ((int64_t)1000));
    delay(2000); // give DPLL some time to adjust
  }  
  if ( (millis() - health_check_time) > 500 ) {
    health_check();
  }
 // sanity slow loop
  if ( (millis() - led_counter) >= 1 ) {
    if ( digitalRead(PIN_DW_IRQ) != debug_dw_irq_val ) {
      //SerialUSB.print("DW IRQ changed! Now: "); SerialUSB.println(digitalRead(PIN_DW_IRQ));
      debug_dw_irq_val = digitalRead(PIN_DW_IRQ);
    } 
  }  
  decawave_led_loop();
  update_dpll = false;

#if DEBUG_BASIC_TX_RX
  if ( is_gug ) {
    BasicSender();
  } else {
    BasicReceiver();
  }
#else  
  TopLevelFSM();
#endif
  
  if ( update_dpll && ((millis() - time_last_updated_dpll) > 2000) ) {
    if ( dpll_adjust_error() ) {      
      delay(500);
      // Maybe Decawave isn't happy after this, re-init decawave after DPLL adjust     
      decawave_ptp_init();
      deca_setup_gpio(); 
      time_last_updated_dpll = millis();
    }    
  }  
}
