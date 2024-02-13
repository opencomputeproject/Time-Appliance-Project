


#include "SI5341B.h"
#include "WWVB_Arduino.h"
#include "SiTime.h"
#include "SX1276_LORA.h"
#include "SX1257.h"
//#include <arm_math.h> // Arduino CMSIS DSP library for signal processing 
//#include "mbed.h"


// the setup function runs once when you press reset or power the board

LoRaClass SX1276_Lora;

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
  Serial.println("LORA WiWi basic test start");
  uint32_t pll_start_time = 0;
  uint32_t pll_end_time = 0;

  // enable SDR
  SX1257_SDR.set_antenna(0); // setup SDR for RX
  SX1276_Lora.setantenna(1, 1, 0); // high frequency SMA SX1276->TX on standard transceiver

  // setup SDR RX
  SX1257_SDR.set_rx_parameters(0x6, 0xf, 0x7, 0x0, 0x3);
  SX1257_SDR.set_rx_mode(1, 1); // enable SDR RX path

  pll_start_time = millis();

  // wait for SDR RX PLL to start
  while ( (SX1257_SDR.readRegister(0x11) & 0x2) == 0 ) {    
  }


  pll_end_time = millis();
  Serial.print("Waited for SDR PLL for ");
  Serial.print(pll_end_time - pll_start_time);
  Serial.println("");

  //SX1257_SDR.dumpRegisters(Serial);
  
  SX1257_SDR.reset_dma_buffers();

  uint32_t i_start_count = 0;
  uint32_t q_start_count = 0;
  uint32_t i_end_count = 0;
  uint32_t q_end_count = 0;


  // enable SDR DMA
  SX1257_SDR.enable_rx_dma();
  i_start_count = __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi1_rx);
  q_start_count = __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi2_rx);

  // USING EXTERNAL GENERATOR, DONT NEED SX1276

  // send a packet with SX1276
  /*
  SX1276_Lora.beginPacket();
  SX1276_Lora.print("hello ");
  SX1276_Lora.endPacket(false);
  */
  //delay(10);
  while ( __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi1_rx) > 200 ) {

  }

  // packet was sent, disable SDR DMA
  SX1257_SDR.disable_dma();

  i_end_count = __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi1_rx);
  q_end_count = __HAL_DMA_GET_COUNTER(&SX1257_SDR.hdma_spi2_rx);

  Serial.print("I start ");
  Serial.print(i_start_count);
  Serial.print(" end ");
  Serial.print(i_end_count);

  Serial.print(" Q start ");
  Serial.print(q_start_count);
  Serial.print(" end ");
  Serial.println(q_end_count);

  // dump out DMA data
  SX1257_SDR.print_rx_iq_data();

  Serial.println("LORA WIWI basic test end");  
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
      SX1257_SDR.enable_rx_dma();
      break;
    case '1':      
      Serial.println("Stopping RX IQ");
      SX1257_SDR.disable_dma();
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
      SX1257_SDR.print_rx_iq_data();
      break;
    case '5':
      LoRA_WiWi_basic_test();
      break;
    case '6':
      SX1257_SDR.dumpRegisters(Serial);
      break;
    case '7':
      Serial.println("Resetting SDR!");
      SX1257_SDR.init(0);
    case '8':
      debug_spi1_print();
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
  init_sitime(10e6);
  init_si5341b();

  Serial.println("");
  Serial.println("");
  Serial.println("");


  /****** INTERRUPT IS DEFINITELY NOT RIGHT FOR PROPER LORA OPERATION IN SX1276_LORA ***/
  // SX1276 can use external interrupt
  
  SX1276_Lora.init(); 
  Serial.println("Beginning SX1276 LORA");
  if ( !SX1276_Lora.begin(915e6) ) {
    Serial.println("LoRA SX1276 init failed!");
  } else {
    Serial.println("LoRA SX1276 init successful!");
    SX1276_Lora.dumpRegisters(Serial);
  }
  

  SX1257_SDR.init(1);
  SX1257_SDR.set_tx_freq(915e6);
  SX1257_SDR.set_rx_freq(915e6);

  SX1257_SDR.dumpRegisters(Serial);

}




unsigned long last_led_toggle_millis = 0;
bool leds_on = 0;
int led_count = 0;

void led_loop() {
  if ( millis() - last_led_toggle_millis >= 1000 ) {
	  Serial.println("LED Loop start");
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
