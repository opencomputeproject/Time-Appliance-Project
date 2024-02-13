#ifndef WWVB_ARDUINO_H
#define WWVB_ARDUINO_H

#include <stdint.h>
#include <stm32h7xx_hal.h>
#include <stm32h7xx_hal_gpio.h>
#include <Arduino.h>

#include <stm32h7xx_hal_spi.h>
#include <stm32h7xx_hal_dma.h>
#include <stm32h7xx.h>

//#include "mbed.h"
//#include "rtos.h"
//#include <RPC.h>
// Low level library to make it easier to use WWVB board

//using namespace mbed;
//using namespace rtos;
//using namespace std::chrono_literals;






// just sequential, easy addressable
enum WWVB_Pin_Name {
	WLED_RED=0,
	WLED_GREEN,
	WLED_BLUE,

	PLL_SDA,
	PLL_SCL,
	PLL_RST,

  SIT5501_SDA,
  SIT5501_SCL,

  SX1276_MISO,
  SX1276_MOSI,
  SX1276_SCK,
  SX1276_NSS,
  SX1276_RST,
  SX1276_DIO0,
  SX1276_DIO1,
  SX1276_DIO2,
  SX1276_DIO3,
  SX1276_DIO4,
  SX1276_DIO5,

  SX1257_MISO,
  SX1257_MOSI,
  SX1257_SCK,
  SX1257_NSS,
  SX1257_RST,
  SX1257_CLK_OUT_SPI1,
  SX1257_CLK_OUT_SPI2,
  SX1257_I_IN,
  SX1257_Q_IN,
  SX1257_I_OUT,
  SX1257_Q_OUT,

  WWVB_AMP1_CS,
  WWVB_AMP2_CS,

  // RF switch control signals
  WWVB_SMA_UFL_SEL,
  WWVB_AMPLIFIER_SEL0,
  WWVB_AMPLIFIER_SEL1,

  LORA_SMA_UFL_SEL,
  SDR_TX_RX_SEL,
  LORA_LF_TXRX_SEL,
  LORA_HF_TXRX_SEL,
  LORA_LF_HF_SEL,






  NUM_PINS
};

typedef struct WWVB_Pin_Config {
	uint16_t GPIO_Pin;
	GPIO_TypeDef * GPIO_Group;
} WWVB_Pin;

extern WWVB_Pin WWVB_Pins[];

/****** LED pins **********/
#define WLED_RED_N GPIO_PIN_5
#define WLED_RED_G GPIOK

#define WLED_GREEN_N GPIO_PIN_6
#define WLED_GREEN_G GPIOK

#define WLED_BLUE_N GPIO_PIN_7
#define WLED_BLUE_G GPIOK

/******* SIT5501 Oscillator Pins *******/
#define SIT5501_SDA_N GPIO_PIN_0
#define SIT5501_SDA_G GPIOF

#define SIT5501_SCL_N GPIO_PIN_1
#define SIT5501_SCL_G GPIOF


/******* PLL pins ********/
#define PLL_SDA_N GPIO_PIN_2
#define PLL_SDA_G GPIOJ

#define PLL_SCL_N GPIO_PIN_1
#define PLL_SCL_G GPIOJ

#define PLL_RST_N GPIO_PIN_3
#define PLL_RST_G GPIOJ

/******* SX1276 pins ******/
// uses SPI5
#define SX1276_MISO_N GPIO_PIN_8
#define SX1276_MISO_G GPIOF

#define SX1276_MOSI_N GPIO_PIN_9
#define SX1276_MOSI_G GPIOF

#define SX1276_SCK_N GPIO_PIN_0
#define SX1276_SCK_G GPIOK

#define SX1276_NSS_N GPIO_PIN_1
#define SX1276_NSS_G GPIOK

#define SX1276_RST_N GPIO_PIN_3
#define SX1276_RST_G GPIOB

#define SX1276_DIO0_N GPIO_PIN_9
#define SX1276_DIO0_G GPIOB

#define SX1276_DIO1_N GPIO_PIN_10
#define SX1276_DIO1_G GPIOB

#define SX1276_DIO2_N GPIO_PIN_11
#define SX1276_DIO2_G GPIOB

#define SX1276_DIO3_N GPIO_PIN_12
#define SX1276_DIO3_G GPIOB

#define SX1276_DIO4_N GPIO_PIN_13
#define SX1276_DIO4_G GPIOB

#define SX1276_DIO5_N GPIO_PIN_14
#define SX1276_DIO5_G GPIOB


/******** SX1257 pins ***********/
// SPI6 for SX1257 management and WWVB amplifier config, but just define here
#define SX1257_MISO_N GPIO_PIN_12
#define SX1257_MISO_G GPIOG // AF5 for SPI6_MISO

#define SX1257_MOSI_N GPIO_PIN_7
#define SX1257_MOSI_G GPIOA // AF8 for SPI6_MOSI

#define SX1257_SCK_N GPIO_PIN_13
#define SX1257_SCK_G GPIOG // AF5 for SPI6_SCLK

#define SX1257_NSS_N GPIO_PIN_15
#define SX1257_NSS_G GPIOA

#define SX1257_RST_N GPIO_PIN_5
#define SX1257_RST_G GPIOJ

// SPI1_SCK
#define SX1257_CLK_OUT_SPI1_N GPIO_PIN_11
#define SX1257_CLK_OUT_SPI1_G GPIOG

// SPI2_SCK
#define SX1257_CLK_OUT_SPI2_N GPIO_PIN_1
#define SX1257_CLK_OUT_SPI2_G GPIOI

// SPI1_MOSI
#define SX1257_I_IN_N GPIO_PIN_7
#define SX1257_I_IN_G GPIOD

// SPI2_MOSI
#define SX1257_Q_IN_N GPIO_PIN_3
#define SX1257_Q_IN_G GPIOI

// SPI1_MISO
#define SX1257_I_OUT_N GPIO_PIN_6
#define SX1257_I_OUT_G GPIOA

// SPI2_MISO
#define SX1257_Q_OUT_N GPIO_PIN_2
#define SX1257_Q_OUT_G GPIOI


#define WWVB_AMP1_CS_N GPIO_PIN_12
#define WWVB_AMP1_CS_G GPIOJ

#define WWVB_AMP2_CS_N GPIO_PIN_13
#define WWVB_AMP2_CS_G GPIOJ

// RF switches on the board
#define WWVB_SMA_UFL_SEL_N GPIO_PIN_11
#define WWVB_SMA_UFL_SEL_G GPIOF

#define WWVB_AMPLIFIER_SEL0_N GPIO_PIN_14
#define WWVB_AMPLIFIER_SEL0_G GPIOJ

#define WWVB_AMPLIFIER_SEL1_N GPIO_PIN_15
#define WWVB_AMPLIFIER_SEL1_G GPIOJ

#define LORA_SMA_UFL_SEL_N GPIO_PIN_0
#define LORA_SMA_UFL_SEL_G GPIOG

#define SDR_TX_RX_SEL_N GPIO_PIN_11
#define SDR_TX_RX_SEL_G GPIOJ

#define LORA_LF_TXRX_SEL_N GPIO_PIN_0
#define LORA_LF_TXRX_SEL_G GPIOB

#define LORA_HF_TXRX_SEL_N GPIO_PIN_1
#define LORA_HF_TXRX_SEL_G GPIOB

#define LORA_LF_HF_SEL_N GPIO_PIN_2
#define LORA_LF_HF_SEL_G GPIOB




#define PIN_STRUCT(pin) \
  { pin##_N, pin##_G }




void wwvb_digital_write(int pin, bool val);
bool wwvb_digital_read(int pin);
void wwvb_gpio_pinmode(int pin, int dir);
void wwvb_m4_print_val(char * name, uint32_t val);
void wwvb_m4_print_bool(char * name, bool val);

#define SX1257_I_RX_DMA_STREAM DMA2_Stream7
#define SX1257_I_RX_DMA_STREAM_IRQ DMA2_Stream7_IRQn
#define SX1257_I_RX_DMA_STREAM_HANDLER DMA2_Stream7_IRQHandler

#define SX1257_I_TX_DMA_STREAM DMA2_Stream6
#define SX1257_I_TX_DMA_STREAM_IRQ DMA2_Stream6_IRQn
#define SX1257_I_TX_DMA_STREAM_HANDLER DMA2_Stream6_IRQHandler

#define SX1257_Q_RX_DMA_STREAM DMA2_Stream5
#define SX1257_Q_RX_DMA_STREAM_IRQ DMA2_Stream5_IRQn
#define SX1257_Q_RX_DMA_STREAM_HANDLER DMA2_Stream5_IRQHandler

#define SX1257_Q_TX_DMA_STREAM DMA2_Stream4
#define SX1257_Q_TX_DMA_STREAM_IRQ DMA2_Stream4_IRQn
#define SX1257_Q_TX_DMA_STREAM_HANDLER DMA2_Stream4_IRQHandler




/* Use of CMSIS compiler intrinsics for register exclusive access */
/* Atomic 32-bit register access macro to set one or several bits */
#define ATOMIC_SET_BIT(REG, BIT)                             \
  do {                                                       \
    uint32_t val;                                            \
    do {                                                     \
      val = __LDREXW((__IO uint32_t *)&(REG)) | (BIT);       \
    } while ((__STREXW(val,(__IO uint32_t *)&(REG))) != 0U); \
  } while(0)

/* Atomic 32-bit register access macro to clear one or several bits */
#define ATOMIC_CLEAR_BIT(REG, BIT)                           \
  do {                                                       \
    uint32_t val;                                            \
    do {                                                     \
      val = __LDREXW((__IO uint32_t *)&(REG)) & ~(BIT);      \
    } while ((__STREXW(val,(__IO uint32_t *)&(REG))) != 0U); \
  } while(0)

/* Atomic 32-bit register access macro to clear and set one or several bits */
#define ATOMIC_MODIFY_REG(REG, CLEARMSK, SETMASK)                          \
  do {                                                                     \
    uint32_t val;                                                          \
    do {                                                                   \
      val = (__LDREXW((__IO uint32_t *)&(REG)) & ~(CLEARMSK)) | (SETMASK); \
    } while ((__STREXW(val,(__IO uint32_t *)&(REG))) != 0U);               \
  } while(0)


// Fixing HAL SPI APIs to work for circular mode
// Default HAL_SPI_IRQHandler does not work for circular DMA mode!
void HAL_SPI_IRQHandler_CircFix(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef HAL_SPI_DMAPause_Fix(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef HAL_SPI_DMAResume_Fix(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef HAL_SPI_DMAStop_Fix(SPI_HandleTypeDef *hspi);




#endif