#include "my_ethernet.h"

/**************** Ethernet functions with STM32 APIs ************/
// Ethernet handle
ETH_HandleTypeDef heth;
ETH_TxPacketConfig TxConfig;



// Define RX and TX buffer sizes and descriptor counts
#define ETH_RX_DESC_CNT 4   // Number of RX descriptors
#define ETH_TX_DESC_CNT 4   // Number of TX descriptors
#define ETH_RX_BUFFER_CNT             12U
#define ETH_RX_BUFFER_SIZE 1536

// RX and TX buffers
ALIGN_32BYTES(static uint8_t rx_buffer[ETH_RX_DESC_CNT][ETH_MAX_PACKET_SIZE]);
ALIGN_32BYTES(static uint8_t tx_buffer[ETH_TX_DESC_CNT][ETH_MAX_PACKET_SIZE]);


#pragma location=0x30040000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x30040100
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#pragma location = 0x30040200
uint8_t memp_memory_RX_POOL_base[ ETH_RX_BUFFER_CNT * (ETH_RX_BUFFER_SIZE + 24) ];

__IO uint32_t TxPkt = 0;
__IO uint32_t RxPkt = 0;

uint8_t eth_mac[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};



void init_ethernet()
{
  // some of this from chatgpt
  // some from https://github.com/stm32-hotspot/STM32H7-LwIP-Examples/blob/main/STM32H747_Disco_M7_ETH/CM7/LWIP/Target/ethernetif.c
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  ETH_MACConfigTypeDef MACConf = {0};

  /** Ethernet pins configuration
    * PA1  ------> ETH_REF_CLK
    * PA2  ------> ETH_MDIO
    * PA7  ------> ETH_CRS_DV
    * PC1  ------> ETH_MDC
    * PG11 ------> ETH_TX_EN
    * PG13 ------> ETH_TXD0
    * PG14 ------> ETH_TXD1
    * PC4  ------> ETH_RXD0
    * PC5  ------> ETH_RXD1
    */
  
  // Configure GPIO pins for Ethernet
  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  // Configure Ethernet peripheral
  heth.Instance = ETH;
  heth.Init.MACAddr = eth_mac; // Example MAC address
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;

  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1536;

  if (HAL_ETH_Init(&heth) != HAL_OK) {
      // Initialization Error
    Serial.println("HAL Ethernet init not good!");
  } else {
    Serial.println("HAL Ethernet init successful!");
  }


  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;

}







/************** CLI Functions *************/


void onEthernetMAC(EmbeddedCli *cli, char *args, void *context)
{
  uint32_t addr = 0;
  int value_count = 0;

  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("ethernet EEPROM Write no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) < 2 ) {
    Serial.println("ethernet EEPROM Write needs at least 2 arguments!");
    return;
  } 

  Serial.println("On ethernet mac!");
}




static Node ethernet_mac_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {
    "mac",
    "Pass MAC to set MAC, pass nothing to print current MAC",
    true,
    nullptr,
    onEthernetMAC
  }
};

/****** Boiler plate kind of code ********/

void ethernet_dir_operation(EmbeddedCli *cli, char *args, void *context);

static Node * ethernet_files[] = { &ethernet_mac_node };

static Node ethernet_dir = {
    .name = "ethernet",
    .type = MY_DIRECTORY,
    .cliBinding = {"ethernet",
          "ethernet mode",
          true,
          nullptr,
          ethernet_dir_operation},
    .parent = 0,
    .children = ethernet_files,
    .num_children = sizeof(ethernet_files) / sizeof(ethernet_files[0])
};

void ethernet_dir_operation(EmbeddedCli *cli, char *args, void *context) {
  change_to_node(&ethernet_dir);
}

// Initialize function to set the parent pointers if needed
void ethernet_fs_init() {
  for (int i = 0; i < ethernet_dir.num_children; i++) {
    ethernet_files[i]->parent = &ethernet_dir;
  }
  add_root_filesystem(&ethernet_dir);
}








void init_ethernet_cli()
{
  init_ethernet();
    // expose ethernet CLI
  ethernet_fs_init();
}