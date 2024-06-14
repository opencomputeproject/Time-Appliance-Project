


#include "SI5341B.h"
#include "WWVB_Arduino.h"
#include "SiTime.h"
#include "SX1276_LORA.h"
#include "SX1257.h"
#include "stm32_sdr.h"
#include "ICE40.h"
//#include <arm_math.h> // Arduino CMSIS DSP library for signal processing 
//#include "mbed.h"


// the setup function runs once when you press reset or power the board



extern SX1257Class SX1257_SDR;

void SDR_Test_init();







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



void LoRA_WiWi_basic_test() {
  /*
  char print_buf[256];

  Serial.println("LORA WiWi basic test start");

  sprintf(print_buf, "MDMA Debug, sdr_I_Ch0_setup_lookup=0x%p, sdr_I_Ch0_dummy4_node=0x%p "
    "sdr_I_Ch1_setup_lookup=0x%p sdr_I_Ch1_dummy4_node=0x%p lookuptable=0x%p spi1_rx_data=0x%p",
    &sdr_I_Ch0_setup_lookup, &sdr_I_Ch0_dummy4_node,
    &sdr_I_Ch1_setup_lookup, &sdr_I_Ch1_dummy4_node,
    SDR_lookup_table, &spi1_rx_data);
  Serial.println(print_buf);

  SX1257_SDR.set_rx_mode(0, 0); // disable RX SDR path
  stm32_sdr_spi_init();
  stm32_sdr_dma_init();
  stm32_sdr_dfsdm_init();

  //SX1257_SDR.set_rx_mode(1, 1); // enable RX SDR path -> For actual data, only enable this after SPI pipeline is running

  //stm32_sdr_spi_test(); // THIS WORKED, I and Q both changing!
  //stm32_sdr_dma_test(); // THIS WORKS WOWWWWWW
  stm32_sdr_dfsdm_test();
  */

  //stm32_sdr_init();
  //stm32_lora_test();

  // V2 board code here
  // pipeline should be up, just need to enable SPI



  print_spi_registers("SPI1 I before SPI test", &SX1257_SDR._spi_I_Data);
  print_spi_registers("SPI2 Q before SPI test", &SX1257_SDR._spi_Q_Data);

  print_dma_registers("SPI1 I DMA before SPI test",(DMA_Stream_TypeDef *)(SX1257_SDR.hdma_spi1_rx.Instance) );
  print_dma_registers("SPI2 Q DMA before SPI test",(DMA_Stream_TypeDef *)(SX1257_SDR.hdma_spi2_rx.Instance) );

  SX1257_SDR.dumpRegisters(Serial);
  uint8_t last_I_data_num = 0;
  uint8_t last_Q_data_num = 0;
  char printbuf[256];

  // slow but fine, init all the buffers
  for ( int i =0; i < BUFFER_SIZE; i++ ){
    sram1_data->I_data[i] = 0x0;
    sram1_data->Q_data[i] = 0x0;
  }

  Serial.println("ENABLING STM32 SDR SPI WITH ICE40 RESET HELD");
  hold_ice40_reset();
  __HAL_SPI_ENABLE(&SX1257_SDR._spi_I_Data); 
  __HAL_SPI_ENABLE(&SX1257_SDR._spi_Q_Data); 
  delay(5);
  release_ice40_reset();
  delay(100);

  Serial.println("DISABLING STM32 SDR SPI");
  __HAL_SPI_DISABLE(&SX1257_SDR._spi_I_Data); 
  __HAL_SPI_DISABLE(&SX1257_SDR._spi_Q_Data); 

  last_I_data_num = __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi1_rx);
  last_Q_data_num = __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi2_rx);
  sprintf(printbuf, "Last I num=%d, Last Q num=%d\r\n", last_I_data_num, last_Q_data_num);
  Serial.print(printbuf);

  for ( int i =0; i < BUFFER_SIZE; i++ ) {
    //last_I_data_num = SX1257_SDR._spi_I_Data.Instance->RXDR;
    //last_Q_data_num = SX1257_SDR._spi_Q_Data.Instance->RXDR;

    last_I_data_num = __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi1_rx);
    last_Q_data_num = __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi2_rx);

    sprintf(printbuf,"I=%08d, I=0x%x, Q=0x%x\r\n", i, sram1_data->I_data[i] & 0xffff, sram1_data->Q_data[i] & 0xffff );
    Serial.print(printbuf);
  }


  return;



  //SPI debug
  // just stop SPI DMA and dump the buffer 



  // DFSDM debug
  // just stop DFSDM DMA and dump registers and re-enable it

  //stop_dfsdm_dma();



  // get the last DMA
  /*

  // print starting from the last one + 1, that's the oldest
  int last_I_data_num = 0;
  int last_Q_data_num = 0;
  char printbuf[256];

  for ( int i = 0; i < 10; i++ )
  {
    int i_offset = 0;
    int q_offset = 0;
    uint32_t i_data = 0;
    uint32_t q_data = 0;

    i_data = SX1257_SDR.hdfsdm_I.Instance->CHDATINR;
    q_data = SX1257_SDR.hdfsdm_Q.Instance->CHDATINR;

    last_I_data_num = __HAL_DMA_GET_COUNTER(SX1257_SDR.hdfsdm_filt_I.hdmaReg);
    last_Q_data_num = __HAL_DMA_GET_COUNTER(SX1257_SDR.hdfsdm_filt_Q.hdmaReg);
    i_offset = (i + last_I_data_num + 1) % FILTERED_BUFFER_SIZE;
    q_offset = (i + last_Q_data_num + 1) % FILTERED_BUFFER_SIZE;
    sprintf(printbuf, "Data %d, Idat=0x%x, Qdat=0x%x, DFSDM -> I=0x%x, Q=0x%x\r\n", i, i_data, q_data,
      sram1_data->filtered_I_buffer[i_offset], sram1_data->filtered_Q_buffer[q_offset]);
    Serial.print(printbuf);
  }
  */


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
  switch (command) {
    case '0':
      Serial.println("Starting RX IQ");
      // enable SDR
      SX1257_SDR.set_antenna(0); // setup SDR for RX
      SX1276_Lora.setantenna(1, 1, 0); // high frequency SMA SX1276->TX on standard transceiver
      // setup SDR RX
      SX1257_SDR.set_rx_parameters(0x6, 0xf, 0x7, 0x1, 0x1);
      SX1257_SDR.set_rx_mode(1, 1); // enable SDR RX path
      //SX1257_SDR.enable_rx_dma();
      break;
    case '1':      
      Serial.println("Stopping RX IQ");
      //SX1257_SDR.disable_dma();
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
    case '5':
      LoRA_WiWi_basic_test();
      break;
    case '6':
      SX1257_SDR.dumpRegisters(Serial);
      break;
    case '7':
      Serial.println("Enabling SDR TX!");
      SX1257_SDR.set_antenna(1); // setup SDR for TX
      SX1257_SDR.set_tx_parameters(0x3, 0xf,
        0x0, 0x0, 0x0); // max gain, bandwidth doesnt matter
      SX1257_SDR.set_tx_mode(1,1);
      break;
    case '8':
      Serial.println("Disabling SDR TX!");      
      SX1257_SDR.set_tx_mode(0,0);
      SX1257_SDR.set_antenna(0); // setup SDR for TX
      break;
    case '&':
      Serial.print("User command, restarting STM32!\r\n");
      delay(200);
      HAL_NVIC_SystemReset();
      break;
    // Add more cases as needed
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
          
          // Parse SET command
          int spaceIndex = commandString.indexOf(' ');
          if (spaceIndex != -1) {
            currentVariable = commandString.substring(spaceIndex + 1, commandString.indexOf(' ', spaceIndex + 1));
            currentValue = commandString.substring(commandString.indexOf(' ', spaceIndex + 1) + 1);
            processSetCommand(currentVariable, currentValue);
          }          
          inSetCommand = false;
          bufferIndex = 0; // Clear the buffer for the next command
        }
      } else {
        buffer[bufferIndex] = incomingChar;
        bufferIndex++;
        if (bufferIndex >= MAX_BUFFER_SIZE - 1) {
          // Buffer overflow, reset the buffer
          bufferIndex = 0;
          inSetCommand = false;
        }
      }
    } else {
      // If not inSetCommand, process single character command immediately
      if (incomingChar == 'S') {
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

  init_sram1_data();


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

  release_ice40_reset(); // release ice40 reset so I can access SX1257

  SX1257_SDR.init(1);


  SX1257_SDR.set_tx_freq(900e6);
  SX1257_SDR.set_rx_freq(900e6); 

  // enable SDR
  SX1257_SDR.set_antenna(0); // setup SDR for RX
  SX1276_Lora.setantenna(1, 1, 0); // high frequency SMA SX1276->TX on standard transceiver

  // setup SDR RX
  SX1257_SDR.set_rx_parameters(0x6, 0xf, 0x7, 0x1, 0x1);
  SX1257_SDR.set_rx_mode(1, 1); // enable SDR RX path
  Serial.println("****SX1257 registers during init****");
  SX1257_SDR.dumpRegisters(Serial);


  hold_ice40_reset(); // enabling pipelines , hold ice40 reset again

  // now SDR is setup in receive mode, good, enable DMA paths in STM32 for SPI and DFSDM
  // enable DFSDM first then SPI, basically reverse order of data flow
  // V2 , moving away from DFSDM, push it into FPGA 
  //Serial.println("Enabling DFSDM DMA pipelines!");
  //stm32_sdr_dfsdm_init();

  Serial.println("Enabling SPI DMA pipeline!");
  stm32_sdr_spi_init();



  // now DMA pipeline is setup, release ice40 reset so it starts converting SDR IQ RX data
  delay(10);
  release_ice40_reset();
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



// the loop function runs over and over again forever
void loop() {

  led_loop();
  handle_user_data();
  
  
  //SDR_DMA_Test();
  //LoRA_Test();
  //SDR_Test(); 


}
