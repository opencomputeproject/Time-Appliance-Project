



#ifndef _DPLL_AD9546_H_
#define _DPLL_AD9546_H_

#include <Arduino.h>
//#include <PID_v2.h> // can use this library for PID eventually but not sure if itll work


#define MILLI_TO_PICO(x) (((int64_t)x)*((int64_t)1000)*((int64_t)1000)*((int64_t)1000))
#define MICRO_TO_PICO(x) (((int64_t)x)*((int64_t)1000)*((int64_t)1000))
#define SECONDS_TO_PICO(x) (((int64_t)x)*((int64_t)1000)*((int64_t)1000)*((int64_t)1000)*((int64_t)1000))

// for a ratio of two frequencies, this is the PPM they're off, just multiply by 1M
#define FREQ_PPM(x) ((1.0-x)*1000.0*1000.0)  
#define FREQ_PPB(x) ((1.0-x)*1000.0*1000.0*1000.0)

// given a PPB , like 1 , give the value to multiply a frequency value by
// example. 1 PPB = increase frequency by 1PPB , multiply by 1 + 1e-9
//    
#define PPB_TO_FREQ_RATIO(x) (1.0+(x * 1000.0 * 1000.0 * 1000.0))

#define INT_PPB(x) (x / (1000*1000*1000))



// BE CAREFUL WITH THIS MAX VALUE, Decawave or DPLL (probably DPLL) gets unstable if you adjust too quickly
// ideally the delay after adjusting the DPLL should be proportional to the amount adjusted
// or should be read back by a status register in the DPLL
#define MAX_FREQ_ADJ_PPB 1000.0
#define MAX_FREQ_ADJ_PPB_INT ((uint64_t)MAX_FREQ_ADJ_PPB)

#define MAX_PHASE_ADJ_MS ((int64_t)200)
#define MAX_PHASE_ADJ_PS ((int64_t)(MAX_PHASE_ADJ_MS * ((int64_t)1000) * ((int64_t)1000) * ((int64_t)1000)))

#define FREQ_ADJ_KP 0.7
#define PHASE_ADJ_KP 0.7

#define SERIAL_PORT_CONFIG_0 0x0
#define SERIAL_PORT_CONFIG_1 0x1

#define CHIP_TYPE 0x3
#define DEVICE_CODE_0 0x4
#define DEVICE_CODE_1 0x5
#define DEVICE_CODE_2 0x6
#define SPI_VERSION 0xB
#define VENDOR_ID_0 0xC
#define VENDOR_ID_1 0xD

#define ADDRLOOP_IOUPDATE 0xF
#define IO_UPDATE_FLAG 0x1


#define ADDRLOOP_LENGTH 0x10

#define SCRATCH_0 0x20
#define SCRATCH_1 0x21
#define SCRATCH_2 0x22
#define SCRATCH_3 0x23

#define MX_PINMODE_0 0x100
#define MX_PINMODE_1 0x101

#define M0_PINSTATUS_CONTROL 0x102
#define M1_PINSTATUS_CONTROL 0x103
#define M2_PINSTATUS_CONTROL 0x104
#define M3_PINSTATUS_CONTROL 0x105
#define M4_PINSTATUS_CONTROL 0x106
#define M5_PINSTATUS_CONTROL 0x107
#define M6_PINSTATUS_CONTROL 0x108

#define SERIAL_MPIN_DRVCURRENT 0x109

#define WATCHDOG_TIMER0 0x10a
#define WATCHDOG_TIMER1 0x10b

#define IRQ_ENABLE_0 0x10c
#define IRQ_ENABLE_1 0x10d
#define IRQ_ENABLE_2 0x10e
#define IRQ_ENABLE_3 0x10f
#define IRQ_ENABLE_4 0x110
#define IRQ_ENABLE_5 0x111
#define IRQ_ENABLE_6 0x112
#define IRQ_ENABLE_7 0x113
#define IRQ_ENABLE_8 0x114
#define IRQ_ENABLE_9 0x115
#define IRQ_ENABLE_10 0x116
#define IRQ_ENABLE_11 0x117
#define IRQ_ENABLE_12 0x118
#define IRQ_ENABLE_13 0x119
#define IRQ_ENABLE_14 0x11a
#define IRQ_ENABLE_15 0x11b
#define IRQ_ENABLE_16 0x11c
#define IRQ_ENABLE_17 0x11d
#define IRQ_ENABLE_18 0x11e
#define IRQ_ENABLE_19 0x11f

#define M0_PINFUNCTION 0x182
#define M1_PINFUNCTION 0x183
#define M2_PINFUNCTION 0x184
#define M3_PINFUNCTION 0x185
#define M4_PINFUNCTION 0x186
#define M5_PINFUNCTION 0x187
#define M6_PINFUNCTION 0x188

/* Multi-byte register values, list start and size, address zero is LSB, [7:0] */


/* AUXNCO0, used for my purpose as the adjustable source */
#define AUXNCO0_CENTERFREQ 0x2800
#define AUXNCO0_CENTERFREQ_SIZE 7

#define AUXNCO0_FREQOFFSET 0x2807
#define AUXNCO0_FREQOFFSET_SIZE 4

#define AUXNCO0_ELAPSEDTYPE 0x280f

// delta rate in the GUI
#define AUXNCO0_PHASESLEW_LIMIT 0x2810
#define AUXNCO0_PHASESLEW_LIMIT_SIZE 4

// delta t in the GUI
#define AUXNCO0_PHASEOFFSET 0x2814
#define AUXNCO0_PHASEOFFSET_SIZE 5

// delta ui in the GUI
#define AUXNCO0_UIADJUST 0x2819
#define AUXNCO0_UIADJUST_SIZE 5

#define AUXNCO0_PULSEWIDTH 0x281e


#define Q0B_DIV_RATIO 0x1112
#define Q0B_DIV_RATIO_SIZE 5


#define PLL0_STATUS_0 0x3100
#define APLL0_DONE_CAL (1<<5)
#define APLL0_BUSY_CAL (1<<4)
#define APLL0_PHASE_LOCKED (1<<3)
#define DPLL0_FREQ_LOCKED (1<<2)
#define DPLL0_PHASE_LOCKED (1<<1)
#define PLL0_LOCKED (1<<0)


void pinStr( uint32_t ulPin, unsigned strength);
void dpll_init(const PROGMEM uint16_t regcount, const PROGMEM uint16_t regs[],const PROGMEM uint8_t vals[] );

byte dpll_read_register(int addr);

void dpll_adjust_phase_picoseconds(int64_t picoseconds);

void dpll_io_update();
bool dpll_adjust_error();


#endif
