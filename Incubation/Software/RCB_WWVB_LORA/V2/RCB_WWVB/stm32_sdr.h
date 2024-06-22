#ifndef STM32_SDR_H
#define STM32_SDR_H



#include "WWVB_Arduino.h"
#include "SX1257.h"
#include "SX1276_LORA.h"
#include "WiWi_Data.h"
#include "ICE40.h"

inline int16_t convert_spi_to_dfsdm(uint32_t val);
inline uint32_t calc_ampl(uint32_t i_val, uint32_t q_val);


void stm32_sdr_spi_init();
void stm32_sdr_spi_test();
void stop_spi_dma();

void print_spi_registers(char * name, SPI_HandleTypeDef * hspi);
void print_dma_registers(char * name, DMA_Stream_TypeDef * hdma);

void stm32_sdr_init();

void stm32_lora_test();


/********** More top level APIs ********/
bool check_lora_done();
void clear_lora_done();
void check_start_lora_rx();
void force_restart_lora_rx();
void get_lora_iq_pointers(int * last_I, int * last_Q);
void dump_lora_iq_from_oldest() ;
void compute_phase_from_lora_iq(phaseUnion * phi);



#endif