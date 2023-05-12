
#ifndef _DW1000_GPIO_LEDS_H_
#define _DW1000_GPIO_LEDS_H_




#define GPIO_DEBUG 0

#define DECA_SETUP_BLINK_INTERVAL 500
#define DECA_LEDMODE_BLINK 0
#define DECA_LEDMODE_COUNTUP 1
#define DECA_LEDMODE_COUNTDOWN 2


uint8_t deca_led_mode = DECA_LEDMODE_COUNTUP; 
// the loop function runs over and over again forever
uint32_t deca_led_counter = 0;
uint8_t deca_led_shift_counter = 0;


uint64_t dw1000_read_reg(uint16_t reg, uint16_t subaddress, uint8_t nbytes) {

  byte buf[8];
  uint64_t to_return = 0;
#if GPIO_DEBUG
  SerialUSB.print("DW1000 read reg 0x"); SerialUSB.print(reg, HEX);
  SerialUSB.print(" Subaddr 0x"); SerialUSB.print(subaddress, HEX);
#endif
  if ( nbytes > 8 ) return 0;
  DW1000.readBytes(reg, subaddress, buf, nbytes);
  memcpy(&to_return, buf, nbytes);
  
#if GPIO_DEBUG
  SerialUSB.print(" = 0x"); SerialUSB.println((long unsigned int)to_return, HEX);
#endif
  return to_return;
}

void dw1000_write_reg(uint16_t reg, uint16_t subaddress, uint64_t data, uint8_t nbytes) {  
  byte buf[8];

#if GPIO_DEBUG 
  SerialUSB.print("DW1000 write reg 0x"); SerialUSB.print(reg, HEX);
  SerialUSB.print(" Subaddr 0x"); SerialUSB.print(subaddress, HEX);
#endif
  if ( nbytes > 8 ) return;
  memcpy(buf, &data, nbytes);


#if GPIO_DEBUG
  SerialUSB.print(" 0x"); SerialUSB.println((long unsigned int)data, HEX);
#endif

  DW1000.writeBytes(reg, subaddress, buf, nbytes);
}

void dw1000_gpio_set_mode(uint8_t gpioNum, uint8_t mode) {
    uint32_t reg;
#if GPIO_DEBUG
    SerialUSB.print("dw1000_gpio_set_mode gpio "); SerialUSB.print(gpioNum);
    SerialUSB.print(" mode 0x"); SerialUSB.println(mode,HEX);
#endif
    reg = (uint32_t) dw1000_read_reg(GPIO_CTRL_ID, GPIO_MODE_OFFSET , sizeof(uint32_t));
    reg &= ~(0x3UL << (6+gpioNum*2));
#if GPIO_DEBUG
    SerialUSB.print("First reg &= 0x"); SerialUSB.println(reg, HEX);
#endif
    reg |= (mode << (6+gpioNum*2));
#if GPIO_DEBUG
    SerialUSB.print("Second reg |= 0x"); SerialUSB.println(reg, HEX);
#endif
    dw1000_write_reg(GPIO_CTRL_ID, GPIO_MODE_OFFSET , reg, sizeof(uint32_t));
}
uint8_t dw1000_gpio_get_mode(uint8_t gpioNum)
{
    uint32_t reg;
    reg = (uint32_t) dw1000_read_reg(GPIO_CTRL_ID, GPIO_MODE_OFFSET, sizeof(uint32_t));
    reg &= (0x3UL << (6+gpioNum*2));
    reg >>= (6+gpioNum*2);
    return (uint8_t)(reg&0x3);
}

void dw1000_gpio_set_direction(uint8_t gpioNum, uint8_t dir)
{
    uint32_t reg;
    uint8_t buf[GPIO_DIR_LEN];
    uint32_t command;
#if GPIO_DEBUG
    SerialUSB.print("dw1000_gpio_set_direction gpio "); SerialUSB.print(gpioNum);
    SerialUSB.print(" dir "); SerialUSB.println(dir);
#endif
    if (!(gpioNum < 9)) return;
    /* Activate GPIO Clock if not already active */
    reg = dw1000_read_reg(PMSC , PMSC_CTRL0_SUB , sizeof(uint32_t));
    if ((reg&PMSC_CTRL0_GPCE) == 0 || (reg&PMSC_CTRL0_GPRN) == 0) {
        dw1000_write_reg(PMSC, PMSC_CTRL0_SUB,
                         reg|PMSC_CTRL0_GPCE|PMSC_CTRL0_GPRN,
                         sizeof(uint32_t));
    }
    /* See GxM1-8 and GxP1-8 in dw1000_regs.h. Mask | Value */
    if (gpioNum < 4) { // gpio 0-3
        command = (1 << (gpioNum+4)) | (dir << gpioNum);
    } else if (gpioNum<8) { // gpio 4-7
        command = (1 << (gpioNum+8)) | (dir << (gpioNum+8-4));
    } else { // gpio 8
        command = (1 << (20)) | (dir << (16));
    }
    buf[0] = command & 0xff;
    buf[1] = (command >> 8) & 0xff;
    buf[2] = (command >> 16) & 0xff;
#if GPIO_DEBUG
    SerialUSB.print("writeBytes reg 0x"); SerialUSB.print(GPIO_CTRL_ID, HEX);
    SerialUSB.print(" subaddress 0x"); SerialUSB.print(GPIO_DIR_OFFSET, HEX);
    SerialUSB.print(" len "); SerialUSB.print(GPIO_DIR_LEN);
    SerialUSB.print(" command 0x"); SerialUSB.print(command, HEX);
    for ( int i = 0; i < GPIO_DIR_LEN; i++ ) {
      SerialUSB.print(" 0x"); SerialUSB.print(buf[i], HEX);
    }
    SerialUSB.println("");
#endif
    DW1000.writeBytes(GPIO_CTRL_ID, GPIO_DIR_OFFSET, buf, GPIO_DIR_LEN);
}

uint8_t dw1000_gpio_get_direction(uint8_t gpioNum)
{
    uint8_t res;
    uint32_t reg;
    if (!(gpioNum < 9)) return 0;
    reg = dw1000_read_reg(GPIO_CTRL_ID, GPIO_DIR_OFFSET, sizeof(uint32_t));
    if (gpioNum < 4) {
        res = (uint8_t)(0x1 & (reg >> gpioNum));
    } else if (gpioNum<8) {
        res = (uint8_t)(0x1 & (reg >> (gpioNum+8)));
    } else {
        res = (uint8_t)(0x1 & (reg >> (gpioNum+12)));
    }
    return res;
}
void dw1000_gpio_set_value(uint8_t gpioNum, uint8_t value)
{
    uint8_t buf[GPIO_DOUT_LEN];
    uint32_t command;

    if (!(gpioNum < 9)) return;

    /* See GxM1-8 and GxP1-8 in dw1000_regs.h. Mask | Value */
    if (gpioNum < 4) { //gpio 0-3
        command = (1 << (gpioNum+4)) | (value << gpioNum);
    } else if (gpioNum<8) { //gpio 4-7
        command = (1 << (gpioNum+8)) | (value << (gpioNum+8-4));
    } else { // gpio 8
        command = (1 << (20)) | (value << (16));
    }
    buf[0] = command & 0xff;
    buf[1] = (command >> 8) & 0xff;
    buf[2] = (command >> 16) & 0xff;

#if GPIO_DEBUG
    SerialUSB.print("dw1000_gpio_set_value reg 0x"); SerialUSB.print(GPIO_CTRL_ID, HEX);
    SerialUSB.print(" subaddress 0x"); SerialUSB.print(GPIO_DOUT_OFFSET, HEX);
    SerialUSB.print(" len "); SerialUSB.print(GPIO_DOUT_LEN);
    SerialUSB.print(" command 0x"); SerialUSB.print(command, HEX);
    for ( int i = 0; i < GPIO_DOUT_LEN; i++ ) {
      SerialUSB.print(" 0x"); SerialUSB.print(buf[i], HEX);
    }
    SerialUSB.println("");
#endif


    DW1000.writeBytes(GPIO_CTRL_ID, GPIO_DOUT_OFFSET, buf, GPIO_DOUT_LEN);
}
uint32_t dw1000_gpio_get_values()
{
    uint32_t reg;
    reg = (uint32_t) dw1000_read_reg(GPIO_CTRL_ID, GPIO_RAW_OFFSET,
                                     sizeof(uint32_t));
    return (reg&0xFFF);
}
void dw1000_gpio_init_out(int gpioNum, int val)
{
#if GPIO_DEBUG
    SerialUSB.print("dw1000_gpio_init_out gpio "); SerialUSB.print(gpioNum); 
    SerialUSB.print(" val "); SerialUSB.println(val);
#endif
    dw1000_gpio_set_direction(gpioNum, 0);
    dw1000_gpio_set_value(gpioNum, val);
}
void dw1000_gpio_init_in(int gpioNum)
{
#if GPIO_DEBUG
    SerialUSB.print("dw1000_gpio_init_in gpio "); SerialUSB.println(gpioNum); 
#endif
    dw1000_gpio_set_direction(gpioNum, 1);
}
int dw1000_gpio_read(uint8_t gpioNum)
{
  uint32_t reg = dw1000_gpio_get_values();
  if (gpioNum == 7 ) {
    // seems like a chip bug????? Reading raw register, sync input (gpio7) is bit 11
    return (reg&(1<<11)) ? 1:0;
  }
  return (reg&(1<<gpioNum)) ? 1:0;
}
void dw1000_gpio_write(int gpioNum, int val)
{
#if GPIO_DEBUG
    SerialUSB.print("dw1000_gpio_write "); SerialUSB.print(gpioNum);
    SerialUSB.print(" = "); SerialUSB.println(val);
#endif
    dw1000_gpio_set_value(gpioNum, val);
}

void dw1000_phy_external_sync(uint8_t delay, bool enable){

    uint16_t reg = dw1000_read_reg(EXT_SYNC_ID, EC_CTRL_OFFSET, sizeof(uint16_t));
    if (enable) {
        reg &= ~EC_CTRL_WAIT_MASK; //clear timer value, clear OSTRM
        reg |= EC_CTRL_OSTRM;      //External timebase reset mode enable
        reg |= ((((uint16_t) delay) & 0xff) << 3); //set new timer value

    }else {
        reg &= ~(EC_CTRL_WAIT_MASK | EC_CTRL_OSTRM); //clear timer value, clear OSTRM
    }
    dw1000_write_reg(EXT_SYNC_ID, EC_CTRL_OFFSET, reg, sizeof(uint16_t));
}

void debug_toggle_sync_as_output() {
  
  /* Debug toggle the sync pin 
  dw1000_gpio_init_out(7,0);
  dw1000_gpio_set_mode(7, 0x1);
  dw1000_gpio_init_out(7,0);
  while ( 1 ) {
    if ( (millis() - led_counter) >= 1000 ) {
      led_counter = millis();
      SerialUSB.println("Toggle sync output");
      dw1000_gpio_write(7, blink ? 1 : 0 );
      blink = !blink;
    }
  }
  */
}


void deca_setup_gpio() {
  bool blink = true;
  // GPIO7-8 , Sync / IRQ, are 0x0 for Sync/IRQ (default) and 0x1 for GPIO mode
    

  
  /* setup decawave LEDs as gpios for fun */
  // GPIO0-6 are gpio by default, 0x0 sets as GPIO
  dw1000_gpio_set_mode(0, 0x0);
  dw1000_gpio_set_mode(1, 0x0);
  dw1000_gpio_set_mode(2, 0x0);
  dw1000_gpio_set_mode(3, 0x0);
  dw1000_gpio_init_out(0, 0); 
  dw1000_gpio_init_out(1, 0); 
  dw1000_gpio_init_out(2, 0); 
  dw1000_gpio_init_out(3, 0); 


  /* Wait for sync pin to get exercised
   *  Old method, instead verify the OSTR mode works and time is getting reset
  SerialUSB.println("Waiting for decawave sync pin to toggle");
  dw1000_gpio_init_in(7); 
  dw1000_gpio_set_mode(7, 0x1);
  dw1000_gpio_init_in(7); 
  int init_val = 0;
  int sync_toggle_count = 0;
  
  init_val = dw1000_gpio_read(7);
  while ( sync_toggle_count < 2 ) {
    while ( init_val == dw1000_gpio_read(7) ) {
      delay(100);
      if ( (millis() - deca_led_counter) >= DECA_SETUP_BLINK_INTERVAL ) {
        deca_led_counter = millis();
        dw1000_gpio_write(0, blink ? 0 : 1 );
        dw1000_gpio_write(1, blink ? 0 : 1 );
        dw1000_gpio_write(2, blink ? 1 : 0 );
        dw1000_gpio_write(3, blink ? 1 : 0 );
        blink = !blink;
      }
    }
    sync_toggle_count++;
  }
  SerialUSB.println("Saw sync pin toggle!");
   */

  SerialUSB.println("Setting sync pin to external reset mode (OSTR)");
  SerialUSB.print("Value of IRQ pin:"); SerialUSB.println(digitalRead(22));
  dw1000_gpio_init_in(7); 
  dw1000_gpio_set_mode(7, 0x0); // set it to mode zero, sync mode 
  dw1000_gpio_init_in(7); 
  dw1000_phy_external_sync(33, true); // 33 recommended by user guide
  // wait is 8 bits, and modulo 4 should give 1 
  delay(1000);

  int saw_deca_time_roll_correct = 0;

  DW1000Time curDecaTime;
  DW1000Time lastDecaTime;
  bool lastPinVal = false;
  SerialUSB.println("Verifying Decawave Time is resetting at ~1Hz");
  // SYNC PIN RESETS SYSTEM TIME WHILE ITS HIGH
  // SO SYSTEM TIME WILL CONSTANTLY BE A VERY SMALL NUMBER FOR A LONG TIME
  while ( true ) {
    lastDecaTime = curDecaTime;
    DW1000.getSystemTimestamp(curDecaTime); 
    //SerialUSB.print("DW1000 system time:"); SerialUSB.println(curDecaTime.getAsMicroSeconds());
    if (( curDecaTime.getAsMicroSeconds() < lastDecaTime.getAsMicroSeconds() ) && 
          (lastDecaTime.getAsMicroSeconds() < 600000) && 
          (lastDecaTime.getAsMicroSeconds() > 300000) ) {
      saw_deca_time_roll_correct++; 
      SerialUSB.print("Saw deca time roll once:");
      SerialUSB.print("curDecaTime: "); SerialUSB.print(curDecaTime.getAsMicroSeconds());
      SerialUSB.print(" , lastDecaTime: "); SerialUSB.println(lastDecaTime.getAsMicroSeconds() );
    }

    if ( (millis() - deca_led_counter) >= DECA_SETUP_BLINK_INTERVAL ) {
      deca_led_counter = millis();
      dw1000_gpio_write(0, blink ? 1 : 0 );
      dw1000_gpio_write(1, blink ? 0 : 1 );
      dw1000_gpio_write(2, blink ? 0 : 1 );
      dw1000_gpio_write(3, blink ? 1 : 0 );
      blink = !blink;
    }
    if ( saw_deca_time_roll_correct > 2 ) {
      SerialUSB.println("Saw decawave timer get reset 5 times correctly, health check good!");
      break;
    }
    delay(10);
  }
}

void decawave_led_setmode(uint8_t dw_led_mode) {
  deca_led_mode = dw_led_mode;
}


// For some reason, the first line doesn't get applied????
void decawave_led_loop() {
  if ( (millis() - deca_led_counter) >= DECA_SETUP_BLINK_INTERVAL ) {
    deca_led_counter = millis();
    
    if ( deca_led_mode == DECA_LEDMODE_COUNTUP ) {  
      //SerialUSB.println("DECAWAVE COUNTUP");  
      dw1000_gpio_write(1, deca_led_shift_counter & 0x1 ? 1 : 0 );
      dw1000_gpio_write(3, deca_led_shift_counter & 0x2 ? 1 : 0 );
      dw1000_gpio_write(2, deca_led_shift_counter & 0x4 ? 1 : 0 );
      dw1000_gpio_write(0, deca_led_shift_counter & 0x8 ? 1 : 0 );
      deca_led_shift_counter++;
    } else if ( deca_led_mode == DECA_LEDMODE_COUNTDOWN ) {
      //SerialUSB.println("DECAWAVE COUNTDOWN");
      dw1000_gpio_write(1, deca_led_shift_counter & 0x1 ? 1 : 0 );
      dw1000_gpio_write(3, deca_led_shift_counter & 0x2 ? 1 : 0 );
      dw1000_gpio_write(2, deca_led_shift_counter & 0x4 ? 1 : 0 );
      dw1000_gpio_write(0, deca_led_shift_counter & 0x8 ? 1 : 0 );
      deca_led_shift_counter--;
    } else {
      //SerialUSB.println("DECAWAVE BLINK");
      dw1000_gpio_write(0, deca_led_shift_counter & 0x1 ? 1 : 0 );
      dw1000_gpio_write(2, deca_led_shift_counter & 0x1 ? 1 : 0 );
      dw1000_gpio_write(3, deca_led_shift_counter & 0x1 ? 1 : 0 );
      dw1000_gpio_write(1, deca_led_shift_counter & 0x1 ? 1 : 0 );
      deca_led_shift_counter++;
    }    
#if GPIO_DEBUG
    SerialUSB.println("");
#endif
  }
}











#endif

  
