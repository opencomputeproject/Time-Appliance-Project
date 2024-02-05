#include "WWVB_Arduino.h"


WWVB_Pin WWVB_Pins[] = {
	PIN_STRUCT(WLED_RED),
	PIN_STRUCT(WLED_GREEN),
	PIN_STRUCT(WLED_BLUE),
  
	PIN_STRUCT(PLL_SDA),
	PIN_STRUCT(PLL_SCL),
	PIN_STRUCT(PLL_RST),

  PIN_STRUCT(SIT5501_SDA),
  PIN_STRUCT(SIT5501_SCL),

  PIN_STRUCT(SX1276_MISO),
  PIN_STRUCT(SX1276_MOSI),
  PIN_STRUCT(SX1276_SCK),
  PIN_STRUCT(SX1276_NSS),
  PIN_STRUCT(SX1276_RST),
  PIN_STRUCT(SX1276_DIO0),
  PIN_STRUCT(SX1276_DIO1),
  PIN_STRUCT(SX1276_DIO2),
  PIN_STRUCT(SX1276_DIO3),
  PIN_STRUCT(SX1276_DIO4),
  PIN_STRUCT(SX1276_DIO5),

  // SX1257 management SPI
  PIN_STRUCT(SX1257_MISO),
  PIN_STRUCT(SX1257_MOSI),
  PIN_STRUCT(SX1257_SCK),
  PIN_STRUCT(SX1257_NSS),
  PIN_STRUCT(SX1257_RST),
  //SX1257 data path SPI 1 & 2
  PIN_STRUCT(SX1257_CLK_OUT_SPI1),
  PIN_STRUCT(SX1257_CLK_OUT_SPI2),
  PIN_STRUCT(SX1257_I_IN),
  PIN_STRUCT(SX1257_Q_IN),
  PIN_STRUCT(SX1257_I_OUT),
  PIN_STRUCT(SX1257_Q_OUT),

  PIN_STRUCT(WWVB_AMP1_CS),
  PIN_STRUCT(WWVB_AMP2_CS),

  PIN_STRUCT(WWVB_SMA_UFL_SEL),
  PIN_STRUCT(WWVB_AMPLIFIER_SEL0),
  PIN_STRUCT(WWVB_AMPLIFIER_SEL1),

  PIN_STRUCT(LORA_SMA_UFL_SEL),
  PIN_STRUCT(SDR_TX_RX_SEL),
  PIN_STRUCT(LORA_LF_TXRX_SEL),
  PIN_STRUCT(LORA_HF_TXRX_SEL),
  PIN_STRUCT(LORA_LF_HF_SEL),
};




static GPIO_InitTypeDef wwvb_gpio_init;

void wwvb_digital_write(int pin, bool val) {
  if ( val ) {
    HAL_GPIO_WritePin( WWVB_Pins[pin].GPIO_Group , 
      WWVB_Pins[pin].GPIO_Pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin( WWVB_Pins[pin].GPIO_Group , 
      WWVB_Pins[pin].GPIO_Pin, GPIO_PIN_RESET);
  }

}

bool wwvb_digital_read(int pin) {
	return (bool) HAL_GPIO_ReadPin( WWVB_Pins[pin].GPIO_Group ,
		WWVB_Pins[pin].GPIO_Pin );
}

void wwvb_gpio_pinmode(int pin, int dir) {
	wwvb_gpio_init.Pin = WWVB_Pins[pin].GPIO_Pin;
	if ( dir == OUTPUT ) {
		wwvb_gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	} else {
		wwvb_gpio_init.Mode = GPIO_MODE_INPUT;
	}
	wwvb_gpio_init.Pull = GPIO_NOPULL;
	wwvb_gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	wwvb_gpio_init.Alternate = 0x0;
	HAL_GPIO_Init(WWVB_Pins[pin].GPIO_Group , &wwvb_gpio_init);	
}