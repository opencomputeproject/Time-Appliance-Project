


#include "SI5341B.h"
#include "WiWi_Network.h"
#include "WWVB_Arduino.h"
#include "SiTime.h"
#include "SX1276_LORA.h"
#include "SX1257.h"
#include "stm32_sdr.h"
#include "ICE40.h"
#include "stm32_pps.h"
#include "WWVB_RF.h"

//#include "mbed.h"


// the setup function runs once when you press reset or power the board



extern SX1257Class SX1257_SDR;

bool start_wiwi = false;

void SDR_Test_init();

RNG_HandleTypeDef hrng;





#define MAX_BUFFER_SIZE 64

char buffer[MAX_BUFFER_SIZE];
int bufferIndex = 0;
bool inSetCommand = false;

String currentVariable;
String currentValue;

void processSetCommand(String variable, String value) {
  // Check for specific variables
  // Implement your logic for handling specific variables here
  // Example:
  /*
  sprintf(print_buffer, "processSetCommand, variable = %s, value = %s\r\n", 
	variable.c_str(), value.c_str());
	Serial.print(print_buffer);
	*/
  if (variable == "KP") {
    float floatValue = value.toFloat();
    //Serial.printf("Setting KP to %f\r\n",floatValue);
    //KP = floatValue;
    // Your logic for handling KP here
  } else if (variable == "KI") {
    float floatValue = value.toFloat();
    //Serial.printf ("Setting KI to %f\r\n", floatValue);
    // Your logic for handling KI here
    //KI = floatValue;
  } else if (variable == "KD") {
    float floatValue = value.toFloat();
    //Serial.printf ("Setting KD to %f\r\n", floatValue);
    // Your logic for handling KI here
    //KD = floatValue;
  } else if (variable == "ALPHA") {
    float floatValue = value.toFloat();
    //Serial.printf ("Setting ALPHA to %f\r\n", floatValue);
    // Your logic for handling KI here
    //ALPHA = floatValue;
  } else if (variable == "MROC") {
    float floatValue = value.toFloat();
    //Serial.printf ("Setting Max rate of change (MROC) to %f\r\n", floatValue);
    // Your logic for handling KI here
    //MAX_RATE_OF_CHANGE = floatValue;
  } else if ( variable == "wiwimac" ) {
	  uint8_t macval = ( (uint8_t)value.toInt() ) ;
	  wiwi_mac_addr = macval;
	  sprintf(print_buffer, "Setting WiWi mac from manual command to 0x%x\r\n", wiwi_mac_addr);
	  Serial.print(print_buffer);
  } else if ( variable == "add_clientAnchor" ) {
	  uint8_t macval = ( (uint8_t)value.toInt() ) ;
	  masterAnchor_startAnchorSub(macval);
	  sprintf(print_buffer, "Master anchor, adding client anchor 0x%x\r\n", macval);
	  Serial.print(print_buffer);
  } else if ( variable == "add_wiwiTag" ) {
	  uint8_t macval = ( (uint8_t)value.toInt() ) ;
	  masterAnchor_startTagSub(macval);
	  sprintf(print_buffer, "Master anchor, adding tag 0x%x\r\n", macval);
	  Serial.print(print_buffer);
  }

}

void print_SDR_DMA_Stats() {

  if ( SX1257_SDR._spi_I_Data.State == 0x4 ) {
  Serial.print("I RX -> DMA Fifo Fill state: 0x");
  Serial.println(__HAL_DMA_GET_FS(SX1257_SDR._spi_I_Data.hdmarx), HEX);
  Serial.print("I RX -> DMA counter remaining:");
  Serial.println(__HAL_DMA_GET_COUNTER(SX1257_SDR._spi_I_Data.hdmarx));

  Serial.print("Q RX -> DMA Fifo Fill state: 0x");
  Serial.println(__HAL_DMA_GET_FS(SX1257_SDR._spi_Q_Data.hdmarx), HEX);
  Serial.print("Q RX -> DMA counter remaining:");
  Serial.println(__HAL_DMA_GET_COUNTER(SX1257_SDR._spi_Q_Data.hdmarx));
  } 
  else {
    Serial.print("SX1257 SDR DMA not running, current state 0x");
    Serial.println(SX1257_SDR._spi_I_Data.State, HEX);

  Serial.print("I RX -> DMA counter remaining:");
  Serial.println(__HAL_DMA_GET_COUNTER(SX1257_SDR._spi_I_Data.hdmarx));
  Serial.print("Q RX -> DMA counter remaining:");
  Serial.println(__HAL_DMA_GET_COUNTER(SX1257_SDR._spi_Q_Data.hdmarx));
  }
}




void debug_spi1_print() {
  uint32_t vals[512];
  Serial.print("SPI1 debug:");
  for ( int i =0 ; i < 512; i++ ) {
    vals[i] = SX1257_SDR._spi_I_Data.Instance->RXDR;
  }
  for ( int i = 0; i < 512; i++ ) {
    Serial.print("0x");
    Serial.print(vals[i], HEX);
    Serial.print(",");
  }
  Serial.println("");
}




void processSingleCharCommand(char command) {
  // Implement your logic for handling single character commands here
  // Example:
  wiwi_pkt_announce fake_packet;
  wiwi_pkt_delay fake_delay_packet;
  int count = 0;
  int rssi = 0;
  float snr = 0;
  int packetSize;
  uint8_t pktbuf[40];
  static int64_t adjust_val = 1;
  static int64_t adjust_val_accum = 0;
  phaseUnion dummyVal;
  uint32_t randval;
  switch (command) {
    case '0': // master anchor mode
    case '!': // slave anchor mode
    case '1': // tag mode, device tracking 
      Serial.println("Setting start_wiwi flag due to user input!\r\n");
      sprintf(print_buffer,"WiWi disc parameters: Kp=%0.15f Ki=%0.15f Kd=%0.15f\r\n", KP, KI, KD);
      Serial.print(print_buffer); 
      start_wiwi = true;
      if ( command == '0' ) {
        wiwi_network_mode = WIWI_MODE_MASTER_ANCHOR;
        wiwi_mac_addr = 0; // only one master, 0
		Serial.println("Starting as wiwi master anchor!");
      } else if ( command == '!') {
        wiwi_network_mode = WIWI_MODE_SLAVE_ANCHOR;
		if ( wiwi_mac_addr == 0 ) {
			HAL_RNG_GenerateRandomNumber(&hrng, &randval);
			wiwi_mac_addr = (randval % 100) + 1; // between 1 and 100
		}
		Serial.println("Starting as wiwi client anchor!");
      } else {
        wiwi_network_mode = WIWI_MODE_TAG;
		if ( wiwi_mac_addr == 0 ) {
			HAL_RNG_GenerateRandomNumber(&hrng, &randval);
			wiwi_mac_addr = (randval % 155) + 101; // between 101 and 255
		}
		Serial.println("Starting as Wiwi tag!");
      }
      switch_lora_to_rx();
      sprintf(print_buffer,"My MAC = 0x%x\r\n", wiwi_mac_addr);
      Serial.print(print_buffer);  
      break;
    /********** Manual frequency up ************/
    case 'u': // ultra small frequency up
      // hack for current boards, frequency resolution is 5e-12
      frequency_offset += 0.01; // 10 mHz
      apply_freq_change(0);
      break;
    case 'i': // incremental frequency up
      frequency_offset += 1; // 1Hz
      apply_freq_change(0);
      break;
    case 'U': // next frequency up
      frequency_offset += 100; // 100Hz
      apply_freq_change(0);
      break;
    case 'I': // next frequency up
      frequency_offset += 1000; // 1KHz
      apply_freq_change(0);
      break;
    /********** Manual frequency down ********/
    case 'c': // ultra small frequency down
      // hack for current boards, frequency resolution is 5e-12
      frequency_offset -= 0.01; // 10 mHz
      apply_freq_change(0);
      break;
    case 'C': // next frequency down
      frequency_offset -= 100; // 100Hz
      apply_freq_change(0);
      break;

    case '2':
      wwvb_digital_write(WLED_RED, LOW);
      Serial.print("Set LED to red!\r\n");
      break;
    case '3':
      Serial.println("Printing SDR stats");
      print_SDR_DMA_Stats();
      break;
    case '4':
      Serial.println("Printing IQ rx buffer");
      //SX1257_SDR.print_rx_iq_data(1);
      break;
    case '6':
    case 'w':
    /*
      // IQ testing announce packet
      fake_packet.hdr.wiwi_id = htonl(0x77697769); // uin32_t / uint16_t needs reverse order
      fake_packet.hdr.mac_src = 0xfe;
      fake_packet.hdr.mac_dest = 0xff;
      fake_packet.hdr.pkt_type = WIWI_PKT_ANNOUNCE;
      fake_packet.hdr.seq_num = 0;
      fake_packet.hdr.ack_num = 0;
      fake_packet.checksum = 0xa5;
      fake_packet.reserved[0] = 0;
      fake_packet.reserved[1] = 0;
      fake_packet.reserved[2] = 0;
      fake_packet.reserved[3] = 0;
      fake_packet.unusedtwo[0] = 0;
      fake_packet.unusedtwo[1] = 0;
      fake_packet.unusedtwo[2] = 0;

      send_packet( (uint8_t*) &fake_packet, sizeof(wiwi_pkt_announce), 0, 0 );
      */
    
      break;
    case 'e':
    /*
      // IQ testing delay packet
      fake_delay_packet.hdr.wiwi_id = htonl(0x77697769); // uin32_t / uint16_t needs reverse order
      fake_delay_packet.hdr.mac_src = 0xfe;
      fake_delay_packet.hdr.mac_dest = 0xff;
      fake_delay_packet.hdr.pkt_type = WIWI_PKT_DELAY_REQ;
      fake_delay_packet.hdr.seq_num = 0;
      fake_delay_packet.hdr.ack_num = 0;
      fake_delay_packet.previous_tx_ts = 0xa5a5a5a5;
      fake_delay_packet.previous_tx_iq.intval = 0xa5a5a5a5;
      fake_delay_packet.previous_rx_ts = 0xa5a5a5a5;
      fake_delay_packet.previous_rx_iq.intval = 0xa5a5a5a5;
      fake_delay_packet.checksum = 0xa5;
      fake_delay_packet.reserved[0] = 0;
      fake_delay_packet.reserved[1] = 0;
      fake_delay_packet.reserved[2] = 0;
      fake_delay_packet.reserved[3] = 0;
      fake_delay_packet.reserved[4] = 0;
      fake_delay_packet.reserved[5] = 0;
      fake_delay_packet.reserved[6] = 0;
      fake_delay_packet.reserved[7] = 0;
      fake_delay_packet.reserved[8] = 0;
      fake_delay_packet.reserved[9] = 0;
      fake_delay_packet.reserved[10] = 0;
      fake_delay_packet.reserved[11] = 0;
      fake_delay_packet.reserved[12] = 0;

      send_packet( (uint8_t*) &fake_delay_packet, WIWI_PKT_DELAY_LEN, 0, 0 );
    */

      break;
    case '7':
      // useful for manufacturing testing, measure RF output
      Serial.println("Enabling SDR TX!");
      SX1257_SDR.set_antenna(1); // setup SDR for TX
      SX1257_SDR.set_tx_parameters(0x3, 0xf, 0x0, 0x0, 0x0); // max gain, bandwidth doesnt matter
      SX1257_SDR.set_tx_mode(1,1);
      break;
    case '8':
      Serial.println("Disabling SDR TX!");      
      SX1257_SDR.set_tx_mode(0,0);
      SX1257_SDR.set_antenna(0); // setup SDR for RX
      break;
    case 'p':
      Serial.println("Enabling HRTIMER PPS output!");
      init_stm_pps();
      break;
    case '+':
    case '=':
      adjust_val *= 2;
      sprintf(print_buffer, "Doubling adjustment value to %"PRId64"\r\n", adjust_val);
      Serial.print(print_buffer);
      break;
    case '-':
    case '_':
      adjust_val /= 2;
      if ( adjust_val == 0 ) adjust_val = 1;
      sprintf(print_buffer, "Halving adjustment value to %"PRId64"\r\n", adjust_val);
      Serial.print(print_buffer);
      break;
    case 'L':
    case 'd':
    case 'D':
      break;
    case 'l':
    case 'a':
    case 'A':
      break;
    case 'M':
      break;
    case 'm':
      break;
    case '&':
      Serial.print("User command, restarting STM32!\r\n");
      delay(200);
      HAL_NVIC_SystemReset();
      break;
    
    case 'r': // test case, wait for and receive one LoRA WiWi packet
      // setup LoRA RX
      Serial.println("Manual command, wait for one packet");
      while ( 1 ) {
        if ( receive_lora_packet(pktbuf, &packetSize, 0) )
        {
          Serial.println("Received one packet!");
          break;
        }
        else
        {
          count++;
          delay(10);
          if ( (count % 100) == 0 ) {
            Serial.println("Waiting for LoRA packet test");
          }
        } 
      }

      break;
    case 'T': // formal test cases;
      Serial.println("Running formal test cases!!!!!!");
      test_dio0_interrupt();
      Serial.println("");
      delay(2000);
      test_ice40_stream_enable_disable();
      Serial.println("");
      delay(2000);
      test_ice40_reset();
      Serial.println("");
      delay(2000);
      test_ice40_fixed_pattern();
      Serial.println("");
      delay(2000);
      break;
    // Add more cases as needed
    case 'f':
      ice40_test();
      break;
    default:
      Serial.println("Unknown command");
      break;
  }
}

void handle_user_data() {
  while (Serial.available() > 0) {
    char incomingChar = Serial.read();

    if (inSetCommand) {
      // If inSetCommand is true, append to the buffer until newline
      if (incomingChar == '\r' || incomingChar == '\n') {
        if (bufferIndex > 0) {
          buffer[bufferIndex] = '\0'; // Null-terminate the string
          String commandString(buffer);
		  sprintf(print_buffer, "Parsing command string %s\r\n", buffer);
		  //Serial.print(print_buffer);
          
          // Parse SET command
          int spaceIndex = commandString.indexOf(' ');
          if (spaceIndex != -1) {
            currentVariable = commandString.substring(spaceIndex + 1, commandString.indexOf(' ', spaceIndex + 1));
            currentValue = commandString.substring(commandString.indexOf(' ', spaceIndex + 1) + 1);
            processSetCommand(currentVariable, currentValue);
          }          
          inSetCommand = false;
		  //Serial.println("Not in set command!");
          bufferIndex = 0; // Clear the buffer for the next command
        }
      } else {
        buffer[bufferIndex] = incomingChar;
        bufferIndex++;
        if (bufferIndex >= MAX_BUFFER_SIZE - 1) {
          // Buffer overflow, reset the buffer
          bufferIndex = 0;
          inSetCommand = false;
		  //Serial.println("Not in set command buffer overflow!");
        }
      }
    } else {
      // If not inSetCommand, process single character command immediately
      if (incomingChar == 'S') {
		//Serial.println("In set command!");
        inSetCommand = true;
        currentVariable = "";
        currentValue = "";
        bufferIndex = 0;
        buffer[bufferIndex++] = incomingChar;
      } else {
        processSingleCharCommand(incomingChar);
      }
    }
  }
}










void setup() {
  
  // Using STM32 HAL in general
  HAL_Init();

  __HAL_RCC_SYSCFG_CLK_ENABLE();

  // STM32 GPIO HAL, https://github.com/STMicroelectronics/stm32h7xx_hal_driver/blob/master/Src/stm32h7xx_hal_gpio.c
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE(); // lots of GPIO on J
  __HAL_RCC_GPIOK_CLK_ENABLE(); // change K if using other GPIOs, but LED are all K
  
  __HAL_RCC_ADC12_CLK_ENABLE(); // ADC for WWVB path
  __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);
  

  // SPI5 for SX1276
  __HAL_RCC_SPI5_CLK_ENABLE();

  // SPI1 / 2 / 6 for SX1257
  __HAL_RCC_SPI1_CLK_ENABLE();
  __HAL_RCC_SPI2_CLK_ENABLE();
  __HAL_RCC_SPI6_CLK_ENABLE();

  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  __HAL_RCC_MDMA_CLK_ENABLE();
  __HAL_RCC_C1_MDMA_CLK_ENABLE();
  __HAL_RCC_C2_MDMA_CLK_ENABLE();

  __HAL_RCC_DFSDM1_CLK_ENABLE();
  //__HAL_RCC_DFSDM2_CLK_ENABLE();

  __HAL_RCC_D2SRAM1_CLK_ENABLE();
  __HAL_RCC_C1_D2SRAM1_CLK_ENABLE();

  __HAL_RCC_D2SRAM2_CLK_ENABLE();
  __HAL_RCC_C1_D2SRAM2_CLK_ENABLE();

  __HAL_RCC_D2SRAM3_CLK_ENABLE();
  __HAL_RCC_C1_D2SRAM3_CLK_ENABLE();

  __HAL_RCC_RNG_CLK_ENABLE(); // enable hardware RNG

  __HAL_RCC_HRTIM1_CLK_ENABLE();
  __HAL_RCC_C1_HRTIM1_CLK_ENABLE();
  __HAL_RCC_C2_HRTIM1_CLK_ENABLE();


  __HAL_RCC_TIM1_CLK_ENABLE();
  __HAL_RCC_C1_TIM1_CLK_ENABLE();
  __HAL_RCC_C2_TIM1_CLK_ENABLE();

  wwvb_gpio_pinmode(WLED_RED, OUTPUT);
  wwvb_gpio_pinmode(WLED_GREEN, OUTPUT);
  wwvb_gpio_pinmode(WLED_BLUE, OUTPUT);

  wwvb_gpio_pinmode(WWVB_AMP1_CS, OUTPUT);
  wwvb_gpio_pinmode(WWVB_AMP2_CS, OUTPUT);

  wwvb_digital_write(WWVB_AMP1_CS, 1);
  wwvb_digital_write(WWVB_AMP2_CS, 1);
  
  Serial.begin(9600);
  while ( !Serial ) {
    delay(1);
  }
  init_sram2_nocache();
  init_sitime(10e6);
  init_si5341b();

  Serial.println("");
  Serial.println("");
  Serial.println("");

  // enable hardware RNG in STM32
  // wiwi_mac_addr = 0 for master anchor, 1 to 100 for slave anchors, 101 to 255 for tags
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK) {
    Serial.println("Failed to init hardware RNG");
    wiwi_mac_addr = random(1,100);
    sprintf(print_buffer, "Using wiwi mac 0x%x\r\n", wiwi_mac_addr);
    Serial.print(print_buffer);
  } else {
    uint32_t rand_num = 0;
    if (HAL_RNG_GenerateRandomNumber(&hrng, &rand_num) != HAL_OK) {
      Serial.println("Failed to get hardware RNG value");
      wiwi_mac_addr = random(1,100);
      sprintf(print_buffer, "Using wiwi mac 0x%x\r\n", wiwi_mac_addr);
      Serial.print(print_buffer);
    } else {
      Serial.println("Hardware RNG value successful!");
      if ( rand_num == 0 ) rand_num = 1;
      wiwi_mac_addr = (uint8_t) (rand_num % 255);
      sprintf(print_buffer, "Using wiwi mac 0x%x\r\n", wiwi_mac_addr);
      Serial.print(print_buffer);
    }
  }

  zero_iq_buffers();



  /****** INTERRUPT IS DEFINITELY NOT RIGHT FOR PROPER LORA OPERATION IN SX1276_LORA ***/
  // SX1276 can use external interrupt
  
  SX1276_Lora.init(); 
  Serial.println("Beginning SX1276 LORA");
  if ( !SX1276_Lora.begin(900e6) ) {
    Serial.println("LoRA SX1276 init failed!");
  } else {
    Serial.println("LoRA SX1276 init successful!");
    SX1276_Lora.dumpRegisters(Serial);
  }

  

  /* ICE40 setup */
  hold_ice40_reset();

  // Hack put SX1257 control pin init here
  wwvb_gpio_pinmode(SX1257_RST, OUTPUT);  
  wwvb_digital_write(SX1257_RST, 1); // RESET IS ACTIVE HIGH

  Serial.println("STARTING ICE40 DOWNLOAD");
  prog_bitstream_start();
  prog_bitstream_send((unsigned char*)fpgaFirmware, fpgaFirmware_size);
  prog_bitstream_finish();


  bool cdone_high = wwvb_digital_read(ICE_CDONE) == HIGH;
  if ( cdone_high ) {
    Serial.println("CDONE HIGH AFTER PROG, ICE40 Download done!");
  } else {
    Serial.println("CDONE LOW AFTER PROG, BAD");
  }
  Serial.println("ICE40 DOWNLOAD DONE!");
  delay(50); // give ice40 some time to chill

  wwvb_gpio_pinmode(ICE_SPARE5, OUTPUT);
  wwvb_digital_write(ICE_SPARE5, 0); // disable FPGA stream by default

  release_ice40_reset(); // release ice40 reset so I can access SX1257

  SX1257_SDR.init(1);


  SX1257_SDR.set_tx_freq(900e6);
  SX1257_SDR.set_rx_freq(900e6); 


  SX1257_SDR.set_antenna(0); // setup SDR for RX
  SX1276_Lora.setantenna(1, 1, 0); // high frequency SMA SX1276->TX on standard transceiver

  // setup SDR RX
  SX1257_SDR.set_rx_parameters(0x6, 0xe, 0x7, 0x0, 0x1);
  SX1257_SDR.set_rx_mode(1, 1); // enable SDR RX path
  Serial.println("****SX1257 registers during init****");
  SX1257_SDR.dumpRegisters(Serial);

  wiwi_network_setup();

  sdr_iq_init();


  // Put it back in continous RX mode
  switch_lora_to_rx();

  WWVB_RF_Init();
  
  init_masterAnchor(); // just initializing data structures


  
  Serial.println("Init fully done!");
  return;

}




unsigned long last_led_toggle_millis = 0;
bool leds_on = 0;
int led_count = 0;

void led_loop() {
  if ( millis() - last_led_toggle_millis >= 1000 ) {
	  //Serial.println("LED Loop start");
    if ( leds_on ) {    

      wwvb_digital_write(WLED_RED, HIGH);
      wwvb_digital_write(WLED_BLUE, HIGH);
      wwvb_digital_write(WLED_GREEN, HIGH);
      leds_on = 0;
    } else {      
      if ( led_count == 0 ) {
        wwvb_digital_write(WLED_RED, LOW);
        wwvb_digital_write(WLED_BLUE, HIGH);
        wwvb_digital_write(WLED_GREEN, HIGH);
        led_count++;
      } 
      else if ( led_count == 1 ) {
        wwvb_digital_write(WLED_RED, HIGH);
        wwvb_digital_write(WLED_BLUE, LOW);
        wwvb_digital_write(WLED_GREEN, HIGH);
        led_count++;
      }
      else if ( led_count == 2 ) {
        wwvb_digital_write(WLED_RED, HIGH);
        wwvb_digital_write(WLED_BLUE, HIGH);
        wwvb_digital_write(WLED_GREEN, LOW);
        led_count = 0;
      }
      leds_on = 1;
    }
    last_led_toggle_millis = millis();
    //SX1257_SDR.dumpRegisters(Serial);
    
    
  } 
  //delay(20);
  //Serial.println("led loop");

}


void run_wiwi_rx() {
  // handle an incoming packet if there is one
  // for this board, basically handle if dio0 interrupt fired
  if ( is_receive_packet_available() )
  {

    //Serial.println("Run wiwi RX, packet available!");
    // could either be reception or TX done
    // get IQ data from DMA for both TX and RX 

    // RX packet
    // Read packet digital data from SX1276
    int rssi = 0;
    int snr = 0;
    phaseUnion tempPhase;
    int rx_packet_index = free_packet_list.shift(); 
    uint8_t * rx_packet_ptr = packet_buffer[rx_packet_index].data;

    receive_lora_packet(rx_packet_ptr, &rssi, &tempPhase);
    packet_buffer[rx_packet_index].phase.value = tempPhase.value;
    packet_buffer[rx_packet_index].pkt_len = (uint8_t) rssi;
    packet_buffer[rx_packet_index].timestamp = micros();
    sprintf(print_buffer, "Run_wiwi_rx, Received packet, phase = 0x%x, timestamp: %ld\r\n", 
		tempPhase.intval,
		packet_buffer[rx_packet_index].timestamp);
    Serial.print(print_buffer);

    rx_packet_list.add(rx_packet_index); // put into RX list for network stack
    //Serial.println("Done with run_wiwi_rx\r\n\r\n");

  }
}



void run_wiwi_tx() {
  if ( tx_packet_list.size() == 0 ) {
    // nothing to transmit right now
    return;
  } else {
    // have a packet network stack wants to send
    // just send it immediately
    int single_pkt = 0;
    single_pkt = tx_packet_list.get(0); // pop the tx packet 
    packet * tx_pkt = &packet_buffer[single_pkt];

    sprintf(print_buffer, "WiWi LoRA TX handle index %d pktlen %d\r\n", single_pkt, tx_pkt->pkt_len);
    Serial.print(print_buffer);

    phaseUnion temp;
    send_packet(tx_pkt->data, tx_pkt->pkt_len, &temp, &tx_pkt->timestamp);
    tx_pkt->phase.value = temp.value;


    // I wrote the network stack assuming the "RX" chip will receive the packet you transmit
    // fake that in the code here
    Serial.println("Done with run_wiwi_tx");
    Serial.println("");
    Serial.println("");

    // push the buffer back into free list
    single_pkt = tx_packet_list.shift(); // free packet from top of tx list
    rx_packet_list.add(single_pkt); // push it on RX with meta data
  }


}

void run_wiwi_discipline()
{
	if ( wiwi_network_mode == WIWI_MODE_MASTER_ANCHOR ) {
		masterAnchor_handleFullWiWiData();
	} else if ( wiwi_network_mode == WIWI_MODE_SLAVE_ANCHOR ) {
		clientAnchor_handleFullWiWiData();
	}
}

void wiwi_run() {
  if ( !start_wiwi ) return;

  run_wiwi_rx();

  run_wiwi_network();

  run_wiwi_tx();
  
  run_wiwi_discipline();
  
  sdr_iq_agc_run();


}

// the loop function runs over and over again forever
void loop() {

  led_loop();
  handle_user_data();
  wiwi_run();

  loop_stm_pps();

  WWVB_RF_Loop();


  
  
  //SDR_DMA_Test();
  //LoRA_Test();
  //SDR_Test(); 


}
