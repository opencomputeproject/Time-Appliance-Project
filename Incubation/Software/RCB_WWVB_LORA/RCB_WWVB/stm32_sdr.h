#ifndef STM32_SDR_H
#define STM32_SDR_H



#include "WWVB_Arduino.h"
#include "SX1257.h"
#include "SX1276_LORA.h"

/*
void HAL_SPI_RxCpltCallback(struct __DMA_HandleTypeDef * hdma);
void HAL_SPI_RxHalfCpltCallback(struct __DMA_HandleTypeDef * hdma);
void HAL_SPI_ErrorCallback(struct __DMA_HandleTypeDef * hdma);
*/

inline int16_t convert_spi_to_dfsdm(uint32_t val);
inline uint32_t calc_ampl(uint32_t i_val, uint32_t q_val);


void stm32_sdr_spi_init();
void stm32_sdr_spi_test();

void stm32_sdr_dfsdm_init();
void stm32_sdr_dfsdm_test();

void stm32_sdr_init();

void stm32_lora_test();






#endif