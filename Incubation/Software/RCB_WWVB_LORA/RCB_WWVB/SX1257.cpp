#include "SX1257.h"


#define REGMODE 0x0
#define TX_PA_DRIVER_ENABLE (1<<3)
#define TX_FRONTEND_PLL_ENABLE (1<<2)
#define RX_FRONTEND_PLL_ENABLE (1<<1)
#define RX_STANDBY_ENABLE (1)

#define REG_FRF_RX_MSB 0x1
#define REG_FRF_RX_MID 0x2
#define REG_FRF_RX_LSB 0x3
#define REG_FRF_TX_MSB 0x4
#define REG_FRF_TX_MID 0x5
#define REG_FRF_TX_LSB 0x6

#define REG_TX_GAIN 0x8
#define TX_GAIN_NEG9DB 0x0
#define TX_GAIN_NEG6DB 0x1
#define TX_GAIN_NEG3DB 0x2
#define TX_GAIN_MAX 0x3
#define TX_GAIN_SHIFT 4
#define TX_MIXER_GAIN_MASK 0xF

#define REG_TX_BW 0xA
#define TX_PLL_BW_75K 0x0
#define TX_PLL_BW_150K 0x1
#define TX_PLL_BW_225K 0x2
#define TX_PLL_BW_300K 0x3
#define TX_PLL_BW_SHIFT 5
#define TX_ANAFILT_BW_MASK 0x1f // check datasheet for formula

#define REG_TX_DAC_BW 0xB
#define TX_DAC_FIR_TAP_CNT_MASK 0x7

#define REG_RX_ANA_GAIN 0xC
#define RX_LNA_GAIN_SHIFT 5
#define RX_LNA_GAIN_MASK 0x7
#define RX_LNA_BASEBAND_GAIN_SHIFT 1
#define RX_LNA_BASEBAND_GAIN_MASK 0xF
#define RX_LNA_IMPEDANCE_50 (0<<0)

#define REG_RX_BW 0xD
#define RX_ADC_BW_100K_200K 0x2
#define RX_ADC_BW_200K_400K 0x5
#define RX_ADC_BW_GT400K 0x7
#define RX_ADC_BW_SHIFT 5
#define RX_ADC_BW_MASK 0x7
#define RX_ADC_TRIM_32M 0x6
#define RX_ADC_TRIM_36M 0x5
#define RX_ADC_TRIM_SHIFT 2
#define RX_ADC_TRIM_MASK 0x7
#define RX_ANALOG_FILT_BW_750K 0x0
#define RX_ANALOG_FILT_BW_500K 0x1
#define RX_ANALOG_FILT_BW_375K 0x2
#define RX_ANALOG_FILT_BW_250K 0x3


#define REG_RX_PLL_BW 0xE
#define RX_PLL_BW_75K 0x0
#define RX_PLL_BW_150K 0x1
#define RX_PLL_BW_225K 0x2
#define RX_PLL_BW_300K 0x3
#define RX_PLL_BW_SHIFT 1
#define RX_PLL_BW_MASK 0x3
#define RX_ADC_TEMP_EN 1

#define REG_DIO_MAPPING 0xF

#define REG_CLK_SELECT 0x10
#define DIG_LOOPBACK (1<<3)
#define RF_LOOPBACK (1<<2)
#define CLK_OUT_ENABLE (1<<1)
#define TX_DAC_CLK_SEL_XTAL (0<<0)
#define TX_DAC_CLK_SEL_CLK_IN (1<<0)

#define REG_MODE_STATUS 0x11
#define RX_PLL_LOCK (1<<1)
#define TX_PLL_LOCK (1<<0)

#define REG_LOW_BAT_THRESH 0x1A // useless for me




SX1257Class::SX1257Class()
{
  // overide Stream timeout value
  setTimeout(0);
  _ss = SX1257_NSS;
  _reset = SX1257_RST;
}



int SX1257Class::init() {

  
  // Init the GPIOs used just for control
  wwvb_gpio_pinmode(SX1257_NSS, OUTPUT);
  wwvb_gpio_pinmode(SX1257_RST, OUTPUT);

  wwvb_digital_write(SX1257_NSS, 1);
  wwvb_digital_write(SX1257_RST, 1);


  
  // Init the STM32 SPI6 interface
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // SPI6 SCK Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1257_SCK].GPIO_Pin;  
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI6;  // Alternate function for SPI6
  HAL_GPIO_Init(WWVB_Pins[SX1257_SCK].GPIO_Group, &GPIO_InitStruct);

  // SPI6 MISO Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1257_MISO].GPIO_Pin;  
  HAL_GPIO_Init(WWVB_Pins[SX1257_MISO].GPIO_Group, &GPIO_InitStruct);

  // SPI6 MOSI Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1257_MOSI].GPIO_Pin; 
  GPIO_InitStruct.Alternate = GPIO_AF8_SPI6; // this pin uses AF8 for some reason
  HAL_GPIO_Init(WWVB_Pins[SX1257_MOSI].GPIO_Group, &GPIO_InitStruct);

  _spi_mgmt.Instance = SPI6;
  _spi_mgmt.Init.Mode = SPI_MODE_MASTER;  // SPI mode (Master/Slave)
  _spi_mgmt.Init.Direction = SPI_DIRECTION_2LINES;  // Full duplex mode
  _spi_mgmt.Init.DataSize = SPI_DATASIZE_8BIT;  // 8-bit data frame format
  _spi_mgmt.Init.CLKPolarity = SPI_POLARITY_LOW;  // Clock polarity
  _spi_mgmt.Init.CLKPhase = SPI_PHASE_1EDGE;  // Clock phase
  _spi_mgmt.Init.NSS = SPI_NSS_SOFT;  // NSS signal is managed by software
  _spi_mgmt.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;  // Baud rate prescaler
  _spi_mgmt.Init.FirstBit = SPI_FIRSTBIT_MSB;  // Data is transmitted MSB first
  _spi_mgmt.Init.TIMode = SPI_TIMODE_DISABLE;  // Disable TI mode
  _spi_mgmt.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Disable CRC calculation
  _spi_mgmt.Init.CRCPolynomial = 7;  // Polynomial for CRC calculation

  if ( HAL_SPI_Init(&_spi_mgmt) != HAL_OK ) {
    Serial.println("FAILED TO INIT SX1257 MANAGEMENT SPI");
    return -1;
  }


  // Init the STM32 SPI1 interface, I Data

  // SPI1 SCK Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1257_CLK_OUT_SPI1].GPIO_Pin;  
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;  // Alternate function for SPI6
  HAL_GPIO_Init(WWVB_Pins[SX1257_CLK_OUT_SPI1].GPIO_Group, &GPIO_InitStruct);

  // SPI1 MISO Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1257_I_IN].GPIO_Pin;  
  HAL_GPIO_Init(WWVB_Pins[SX1257_I_IN].GPIO_Group, &GPIO_InitStruct);

  // SPI1 MOSI Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1257_I_OUT].GPIO_Pin; 
  HAL_GPIO_Init(WWVB_Pins[SX1257_I_OUT].GPIO_Group, &GPIO_InitStruct);

  _spi_I_Data.Instance = SPI1;
  _spi_I_Data.Init.Mode = SPI_MODE_SLAVE;  // SPI mode (Master/Slave)
  _spi_I_Data.Init.Direction = SPI_DIRECTION_2LINES;  // Full duplex mode
  _spi_I_Data.Init.DataSize = SPI_DATASIZE_8BIT;  // 8-bit data frame format
  _spi_I_Data.Init.CLKPolarity = SPI_POLARITY_LOW;  // Clock polarity
  _spi_I_Data.Init.CLKPhase = SPI_PHASE_1EDGE;  // Clock phase
  _spi_I_Data.Init.NSS = SPI_NSS_SOFT;  // NSS signal is managed by software
  _spi_I_Data.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;  // Baud rate prescaler
  _spi_I_Data.Init.FirstBit = SPI_FIRSTBIT_MSB;  // Data is transmitted MSB first
  _spi_I_Data.Init.TIMode = SPI_TIMODE_DISABLE;  // Disable TI mode
  _spi_I_Data.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Disable CRC calculation
  _spi_I_Data.Init.CRCPolynomial = 7;  // Polynomial for CRC calculation

  if ( HAL_SPI_Init(&_spi_I_Data) != HAL_OK ) {
    Serial.println("FAILED TO INIT SX1257 I Data SPI");
    return -1;
  }


  
  // Init the STM32 SPI2 interface, Q Data

  // SPI1 SCK Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1257_CLK_OUT_SPI2].GPIO_Pin;  
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;  // Alternate function for SPI6
  HAL_GPIO_Init(WWVB_Pins[SX1257_CLK_OUT_SPI2].GPIO_Group, &GPIO_InitStruct);

  // SPI1 MISO Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1257_Q_IN].GPIO_Pin;  
  HAL_GPIO_Init(WWVB_Pins[SX1257_Q_IN].GPIO_Group, &GPIO_InitStruct);

  // SPI1 MOSI Pin Configuration
  GPIO_InitStruct.Pin = WWVB_Pins[SX1257_Q_OUT].GPIO_Pin; 
  HAL_GPIO_Init(WWVB_Pins[SX1257_Q_OUT].GPIO_Group, &GPIO_InitStruct);

  _spi_Q_Data.Instance = SPI2;
  _spi_Q_Data.Init.Mode = SPI_MODE_SLAVE;  // SPI mode (Master/Slave)
  _spi_Q_Data.Init.Direction = SPI_DIRECTION_2LINES;  // Full duplex mode
  _spi_Q_Data.Init.DataSize = SPI_DATASIZE_8BIT;  // 8-bit data frame format
  _spi_Q_Data.Init.CLKPolarity = SPI_POLARITY_LOW;  // Clock polarity
  _spi_Q_Data.Init.CLKPhase = SPI_PHASE_1EDGE;  // Clock phase
  _spi_Q_Data.Init.NSS = SPI_NSS_SOFT;  // NSS signal is managed by software
  _spi_Q_Data.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;  // Baud rate prescaler
  _spi_Q_Data.Init.FirstBit = SPI_FIRSTBIT_MSB;  // Data is transmitted MSB first
  _spi_Q_Data.Init.TIMode = SPI_TIMODE_DISABLE;  // Disable TI mode
  _spi_Q_Data.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Disable CRC calculation
  _spi_Q_Data.Init.CRCPolynomial = 7;  // Polynomial for CRC calculation

  if ( HAL_SPI_Init(&_spi_Q_Data) != HAL_OK ) {
    Serial.println("FAILED TO INIT SX1257 I Data SPI");
    return -1;
  }

  // toggle the reset
  wwvb_digital_write(SX1257_RST, 0);
  delay(10);
  wwvb_digital_write(SX1257_RST, 1);
  delay(10);

  // write some basic registers that should never change for current board design
  writeRegister(REG_RX_ANA_GAIN, 0x3e); // 50 ohm LNA input impedance is key setting
  writeRegister(REG_RX_BW, (RX_ADC_BW_GT400K << RX_ADC_BW_SHIFT) + 
      (RX_ADC_TRIM_36M << RX_ADC_TRIM_SHIFT) + RX_ANALOG_FILT_BW_500K ); //36MHz is key, that's what PLL is giving
  writeRegister(REG_CLK_SELECT, CLK_OUT_ENABLE + TX_DAC_CLK_SEL_XTAL); // Need clock out and crystal path for TX

  isTransmitting = 0;
  isReceiving = 0;
}


int SX1257Class::set_tx_mode(bool tx_frontend_pll, bool tx_pa_driver) {
  uint8_t cur_reg = 0;
  cur_reg = readRegister(REGMODE);

  Serial.print("SX1257 Set TX frontend_pll=");
  Serial.print(tx_frontend_pll);
  Serial.print(" , pa_driver=");
  Serial.println(tx_pa_driver);

  if ( tx_frontend_pll ) cur_reg |= TX_FRONTEND_PLL_ENABLE;
  else cur_reg &= (~TX_FRONTEND_PLL_ENABLE);

  if ( tx_pa_driver ) cur_reg |= TX_PA_DRIVER_ENABLE;
  else cur_reg &= (~TX_PA_DRIVER_ENABLE);

  writeRegister(REGMODE, cur_reg);
  return 0;

}
int SX1257Class::set_rx_mode(bool rx_frontend_pll, bool standby) {
  uint8_t cur_reg = 0;
  cur_reg = readRegister(REGMODE);

  Serial.print("SX1257 Set RX frontend_pll=");
  Serial.print(rx_frontend_pll);
  Serial.print(" , standby_mode=");
  Serial.println(standby);

  if ( rx_frontend_pll ) cur_reg |= RX_FRONTEND_PLL_ENABLE;
  else cur_reg &= (~RX_FRONTEND_PLL_ENABLE);

  if ( standby ) cur_reg |= RX_STANDBY_ENABLE;
  else cur_reg &= (~RX_STANDBY_ENABLE);

  writeRegister(REGMODE, cur_reg);
  return 0;
}


// frequency calculation
// 36MHz = frequency resolution = 68.66455 Hz, 0xCB5555 = 915MHz
int SX1257Class::set_tx_freq(long frequency) {
  _txfrequency = frequency;

  uint32_t frf = (uint32_t)( ((float)frequency) / 68.66455);
  Serial.print("SX1257 Set TX Frequency 0x");
  Serial.println(frf, HEX);

  writeRegister(REG_FRF_TX_MSB, (uint8_t)(frf >> 16));
  writeRegister(REG_FRF_TX_MID, (uint8_t)(frf >> 8));
  writeRegister(REG_FRF_TX_LSB, (uint8_t)(frf >> 0));
  return 0;
}

int SX1257Class::set_rx_freq(long frequency) {
  _rxfrequency = frequency;
  
  uint32_t frf = (uint32_t)( ((float)frequency) / 68.66455);
  Serial.print("SX1257 Set RX Frequency 0x");
  Serial.println(frf, HEX);

  writeRegister(REG_FRF_RX_MSB, (uint8_t)(frf >> 16));
  writeRegister(REG_FRF_RX_MID, (uint8_t)(frf >> 8));
  writeRegister(REG_FRF_RX_LSB, (uint8_t)(frf >> 0));
  return 0;
}


int SX1257Class::set_tx_parameters(uint8_t dac_gain, uint8_t mixer_gain,
  uint8_t pll_bw, uint8_t analog_filter_bw,
  uint8_t tx_fir_ntaps) {

  writeRegister(REG_TX_GAIN, ( (dac_gain & TX_GAIN_MAX) << TX_GAIN_SHIFT) +
    (mixer_gain & TX_MIXER_GAIN_MASK) );
  writeRegister(REG_TX_BW, ( (pll_bw & TX_PLL_BW_300K) << TX_PLL_BW_SHIFT) +
    (analog_filter_bw & TX_ANAFILT_BW_MASK) );
  writeRegister(REG_TX_DAC_BW, tx_fir_ntaps & TX_DAC_FIR_TAP_CNT_MASK);

  return 0;
}

int SX1257Class::set_rx_parameters(uint8_t lna_gain, uint8_t baseband_gain,
  uint8_t adc_bw, uint8_t analog_filter_bw, 
  uint8_t pll_bw) {

  writeRegister(REG_RX_ANA_GAIN, ((lna_gain & RX_LNA_GAIN_MASK) << RX_LNA_GAIN_SHIFT) + 
    ((baseband_gain & RX_LNA_BASEBAND_GAIN_MASK) << RX_LNA_BASEBAND_GAIN_SHIFT) + RX_LNA_IMPEDANCE_50);
  
  writeRegister(REG_RX_BW, ((adc_bw & RX_ADC_BW_MASK) << RX_ADC_BW_SHIFT) + 
    (RX_ADC_TRIM_36M << RX_ADC_TRIM_SHIFT) + (analog_filter_bw & RX_ANALOG_FILT_BW_250K)  );

  writeRegister(REG_RX_PLL_BW, ((pll_bw & RX_PLL_BW_MASK) << RX_PLL_BW_SHIFT) ); // always disable temperature

}

int SX1257Class::get_status(bool * rx_pll_lock, bool * tx_pll_lock) {
  uint8_t val = 0;
  val = readRegister(REG_MODE_STATUS);
  if ( rx_pll_lock ) {
    if ( val & RX_PLL_LOCK ) {
      *rx_pll_lock = 1;
    } else {
      *rx_pll_lock = 0;
    }
  }
  if ( tx_pll_lock ){
    if ( val & TX_PLL_LOCK ) {
      *tx_pll_lock = 1;
    } else {
      *rx_pll_lock = 0;
    }
  }
  return 0;
}

void SX1257Class::dumpRegisters(Stream& out) {
  for (int i = 0; i < 0x1a; i++) {
    out.print("SX1257 Register 0x");
    out.print(i, HEX);
    out.print(": 0x");
    out.println(readRegister(i), HEX);
  }
}


uint8_t SX1257Class::readRegister(uint8_t address) {
  return singleTransfer(address & 0x7f, 0x00);
}
void SX1257Class::writeRegister(uint8_t address, uint8_t value) {
  singleTransfer(address | 0x80, value);
}

uint8_t SX1257Class::singleTransfer(uint8_t address, uint8_t value) {

  uint8_t response;
  wwvb_digital_write(_ss, LOW);

  HAL_SPI_TransmitReceive(&_spi_mgmt, &address, &response, sizeof(address), HAL_MAX_DELAY);  //ignore receive data

  HAL_SPI_TransmitReceive(&_spi_mgmt, &value, &response, sizeof(value), HAL_MAX_DELAY);

  wwvb_digital_write(_ss, HIGH);

  return response;

}





/************* Stream class stuff ******/
size_t SX1257Class::write(uint8_t byte) {
  return 0;
}
size_t SX1257Class::write(const uint8_t *buffer, size_t size) {
  return 0;
}

  // from Stream
int SX1257Class::available() {
  return 0;
}

int SX1257Class::read() {
  return 0;
}
int SX1257Class::peek() {
  return 0;
}
void SX1257Class::flush() {
  return;
}

