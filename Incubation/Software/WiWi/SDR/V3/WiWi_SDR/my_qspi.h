#ifndef MY_QSPI_H
#define MY_QSPI_H

#include "menu_cli.h"
#include <Arduino.h>
#include "WWVB_Arduino.h"

#include "QSPIFBlockDevice.h"
#include "MBRBlockDevice.h"

typedef struct {
  uint32_t FlashSize;          /*!< Size of the flash */
  uint32_t EraseSectorSize;    /*!< Size of sectors for the erase operation */
  uint32_t EraseSectorsNumber; /*!< Number of sectors for the erase operation */
  uint32_t ProgPageSize;       /*!< Size of pages for the program operation */
  uint32_t ProgPagesNumber;    /*!< Number of pages for the program operation */
} BSP_QSPI_Info_t;

#define MY_QSPI_FLASH_SIZE                  0x8000000 /* 2 * 512 MBits => 2 * 64MBytes => 128MBytes*/
#define MY_QSPI_SECTOR_SIZE                 0x10000   /* 2 * 1024 sectors of 64KBytes */
#define MY_QSPI_SUBSECTOR_SIZE              0x1000    /* 2 * 16384 subsectors of 4kBytes */
#define MY_QSPI_PAGE_SIZE                   0x100     /* 2 * 262144 pages of 256 bytes */

void init_my_qspi();

/************ Top level init and CLI ***********/

void init_my_qspi_cli();


#endif