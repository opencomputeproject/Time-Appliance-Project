#include "DPLL_AD9546.h"
#include <Arduino.h>



// DPLL Pins 
extern const uint8_t PIN_DPLL_SS;
extern const uint8_t PIN_DPLL_SCK;
extern const uint8_t PIN_DPLL_DAT;
extern const uint8_t PIN_DPLL_RESETB;
extern const uint8_t PIN_DPLL_M5;

extern const uint8_t PIN_LED_1;
extern const uint8_t PIN_LED_2;

extern const PROGMEM uint16_t dpll_regs[];
extern const PROGMEM uint8_t dpll_vals[];


bool debug_dpll_print = true;

/* 3-pin SPI mode by default
 *  SCK is low by default
 *  CSB is active low
 *  Data is transferred out on rising edge
 *  Data is transferred in on falling edge
 */


void dpll_stop() { digitalWrite(PIN_DPLL_SS,1); }
void dpll_start() { digitalWrite(PIN_DPLL_SS,0);  }





void convert_to_40bit(int64_t value, byte data[5]) {
  memset(data, 0, 5);
  if ( value < 0 ) {
    // need to do two's complement
    int64_t unsignedval = ( value * -1 ); // get the value
    unsignedval &= 0x7fffffffff; // look at the signed 40-bit value only
    unsignedval = (~unsignedval) + 1; // convert to two's complement
    memcpy(data, &unsignedval, 5); // copy the 5 bytes   
  } else {
    // same value, just copy it over
    memcpy(data, &value, 5);
  }
}



void dpll_write_register(int addr, byte data) {
  // this is more straight forward, basically shift out addr and data on rising edges
  // SCK is low idle but make sure it is
  dpll_start();
  pinMode(PIN_DPLL_DAT, OUTPUT);
  digitalWrite(PIN_DPLL_SCK, 0);

  for ( int i = 15; i >= 0; i-- ) {
    digitalWrite(PIN_DPLL_DAT, (bool)((addr & (1 << i)) >> i ) ); 
    delayMicroseconds(10);
    digitalWrite(PIN_DPLL_SCK, 1);
    delayMicroseconds(10);
    digitalWrite(PIN_DPLL_SCK, 0);
  }

  for ( int i = 7; i >= 0; i-- ) {
    digitalWrite(PIN_DPLL_DAT, (bool)((data & (1 << i)) >> i ) ); 
    delayMicroseconds(10);
    digitalWrite(PIN_DPLL_SCK, 1);
    delayMicroseconds(10);
    digitalWrite(PIN_DPLL_SCK, 0);
  }
  delayMicroseconds(10);
  dpll_stop();
  delayMicroseconds(10);
  if ( debug_dpll_print ) {
    SerialUSB.print("dpll_write_register addr 0x"); SerialUSB.print(addr, HEX);
    SerialUSB.print(" data 0x"); SerialUSB.println(data, HEX);
  }
}

byte dpll_read_register(int addr) {

  // EVB does a write to 0xf with 0x1, basically IO update, before doing arbitrary read
  // do the same I guess
  //dpll_write_register(0xf, 0x1);

  
  
  byte to_return = 0;
  dpll_start();
  // bit 15 of register address is R / !W bit, set it high
  addr |= (1<<15);

  SerialUSB.print("DPLL Read register debug 0x"); SerialUSB.println(addr,HEX);

  pinMode(PIN_DPLL_DAT, OUTPUT);
  // Send first eight bytes 
  // data on rising edge
  // SCK is low for idle
  
  to_return = (byte) ((addr & 0xff00) >> 8);
  for ( int i = 7; i >= 0; i-- ) {
    digitalWrite(PIN_DPLL_DAT, (bool)((to_return & (1 << i)) >> i ) ); 
    delayMicroseconds(10);
    digitalWrite(PIN_DPLL_SCK, 1);
    delayMicroseconds(10);
    digitalWrite(PIN_DPLL_SCK, 0);
  }
  delayMicroseconds(10);

  to_return = (byte) (addr & 0xff);
  // Send next eight bytes
  // Data on rising edge but don't trigger falling edge on last
  for ( int i = 7; i >= 1; i-- ) {
    digitalWrite(PIN_DPLL_DAT, (bool)((to_return & (1 << i)) >> i ) ); 
    delayMicroseconds(10);
    digitalWrite(PIN_DPLL_SCK, 1);
    delayMicroseconds(10);
    digitalWrite(PIN_DPLL_SCK, 0);
  }

  // SCK is low, send out last bit on rising edge
  digitalWrite(PIN_DPLL_DAT, (bool)(to_return & 0x1));
  delayMicroseconds(10);
  digitalWrite(PIN_DPLL_SCK, 1);

  // now need to read input on falling edges
  pinMode(PIN_DPLL_DAT, INPUT);
  to_return = 0;
  delayMicroseconds(10);

  for ( int i = 0; i < 8; i++ ) {
    digitalWrite(PIN_DPLL_SCK, 0);
    to_return <<= 1;
    to_return += (byte) digitalRead(PIN_DPLL_DAT);
    delayMicroseconds(10);
    digitalWrite(PIN_DPLL_SCK, 1);
    delayMicroseconds(10);
  }


  
  dpll_stop();
  delayMicroseconds(10);
  // now put SCK back to idle and end
  digitalWrite(PIN_DPLL_SCK, 0);
   pinMode(PIN_DPLL_DAT, OUTPUT);
   digitalWrite(PIN_DPLL_DAT, 0);

  if ( debug_dpll_print ) {
    SerialUSB.print("DPLL Read register 0x"); SerialUSB.print(addr & 0x7fff, HEX);
    SerialUSB.print(" = 0x"); SerialUSB.println(to_return, HEX);
  }
  return to_return;
}


void pinStr( uint32_t ulPin, unsigned strength) // works like pinMode(), but to set drive strength
{
  // Handle the case the pin isn't usable as PIO
  if ( g_APinDescription[ulPin].ulPinType == PIO_NOT_A_PIN )
  {
    return ;
  }
  if(strength) strength = 1;      // set drive strength to either 0 or 1 copied
  PORT->Group[g_APinDescription[ulPin].ulPort].PINCFG[g_APinDescription[ulPin].ulPin].bit.DRVSTR = strength ;
}


void dpll_init(const PROGMEM uint16_t regcount, const PROGMEM uint16_t regs[],const PROGMEM uint8_t vals[]) {

  pinMode(PIN_DPLL_SS, OUTPUT);
  pinMode(PIN_DPLL_SCK, OUTPUT);
  pinMode(PIN_DPLL_DAT, OUTPUT);
  pinMode(PIN_DPLL_RESETB, OUTPUT);
  pinMode(PIN_DPLL_M5, OUTPUT);

  SerialUSB.print("DPLL SS PIN OUTPUT "); SerialUSB.println(PIN_DPLL_SS);
  SerialUSB.print("DPLL SCK PIN OUTPUT "); SerialUSB.println(PIN_DPLL_SCK);
  SerialUSB.print("DPLL DAT PIN OUTPUT "); SerialUSB.println(PIN_DPLL_DAT);
  SerialUSB.print("DPLL RESET PIN OUTPUT "); SerialUSB.println(PIN_DPLL_RESETB);
  SerialUSB.print("DPLL M5 PIN OUTPUT "); SerialUSB.println(PIN_DPLL_M5);

  
  digitalWrite(PIN_DPLL_SCK, 0);
  digitalWrite(PIN_DPLL_DAT, 0);
  digitalWrite(PIN_DPLL_SS, 1);
  digitalWrite(PIN_DPLL_M5, 0);

  pinStr(PIN_DPLL_RESETB, 1);


  // toggle the reset pin
  digitalWrite(PIN_DPLL_RESETB, 0); // make sure reset is high
  delay(1000);
  digitalWrite(PIN_DPLL_RESETB, 1); // make sure reset is high
  delay(1000);
  


  // do a sanity check, read the ID register
  byte rddata = 0;
  while (1) {
    dpll_write_register(0xF, 0x1); // IO_UPDATE , same as what eval board does
    delay(1);
    rddata = dpll_read_register(0xC);
    SerialUSB.print("DPLL Debug read Register 0x"); SerialUSB.print(0xC, HEX);
    SerialUSB.print(" = 0x"); SerialUSB.println(rddata, HEX);
    if ( rddata == 0x56 ) break;
    SerialUSB.println("WAITING FOR 0x56");
    delay(1000);
  }

  SerialUSB.println("Writing DPLL config from file");
  
  /* Write the config from the file */
  for ( int i = 0; i < regcount; i++ ) {
    dpll_write_register( regs[i], vals[i] );
  }

  /*
  // JULIAN HACK, make sure 0x280f[1] = 1
  SerialUSB.println("JULIAN HACK MAKE SURE 0x280f[1] = 1");
  dpll_write_register( 0x280f, dpll_read_register(0x280f) | 0x2 );

  */
  SerialUSB.println("JULIAN HACK MAKE SURE 0x111a[4] = 0");
  dpll_write_register( 0x111a, dpll_read_register(0x111a) & 0xEF );

  SerialUSB.println("JULIAN HACK MAKE SURE 0x111a[3] = 1");
  dpll_write_register( 0x111a, dpll_read_register(0x111a) | (1<<3) );

  SerialUSB.println("JULIAN HACK MAKE SURE 0x111a[2:0] = 6");
  dpll_write_register( 0x111a, (dpll_read_register(0x111a) & 0x7) + 6 );

  
  
  // Calibrate all VCOs by setting bit 1 to 1 in register 0x2000
  rddata = dpll_read_register(0x2000);
  dpll_write_register(0x2000, rddata | 0x2);
  dpll_io_update();
  delay(2000); // wait 2 seconds for the PLLs to calibrate 
  dpll_write_register(0x2000, rddata);
  dpll_io_update();

  // Synchronize all distribution dividers by setting bit 3 in register 0x2000 to 1
  rddata = dpll_read_register(0x2000);
  dpll_write_register(0x2000, rddata | 0x8);
  dpll_io_update();
  dpll_write_register(0x2000, rddata);
  dpll_io_update();
  
  delay(2000);
  rddata = dpll_read_register(0x3001);
  Serial.print("DPLL Debug, status register 0x3001 = 0x"); Serial.println(rddata, HEX);
  
}



void dpll_io_update() {
  dpll_write_register(0xf, 0x1);
}

// To adjust outputs, adjusting AuxNCO0 frequency and phase
// for frequency, adjust the center frequency, in steps of 2^-40 Hz
// Is there a disadvantage to center frequency vs offset frequency?

// For phase adjust, adjust delta T, which is in picoseconds

// Following servo similar to ptp4l or ts2phc etc.




void dpll_adjust_nco_phase(int64_t picoseconds) {
  // THIS FUNCTION DOES NOT ALLOW LARGE JUMPS, and DPLL tracking takes times
  // better to use distribution offsets
  // for purposes of aligning the PHC inside the DPLL
  // need to rely on looping back the 1PPS to the DPLL itself 
  // register AUXNCO0_PHASEOFFSET , AUXNCO0_PHASEOFFSET_SIZE
  // basically just write picoseconds to the register

  int64_t cur_val = 0;

  for ( int i = 0; i < AUXNCO0_PHASEOFFSET_SIZE; i++ ) {
    cur_val += ((int64_t)dpll_read_register( AUXNCO0_PHASEOFFSET + i )) << (8*i);
  }

  // have current value, add in amount passed straight, it's in the same units

  cur_val += picoseconds;  
  
  for ( int i = 0; i < AUXNCO0_PHASEOFFSET_SIZE; i++ ) {
    dpll_write_register( AUXNCO0_PHASEOFFSET + i , (cur_val >> (8*i)) & 0xff ); 
  }
  dpll_io_update();
}






// adjFreq in units of 2^-40 Hz
void dpll_adjust_frequency(uint64_t adjFreq) {
  // register AUXNCO0_CENTERFREQ , AUXNCO0_CENTERFREQ_SIZE
  for ( int i = 0; i < AUXNCO0_CENTERFREQ_SIZE; i++ ) {
    dpll_write_register(  AUXNCO0_CENTERFREQ + i ,
      (adjFreq >> (8*i)) & 0xff );
  }
  dpll_io_update();
}


// Top level function called outside this DPLL library
// takes the error from the ptp protocol in picoseconds
// and adjusts the DPLL offset only for now 

extern int64_t picosecond_offset; // absolute offset, remote time - local time of a shared event
// positive value means remote time is ahead of local time
// negative value means remote time is behind local time
extern double frequency_ratio; // ratio of (remote frequency / local frequency)
extern bool update_dpll;  // PTP will return offset and frequency to adjust, 


extern void print_int64t(int64_t val);




void dpll_adjust_phase_picoseconds(int64_t picoseconds) {
  
  // The DPLL distribution outputs are phase based
  // I can set the output phase from 0 to 360 degree with finite resolution
  // I need to determine what phase value to write to the DPLL based on offset I find

  // Based on current distribution phase offset, need to adjust it based on picosecond_offset
  // 0 to 360 degrees = 0 to 1 seconds based on the offset being from 1PPS
  // But picosecond_offset can be negative, in -1 to 1 second range in theory
  // a small negative value in picosecond_offset is almost a max value in DPLL phase, close to 360



  
  // first read back the divide ratio
  int64_t divratio = 0;
  int64_t phasevalue = 0;
  double offset_degrees = 0; 
  
  for ( int i = 0; i < 4; i++ ) {
    divratio += ((int64_t)dpll_read_register(0x1112 + i)) << (8*i);    
  }
  for ( int i = 0; i < 4; i++ ) {
    phasevalue += ((int64_t)dpll_read_register(0x1116 + i)) << (8*i);    
  }  
  if ( dpll_read_register(0x111a) & (1<<6) ) {
    phasevalue += ((int64_t)1)<<32; // bit 32 is 0x111a[6]
  }
  SerialUSB.print("Shift by "); print_int64t(picoseconds); SerialUSB.print(" picoseconds"); SerialUSB.println("");
  SerialUSB.print("Divratio:"); print_int64t(divratio); SerialUSB.println("");
  SerialUSB.print("Initial phasevalue:"); print_int64t(phasevalue); SerialUSB.println("");

  // convert picosecond_offset to degrees 
  // period is 1Hz , 360 degrees in 1e12 picoseconds, multiply by that ratio
  offset_degrees = ((double)picoseconds) * 360.0;
  SerialUSB.print("Offset degrees after multiply by 360:"); SerialUSB.println(offset_degrees,20);

  offset_degrees = offset_degrees / (1.0 * 1000.0 * 1000.0 * 1000.0 * 1000.0);
  SerialUSB.print("Offset degrees after divide by 1e12:"); SerialUSB.println(offset_degrees,20);



  if ( offset_degrees < 0.0 ) { // if it's negative, shift it into 0 to 360 range 
    offset_degrees += 360.0;
  }  
  SerialUSB.print("Offset degrees after shifting into 0-360:"); SerialUSB.println(offset_degrees, 5);


  // convert current phase to degrees
  // 180 degrees per divratio units, multiply current value by that ratio 
  double current_degrees = 0;
  current_degrees = (((double) 180.0 ) / ((double) divratio)) * ((double)phasevalue);
  SerialUSB.print("Current degrees:"); SerialUSB.println(current_degrees,5);
  
  // shift offset degrees 180
  // NEED TO ACTUALLY ALIGN THE FALLING EDGE BUT CAN ONLY CONTROL RISING EDGE
  // THE DECAWAVE TIMER ONLY STARTS COUNTING UP WHEN PPS IS LOW
  // BUT AD9546 OUTPUT IS ADJUSTING THE PHASE OF THE RISING EDGE

  //current_degrees += 180.0; 
  //SerialUSB.print("Current degrees after shifting for negative edge:"); SerialUSB.println(current_degrees,5);

  current_degrees += offset_degrees;
  if ( current_degrees >= 360.0 ) { // if it's outside the range, shift it back into range
    current_degrees -= 360.0; 
  }
  SerialUSB.print("Current degrees after offset:"); SerialUSB.println(current_degrees,5);


  
  // convert degrees into register value
  phasevalue = (int64_t) ( ( ((double)divratio) / ((double) 180.0) ) * current_degrees ); 
  
  SerialUSB.print("####################Phasevalue:0x"); SerialUSB.println((long)phasevalue,HEX);

  
  // write lower four bytes to register
  for ( int i = 0; i < 4; i++ ) {
    dpll_write_register( 0x1116 + i, (phasevalue >> (8*i)) & 0xff ); // OUT0B
    dpll_write_register( 0x1504 + i, (phasevalue >> (8*i)) & 0xff ); // OUT1A
  }
  // set bit 33 in register 0x111a
  if ( phasevalue & (0x100000000) ) {
    dpll_write_register( 0x111a, dpll_read_register(0x111a) | (1<<6)); // OUT0B
    dpll_write_register( 0x1508, dpll_read_register(0x1508) | (1<<6)); // OUT1A
  } else {
    dpll_write_register( 0x111a, dpll_read_register(0x111a) & ~(1<<6)); // OUT0B
    dpll_write_register( 0x1508, dpll_read_register(0x1508) & ~(1<<6)); // OUT1A
  }

  dpll_io_update();



}




void dpll_discipline_offset() {

  SerialUSB.print("dpll_discipline_offset "); print_int64t(picosecond_offset); SerialUSB.println("");


  if ( picosecond_offset > MAX_PHASE_ADJ_PS ) {
    picosecond_offset = MAX_PHASE_ADJ_PS;
  } else if ( picosecond_offset < (((int64_t)-1) * MAX_PHASE_ADJ_PS)) {
    picosecond_offset = (((int64_t)-1) * MAX_PHASE_ADJ_PS);
  }

  picosecond_offset = ((int64_t) (( (double)picosecond_offset ) * PHASE_ADJ_KP) );

  //dpll_adjust_phase_picoseconds(picosecond_offset);  
  dpll_adjust_nco_phase(picosecond_offset);
}

void dpll_discipline_freq() {
  // frequency need to read back and do the multiplication
  uint64_t int_center = 0;
  uint64_t temp_val = 0;

  SerialUSB.println("DPLL_DISCIPLINE_FREQ START");

  for ( int i = 0; i < AUXNCO0_CENTERFREQ_SIZE; i++ ) {
    int_center += (((uint64_t) dpll_read_register( AUXNCO0_CENTERFREQ + i )) << (8*i));
  }
  SerialUSB.print("dpll_discipline_freq read frequency value in units of 2^-40 Hz: 0x"); SerialUSB.println( (unsigned long) int_center, HEX);


  // do math in integer land, doubles get wierd I think
  temp_val = INT_PPB(int_center); // get value of 1 PPB

  // if frequency_ratio is > 1 , then remote frequency is faster
  // if frequency_ratio is < 1 , then remote frequency is slower

  // do this comparison as double, but adjust frequency as integer
  if (  FREQ_PPB(frequency_ratio) > MAX_FREQ_ADJ_PPB ) {
    // frequency wants to adjust too much, want to increase it
    SerialUSB.println("FREQ ADJ INCREASE LIMITED");
    int_center -= MAX_FREQ_ADJ_PPB_INT * INT_PPB(int_center);
  } else if (  FREQ_PPB(frequency_ratio) < -1.0*MAX_FREQ_ADJ_PPB ) {
    // frequency wants to adjust too much, want to decrease it
    SerialUSB.println("FREQ ADJ DECREASE LIMITED");
    int_center += MAX_FREQ_ADJ_PPB_INT * INT_PPB(int_center);
  } else {
    // frequency in the range I can Adjust it
    if ( FREQ_PPB(frequency_ratio) > 0 ) {
      SerialUSB.print("FREQ ADJ to requested amount: 0x"); SerialUSB.println( (unsigned long)(INT_PPB(int_center) * ( (uint64_t)( FREQ_PPB(frequency_ratio) * -1.0))) , HEX);
      int_center -= INT_PPB(int_center) * ( (uint64_t) FREQ_PPB(frequency_ratio) );
    } else {
      SerialUSB.print("FREQ ADJ to requested amount: 0x"); SerialUSB.println( (unsigned long)(INT_PPB(int_center) * ( (uint64_t)( FREQ_PPB(frequency_ratio) * -1.0))) , HEX);
      int_center += INT_PPB(int_center) * ( (uint64_t)( FREQ_PPB(frequency_ratio) * -1.0));      
    }
  }
  
  

  SerialUSB.print("######################dpll_discipline_freq change to 0x"); SerialUSB.println((unsigned long)int_center, HEX);

  for ( int i = 0; i < AUXNCO0_CENTERFREQ_SIZE; i++ ) {
    dpll_write_register(  AUXNCO0_CENTERFREQ + i ,
      (int_center >> (8*i)) & 0xff );
  }

  dpll_io_update();

  
}

// DPLL needs time to adjust to large changes in frequency
// check this first 
bool is_dpll_still_adjusting() {
  return false;
  byte data = 0;
  data = dpll_read_register(PLL0_STATUS_0);  
}


//int64_t debug_picoseconds = MILLI_TO_PICO(-1);
bool dpll_adjust_error() {
  return true;
  SerialUSB.print("DPLL Adjust error:");
  print_int64t(picosecond_offset);
  SerialUSB.print(" ");
  SerialUSB.print(frequency_ratio, 12); SerialUSB.println(" ");


  /*
  if ( debug_picoseconds == 0 ) {
    debug_picoseconds = MILLI_TO_PICO(-1);
  } else {
    debug_picoseconds = 0;
  }
  SerialUSB.println("###################HACK##################"); dpll_set_dist_phase(debug_picoseconds); return;
  */

  
  if (is_dpll_still_adjusting() ) {
    return false; // still doing something from previous loop, just back out
  }

  SerialUSB.print("FREQ_PPB:"); SerialUSB.println(FREQ_PPB(frequency_ratio));


  // simple algorithm, adjust frequency, then phase and frequency
  if ( (FREQ_PPM(frequency_ratio) >= 1000) || (FREQ_PPM(frequency_ratio) <= -1000) ) { // sanity check, 1000ppm is probably math error somewhere or protocol error
    return false;
  }
  if ( (FREQ_PPB(frequency_ratio) > 500) || (FREQ_PPB(frequency_ratio) < -500) ) {
    // adjust frequency first
    dpll_discipline_freq();
    dpll_discipline_offset();
    return true;
  } else {
    dpll_discipline_freq();
    dpll_discipline_offset();
    return true;
  }
  /*
  if ( (picosecond_offset > MICRO_TO_PICO(1)) || (picosecond_offset < MICRO_TO_PICO(-1) ) )  {
    dpll_discipline_offset();
    return true;
  } else if ( (FREQ_PPB(frequency_ratio) > 100) || (FREQ_PPB(frequency_ratio) < -100) ) { 
    if ( (FREQ_PPM(frequency_ratio) >= 1000) || (FREQ_PPM(frequency_ratio) <= -1000) ) { // sanity check, 100ppm is probably math error somewhere or protocol error
      return false;
    }
  } 
  */
  return false;
}
