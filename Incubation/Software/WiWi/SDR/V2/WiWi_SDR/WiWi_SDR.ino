
#include "WWVB_Arduino.h"
#include "SX1276_LORA.h"
#include "clockmatrix.h"



RNG_HandleTypeDef hrng;
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

  Serial.begin(9600);
  while ( !Serial ) {
    delay(1);
  }
  init_sram2_nocache();
 

  Serial.println("");
  Serial.println("");
  Serial.println("");

  // enable hardware RNG in STM32
  
  hrng.Instance = RNG;
  uint8_t wiwi_mac_addr = 0;
  if (HAL_RNG_Init(&hrng) != HAL_OK) {
    Serial.println("Failed to init hardware RNG");
    wiwi_mac_addr = random(0,255);
    sprintf(print_buffer, "Using wiwi mac 0x%x\r\n", wiwi_mac_addr);
    Serial.print(print_buffer);
  } else {
    uint32_t rand_num = 0;
    if (HAL_RNG_GenerateRandomNumber(&hrng, &rand_num) != HAL_OK) {
      Serial.println("Failed to get hardware RNG value");
      wiwi_mac_addr = random(0,255);
      sprintf(print_buffer, "Using wiwi mac 0x%x\r\n", wiwi_mac_addr);
      Serial.print(print_buffer);
    } else {
      Serial.println("Hardware RNG value successful!");
      wiwi_mac_addr = (uint8_t) (rand_num % 255);
      sprintf(print_buffer, "Using wiwi mac 0x%x\r\n", wiwi_mac_addr);
      Serial.print(print_buffer);
    }
  }

  Serial.println("Early init done!");

  init_cli();


  init_clockmatrix();

  // DPLL DEBUG HACK
  //wwvb_gpio_pinmode_pullup(PLL_SDA, OUTPUT);
  //wwvb_gpio_pinmode_pullup(PLL_SCL, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  rtos::ThisThread::sleep_for(1000);
  // This works, probed on board, acknowledged!
  //cm_i2c.llStart((0x58<<1)+1);
  //cm_i2c.llStart((0x58<<1));
  //cm_i2c.llWrite(0x50);
  //cm_i2c.llWrite(0x0);
  //cm_i2c.stop();
  /*
  Serial.println("Loop debug i2c");
  cm_i2c.beginTransmission(0x58);
  cm_i2c.write(0x50);
  cm_i2c.write(0x0);
  cm_i2c.endTransmission() ;
  */
  
}
