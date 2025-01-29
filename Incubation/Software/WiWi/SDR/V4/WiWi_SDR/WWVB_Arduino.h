#ifndef WWVB_ARDUINO_H
#define WWVB_ARDUINO_H


#include "LinkedList_git.h"

#include <stdint.h>
#include <math.h>
#include <stm32h7xx_hal.h>
#include <stm32h7xx_hal_gpio.h>
#include <Arduino.h>
#include <stm32h7xx_hal_exti.h>

#include <stm32h7xx_hal_spi.h>
#include <stm32h7xx_hal_dma.h>
#include <stm32h7xx.h>

#include <mbed.h>
#include <rtos.h>
//#include <RPC.h>
// Low level library to make it easier to use WWVB board

//using namespace mbed;
//using namespace rtos;
//using namespace std::chrono_literals;

extern char print_buffer[10000];




// just sequential, easy addressable
enum WWVB_Pin_Name {
	PLL_SDA,
	PLL_SCL,
	PLL_RST,

  SX1276_DIO0,
  SX1276_SCK,
  SX1276_MISO,
  SX1276_MOSI,  
  SX1276_NSS,
  SX1276_RST,

  LORA_LF_TXRX_SEL,
  LORA_HF_TXRX_SEL,
  LORA_LF_HF_SEL,

  AT86_SPI_SEL,
  AT86_IRQ,
  AT86_RST,
  SDR_WIFI_ATTEN_SEL,
  SDR_SUBG_ATTEN_SEL,

  ESP32_RST,

  QSPI_FPGA_SCLK,
  QSPI_FPGA_MOSI,
  QSPI_FPGA_WP,
  QSPI_FPGA_MISO,
  QSPI_FPGA_RESET,
  QSPI_FPGA_CS,
  FPGA_PROGRAMN,
  FPGA_INITN,
  FPGA_DONE,
  STM_FPGA_SPARE1,
  STM_FPGA_SPARE2,
  STM_FPGA_SPARE3,
  STM_FPGA_SPARE4,
  STM_FPGA_SPARE5,
  STM_FPGA_SPARE6,
  FPGA_SDA,
  FPGA_SCL,
  
  SPI1_SCK,
  SPI1_MOSI,
  SPI1_MISO,
  SPI2_SCK,
  SPI2_MOSI,
  SPI2_MISO,
  SPI3_SCK,
  SPI3_MOSI,
  SPI3_MISO,
  SPI4_SCK,
  SPI4_MOSI,
  SPI4_MISO,
  

  NUM_PINS
};

typedef struct WWVB_Pin_Config {
	uint16_t GPIO_Pin;
	GPIO_TypeDef * GPIO_Group;
} WWVB_Pin;

extern WWVB_Pin WWVB_Pins[];

/******* PLL pins ********/
#define PLL_SDA_N GPIO_PIN_7
#define PLL_SDA_G GPIOB

#define PLL_SCL_N GPIO_PIN_8
#define PLL_SCL_G GPIOB

#define PLL_RST_N GPIO_PIN_1
#define PLL_RST_G GPIOD


/****** SX1276 pins ******/
#define SX1276_DIO0_N GPIO_PIN_7
#define SX1276_DIO0_G GPIOJ

#define SX1276_SCK_N GPIO_PIN_0
#define SX1276_SCK_G GPIOK

#define SX1276_MISO_N GPIO_PIN_8
#define SX1276_MISO_G GPIOF

#define SX1276_MOSI_N GPIO_PIN_9
#define SX1276_MOSI_G GPIOF

#define SX1276_NSS_N GPIO_PIN_1
#define SX1276_NSS_G GPIOK

#define SX1276_RST_N GPIO_PIN_4
#define SX1276_RST_G GPIOB

/***** Antenna select ******/
#define LORA_LF_TXRX_SEL_N GPIO_PIN_3
#define LORA_LF_TXRX_SEL_G GPIOK

#define LORA_HF_TXRX_SEL_N GPIO_PIN_4 
#define LORA_HF_TXRX_SEL_G GPIOK

#define LORA_LF_HF_SEL_N GPIO_PIN_6
#define LORA_LF_HF_SEL_G GPIOK


/***** AT86RF215IQ specific pins *******/
#define AT86_SPI_SEL_N GPIO_PIN_10
#define AT86_SPI_SEL_G GPIOH

#define AT86_IRQ_N GPIO_PIN_12
#define AT86_IRQ_G GPIOH

#define AT86_RST_N GPIO_PIN_11
#define AT86_RST_G GPIOH


#define SDR_WIFI_ATTEN_SEL_N GPIO_PIN_9
#define SDR_WIFI_ATTEN_SEL_G GPIOH

#define SDR_SUBG_ATTEN_SEL_N GPIO_PIN_0
#define SDR_SUBG_ATTEN_SEL_G GPIOI



/******* ESP32 specific pins *******/
#define ESP32_RST_N GPIO_PIN_0
#define ESP32_RST_G GPIOD


/***** ECP5 FPGA pins ******/
#define QSPI_FPGA_SCLK_N GPIO_PIN_10
#define QSPI_FPGA_SCLK_G GPIOF

#define QSPI_FPGA_MOSI_N GPIO_PIN_11
#define QSPI_FPGA_MOSI_G GPIOD

#define QSPI_FPGA_WP_N GPIO_PIN_7
#define QSPI_FPGA_WP_G GPIOF

#define QSPI_FPGA_MISO_N GPIO_PIN_12
#define QSPI_FPGA_MISO_G GPIOD

#define QSPI_FPGA_RESET_N GPIO_PIN_13
#define QSPI_FPGA_RESET_G GPIOD

#define QSPI_FPGA_CS_N GPIO_PIN_6
#define QSPI_FPGA_CS_G GPIOG

#define FPGA_PROGRAMN_N GPIO_PIN_13
#define FPGA_PROGRAMN_G GPIOA

#define FPGA_INITN_N GPIO_PIN_5
#define FPGA_INITN_G GPIOD

#define FPGA_DONE_N GPIO_PIN_3
#define FPGA_DONE_G GPIOC

#define STM_FPGA_SPARE1_N GPIO_PIN_2
#define STM_FPGA_SPARE1_G GPIOK

#define STM_FPGA_SPARE2_N GPIO_PIN_15
#define STM_FPGA_SPARE2_G GPIOH

#define STM_FPGA_SPARE3_N GPIO_PIN_14
#define STM_FPGA_SPARE3_G GPIOH

#define STM_FPGA_SPARE4_N GPIO_PIN_2
#define STM_FPGA_SPARE4_G GPIOG

#define STM_FPGA_SPARE5_N GPIO_PIN_6
#define STM_FPGA_SPARE5_G GPIOH

#define STM_FPGA_SPARE6_N GPIO_PIN_7
#define STM_FPGA_SPARE6_G GPIOH

#define FPGA_SDA_N GPIO_PIN_7
#define FPGA_SDA_G GPIOI

#define FPGA_SCL_N GPIO_PIN_2
#define FPGA_SCL_G GPIOI

#define SPI1_SCK_N GPIO_PIN_3
#define SPI1_SCK_G GPIOB

#define SPI1_MISO_N GPIO_PIN_6
#define SPI1_MISO_G GPIOA

#define SPI1_MOSI_N GPIO_PIN_7
#define SPI1_MOSI_G GPIOD

#define SPI2_SCK_N GPIO_PIN_1
#define SPI2_SCK_G GPIOI

#define SPI2_MISO_N GPIO_PIN_15
#define SPI2_MISO_G GPIOB

#define SPI2_MOSI_N GPIO_PIN_3
#define SPI2_MOSI_G GPIOI

#define SPI3_SCK_N GPIO_PIN_10
#define SPI3_SCK_G GPIOC

#define SPI3_MISO_N GPIO_PIN_11
#define SPI3_MISO_G GPIOC

#define SPI3_MOSI_N GPIO_PIN_6
#define SPI3_MOSI_G GPIOD

#define SPI4_SCK_N GPIO_PIN_2
#define SPI4_SCK_G GPIOE

#define SPI4_MISO_N GPIO_PIN_5
#define SPI4_MISO_G GPIOE

#define SPI4_MOSI_N GPIO_PIN_6
#define SPI4_MOSI_G GPIOE


#define PIN_STRUCT(pin) \
  { pin##_N, pin##_G }




void wwvb_digital_write(int pin, bool val);
bool wwvb_digital_read(int pin);
void wwvb_gpio_pinmode(int pin, int dir);
void wwvb_gpio_pinmode_pullup(int pin, int dir);
void wwvb_m4_print_val(char * name, uint32_t val);
void wwvb_m4_print_bool(char * name, bool val);

#define SUBG_I_RX_DMA_STREAM DMA2_Stream7
#define SUBG_Q_RX_DMA_STREAM DMA2_Stream6
#define WIFI_I_RX_DMA_STREAM DMA2_Stream5
#define WIFI_Q_RX_DMA_STREAM DMA2_Stream4

#define ESP_UART_TX_DMA_STREAM DMA2_Stream3
#define ESP_UART_TX_DMA_STREAM_IRQ DMA2_Stream3_IRQn
#define ESP_UART_TX_DMA_STREAM_HANDLER DMA2_Stream3_IRQHandler

#define ESP_UART_RX_DMA_STREAM DMA2_Stream2
#define ESP_UART_RX_DMA_STREAM_IRQ DMA2_Stream2_IRQn
#define ESP_UART_RX_DMA_STREAM_HANDLER DMA2_Stream2_IRQHandler

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



#define SRAM_SIZE 131072 // 131072 = max size for sram1 in bytes 
#define BUFFER_SIZE (SRAM_SIZE/4)  // each data entry is 2 bytes, and split between I and Q
//#define BUFFER_SIZE 10000
//#define FILTERED_BUFFER_SIZE BUFFER_SIZE/4

typedef volatile struct sram1_data_struct_name {
  int16_t subg_I_data[BUFFER_SIZE]; // int16_t from FPGA
  int16_t subg_Q_data[BUFFER_SIZE];
} sram1_data_struct;

typedef volatile struct sram2_data_struct_name {
  int16_t wifi_I_data[BUFFER_SIZE]; // int16_t from FPGA
  int16_t wifi_Q_data[BUFFER_SIZE];
} sram2_data_struct;


extern sram1_data_struct * sram1_data;
extern sram2_data_struct * sram2_data;
// sram3 used by ethernet


#define DMA_PAUSED 0
#define DMA_RUNNING 1
#define DMA_STOPPED 2


void init_sram2_nocache();

unsigned int countSetBits(unsigned char n);

// Fixing HAL SPI APIs to work for circular mode
// Default HAL_SPI_IRQHandler does not work for circular DMA mode!
void HAL_SPI_IRQHandler_CircFix(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef HAL_SPI_DMAPause_Fix(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef HAL_SPI_DMAResume_Fix(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef HAL_SPI_DMAStop_Fix(SPI_HandleTypeDef *hspi);


extern "C" {
void __attribute__((weak)) SPI_DMAHalfReceiveCplt(DMA_HandleTypeDef *hdma);
void __attribute__((weak)) SPI_DMAReceiveCplt(DMA_HandleTypeDef *hdma);
void __attribute__((weak)) SPI_DMAError(DMA_HandleTypeDef *hdma);
}

#define SPI_2LINES_RX(__HANDLE__) MODIFY_REG((__HANDLE__)->Instance->CFG2, SPI_CFG2_COMM, SPI_CFG2_COMM_1)
// pretty much same as normal API, but doesn't enable SPI peripheral at the end 
HAL_StatusTypeDef HAL_SPI_Receive_DMA_NoStart_NoInterrupt(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_Receive_DMA_NoStart_HackWriteMDMA(SPI_HandleTypeDef *hspi, uint32_t * mdma_reg, uint32_t * mdma_val_to_write);

int32_t extend_sign_24bit(uint32_t value);
int countOneBits(uint32_t n);
int bitDifference(uint32_t value);

uint32_t htonl(uint32_t hostlong);



#endif