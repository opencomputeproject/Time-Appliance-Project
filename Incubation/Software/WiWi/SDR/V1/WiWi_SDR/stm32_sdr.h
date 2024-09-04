#ifndef STM32_SDR_H
#define STM32_SDR_H



#include "WWVB_Arduino.h"
#include "SX1257.h"
#include "SX1276_LORA.h"
#include "WiWi_Data.h"
#include "ICE40.h"
#include "arm_math.h" // Arduino CMSIS DSP library for signal processing 

inline int16_t convert_spi_to_dfsdm(uint32_t val);
inline uint32_t calc_ampl(uint32_t i_val, uint32_t q_val);


void stm32_sdr_spi_init();

//void stop_spi_dma();

void print_spi_registers(char * name, SPI_HandleTypeDef * hspi);
void print_dma_registers(char * name, DMA_Stream_TypeDef * hdma);

void stm32_sdr_init();


/********** More top level APIs ********/


void sdr_iq_init();

void switch_lora_to_rx();
void zero_iq_buffers();
bool check_lora_done();
void clear_lora_done();
void get_lora_iq_pointers(int * last_I, int * last_Q);
void dump_lora_iq_from_oldest() ;
void dump_iq_buffer_straight();
void dump_lora_iq_from_oldest_count(int count) ;
void dump_lora_iq_from_oldest_start_count(int start, int count);
void compute_phase_from_lora_iq(phaseUnion * phi);
void send_packet(uint8_t * pkt_data, int pktlen, phaseUnion * phase, uint32_t * timestamp);
bool is_receive_packet_available();
bool receive_lora_packet(uint8_t * pkt_data, int * pktlen, phaseUnion * phase);



/******** Sanity tests **********/

void test_dio0_interrupt();
void test_ice40_stream_enable_disable();
void test_ice40_reset();
void test_ice40_fixed_pattern(); // needs special FPGA image

#endif