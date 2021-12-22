/*
 * TestApp.c
 *
 *  Created on: 19.10.2020
 *      Author: thomas
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "nettimelogic.h"


//#define pci_base        0x90000000
#define pci_addr_range  0x02000000


#define image_verion_addr 0x00020000

#define gpio_sel_addr 0x00130000
#define gpio_in 0x0
#define gpio_out 0x8

int main(int argc, char**argv) {
    uint32_t read_result;
    uint32_t write_value;
    
    uint32_t seconds;
    uint32_t nanoseconds;
    
    void *pcie_base, *virt_addr;
    unsigned page_size, mapped_size, offset_in_page;
    off_t target;
    uint32_t width = 32;
    int fd;
    
    if (argc != 2) {
        printf("Missing PCIe Base Address: e.g. ./TestApp 0xA0000000\n");
    exit(-1);
    }
    if (strlen(argv[1]) != 10) {
        printf("PCIe Base Address must have following format: ./TestApp 0xA0000000\n");
    exit(-1);
    }
    
    printf("PCIe Base Address is set to 0x%X \n", (int)strtol(argv[1], NULL, 0));
    target = (off_t)strtol(argv[1], NULL, 0);
    
    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) perror("/dev/mem open failed");;
    mapped_size = page_size = getpagesize();
    //printf("page_size = 0x%X\n", (unsigned long)mapped_size);
    
    offset_in_page = (unsigned)target & (page_size - 1);
    //printf("offset_in_page = 0x%X\n", (unsigned long)offset_in_page);


    if (offset_in_page + width > page_size) {
        /* This access spans pages.
        * Must map two pages to make it possible: */
        mapped_size *= 2;
    }
    mapped_size = pci_addr_range;

    pcie_base = mmap(0, mapped_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, target & ~(off_t)(page_size - 1));

    if (pcie_base == MAP_FAILED) {
        perror("Could not mmap");
        return 1;
    }
    
    //Print overall Versions
    virt_addr = (char*)pcie_base + image_verion_addr;
    read_result = *(volatile uint32_t*)virt_addr;
    printf("FPGA Image Version = 0x%X\n", (unsigned int)read_result);
    
    
    //Print all Core Versions
    virt_addr = (char*)pcie_base + NTL_CLK_REGSET_BASE+NTL_CLK_CLKVERSION_REG;
    read_result = *(volatile uint32_t*)virt_addr;
    printf("Clock IP Core Version = 0x%X\n", (unsigned int)read_result);
    
    virt_addr = (char*)pcie_base + NTL_STS1_REGSET_BASE+NTL_STS_VERSION_REG;
    read_result = *(volatile uint32_t*)virt_addr;
    printf("Signal TS IP Core Version = 0x%X\n", (unsigned int)read_result);
    
    virt_addr = (char*)pcie_base + NTL_STS2_REGSET_BASE+NTL_STS_VERSION_REG;
    read_result = *(volatile uint32_t*)virt_addr;
    printf("Signal TS IP Core Version = 0x%X\n", (unsigned int)read_result);
    
    virt_addr = (char*)pcie_base + NTL_PPSM_REGSET_BASE+NTL_PPSM_VERSION_REG;
    read_result = *(volatile uint32_t*)virt_addr;
    printf("PPS Master IP Core Version = 0x%X\n", (unsigned int)read_result);
    
    virt_addr = (char*)pcie_base + NTL_PPSS_REGSET_BASE+NTL_PPSS_VERSION_REG;
    read_result = *(volatile uint32_t*)virt_addr;
    printf("PPS Slave IP Core Version = 0x%X\n", (unsigned int)read_result);
    
    virt_addr = (char*)pcie_base + NTL_TOD_REGSET_BASE+NTL_TOD_VERSION_REG;
    read_result = *(volatile uint32_t*)virt_addr;
    printf("TOD Slave IP Core Version = 0x%X\n", (unsigned int)read_result);
    
    
   // Check Clock status 
    virt_addr = (char*)pcie_base + NTL_CLK_REGSET_BASE+NTL_CLK_CLKCONTROL_REG;
    read_result = *(volatile uint32_t*)virt_addr;
    
    if(read_result&&NTL_CLK_CONTROL_ENABLE_BIT != NTL_CLK_CONTROL_ENABLE_BIT) {
         printf("Clock is not enabled \n\r");
    }
    
    virt_addr = (char*)pcie_base + NTL_CLK_REGSET_BASE+NTL_CLK_CLKSELECT_REG;
    read_result = *(volatile uint32_t*)virt_addr;
    switch((read_result>>16)&0xff){
        case NTL_CLK_SELECT_NONE:   printf("Selected Clk Source is: NONE \n"); break;
        case NTL_CLK_SELECT_TOD:    printf("Selected Clk Source is: TOD \n"); break;
        case NTL_CLK_SELECT_IRIG:   printf("Selected Clk Source is: IRIG \n"); break;
        case NTL_CLK_SELECT_PPS:    printf("Selected Clk Source is: PPS\n"); break;
        case NTL_CLK_SELECT_PTP:    printf("Selected Clk Source is: PTP \n"); break;
        case NTL_CLK_SELECT_RTC:    printf("Selected Clk Source is: RTC \n"); break;
        case NTL_CLK_SELECT_DCF:    printf("Selected Clk Source is: DCF \n"); break;
        case NTL_CLK_SELECT_REGS:   printf("Selected Clk Source is: REGS \n"); break;
        case NTL_CLK_SELECT_EXT:    printf("Selected Clk Source is: EXT \n"); break;
        default: printf("Selected Clk Source is: Unknown \n"); break;
    }

    // Clock and PPS Source select
    printf("-------------------------\n");
    virt_addr = (char*)pcie_base + gpio_sel_addr+gpio_out;
    read_result = *(volatile uint32_t*)virt_addr;
    printf("PPS and Clock Selection is = 0x%X\n", (unsigned int)read_result);

    while(1) {
        // Read PPS and Clock Status
        virt_addr = (char*)pcie_base + gpio_sel_addr+gpio_in;
        read_result = *(volatile uint32_t*)virt_addr;
        printf("PPS and Clock Status is = 0x%X\n", (unsigned int)read_result);
        
        virt_addr = (char*)pcie_base + NTL_PPSS_REGSET_BASE+NTL_PPSS_PULSEWIDTH_REG;
        read_result = *(volatile uint32_t*)virt_addr;
        printf("PPS pulse width is = %d\n", (unsigned int)read_result);
        
        sleep(1);
    }
    
    munmap(pcie_base, mapped_size);
    close(fd);
    return 0;
}