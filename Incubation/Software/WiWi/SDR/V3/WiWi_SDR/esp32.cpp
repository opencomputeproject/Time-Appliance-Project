

#include "esp32.h"



/******* STM32 UART nonsense *****/
// UART handle declaration
UART_HandleTypeDef huart4;
DMA_HandleTypeDef hdma_uart_tx;
DMA_HandleTypeDef hdma_uart_rx;

#define UART_DMA_BUFFER_SIZE 1024 // Size of the DMA circular buffer
static uint8_t dmaRxBuffer[UART_DMA_BUFFER_SIZE]; // ring buffer
volatile uint16_t dmaLastPosition = 0;  // Tracks the last processed position

uint8_t tx_buffer[256]; // just a place holder, not necessarily needed if you have local buffer
#define ESP32_RX_BUFFER_SIZE 4096
char rx_buffer[ESP32_RX_BUFFER_SIZE]; // some ESP commands can be long 
extern "C" {
  void ESP_UART_RX_DMA_STREAM_HANDLER(void)
  {
    HAL_DMA_IRQHandler(huart4.hdmarx);
  }

  void ESP_UART_TX_DMA_STREAM_HANDLER(void)
  {
    HAL_DMA_IRQHandler(huart4.hdmatx);
  }

  void USART4_IRQHandler(void)
  {
    HAL_UART_IRQHandler(&huart4);
  }

};


void init_esp32()
{

  /* this doesn't work? Not sure why, maybe come back and debug later
  wwvb_gpio_pinmode(ESP32_RST, OUTPUT);
  // do short reset just in beginning
  rtos::ThisThread::sleep_for(5);
  wwvb_digital_write(ESP32_RST, 0);
  rtos::ThisThread::sleep_for(5);
  wwvb_digital_write(ESP32_RST, 1);
  rtos::ThisThread::sleep_for(5);
  */

  // init uart4
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // Configure GPIO pins for UART4 TX and RX
  GPIO_InitStruct.Pin = GPIO_PIN_13;  // TX: PH13
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;         // Alternate function, push-pull
  GPIO_InitStruct.Pull = GPIO_PULLUP;             // No pull-up or pull-down
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;   // High speed
  GPIO_InitStruct.Alternate = GPIO_AF8_UART4;     // UART4 Alternate function
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_9;  // RX: PI9
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  huart4.Instance = UART4;                         // UART instance
  huart4.Init.BaudRate = 1000000;                   // Baud rate
  huart4.Init.WordLength = UART_WORDLENGTH_8B;     // Word length: 8 bits
  huart4.Init.StopBits = UART_STOPBITS_1;          // Stop bits: 1
  huart4.Init.Parity = UART_PARITY_NONE;           // Parity: None
  huart4.Init.Mode = UART_MODE_TX_RX;              // TX and RX mode
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;     // Hardware flow control: None
  huart4.Init.OverSampling = UART_OVERSAMPLING_16; // Oversampling: 16
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE; // Disable one-bit sampling
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT; // No advanced features


  if (HAL_UART_Init(&huart4) != HAL_OK) {
    // Initialization error
    Serial.println("Failed to initialize ESP32 uart4!");
  }

  // DMA handlers, make a ring buffer and pull from it
  /* Configure the DMA handler for Transmission process */
  hdma_uart_tx.Instance                 = ESP_UART_TX_DMA_STREAM;
  hdma_uart_tx.Init.Request             = DMA_REQUEST_UART4_TX;
  hdma_uart_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  hdma_uart_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_uart_tx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_uart_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_uart_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  hdma_uart_tx.Init.Mode                = DMA_NORMAL;
  hdma_uart_tx.Init.Priority            = DMA_PRIORITY_LOW;
  hdma_uart_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
  hdma_uart_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
  hdma_uart_tx.Init.MemBurst            = DMA_MBURST_SINGLE;
  hdma_uart_tx.Init.PeriphBurst         = DMA_PBURST_SINGLE;
  HAL_DMA_Init(&hdma_uart_tx);
  __HAL_LINKDMA(&huart4, hdmatx, hdma_uart_tx);

  hdma_uart_rx.Instance                 = ESP_UART_RX_DMA_STREAM;
  hdma_uart_rx.Init.Request             = DMA_REQUEST_UART4_RX;
  hdma_uart_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  hdma_uart_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_uart_rx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_uart_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_uart_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  hdma_uart_rx.Init.Mode                = DMA_CIRCULAR;
  hdma_uart_rx.Init.Priority            = DMA_PRIORITY_LOW;
  hdma_uart_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
  hdma_uart_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
  hdma_uart_rx.Init.MemBurst            = DMA_MBURST_SINGLE;
  hdma_uart_rx.Init.PeriphBurst         = DMA_PBURST_SINGLE;
  HAL_DMA_Init(&hdma_uart_rx);
  __HAL_LINKDMA(&huart4, hdmarx, hdma_uart_rx);


  /* NVIC configuration for DMA transfer complete interrupt (USARTx_TX) */
  //HAL_NVIC_SetPriority(ESP_UART_TX_DMA_STREAM_IRQ, 10, 1);
  //HAL_NVIC_EnableIRQ(ESP_UART_TX_DMA_STREAM_IRQ);

  /* NVIC configuration for DMA transfer complete interrupt (USARTx_RX) */
  //HAL_NVIC_SetPriority(ESP_UART_RX_DMA_STREAM_IRQ, 10, 0);
  //HAL_NVIC_EnableIRQ(ESP_UART_RX_DMA_STREAM_IRQ);

  /* NVIC configuration for USART, to catch the TX complete */
  //HAL_NVIC_SetPriority(UART4_IRQn, 10, 1);
  //HAL_NVIC_EnableIRQ(UART4_IRQn);

  // start DMA transfer
  dmaLastPosition = 0;
  HAL_UART_Receive_DMA(&huart4, (uint8_t*)dmaRxBuffer, UART_DMA_BUFFER_SIZE);
}

bool esp32_uart_available()
{
    uint16_t dmaCurrentPosition = UART_DMA_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart_rx);
    if (dmaCurrentPosition >= dmaLastPosition) {
        return dmaCurrentPosition - dmaLastPosition;
    } else {
        return (UART_DMA_BUFFER_SIZE - dmaLastPosition + dmaCurrentPosition);
    }
}

uint8_t esp32_uart_read_char(void) {
    if (dmaLastPosition == (UART_DMA_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart_rx))) {
        return -1; // No new data
    }

    uint8_t byte = dmaRxBuffer[dmaLastPosition];
    dmaLastPosition = (dmaLastPosition + 1) % UART_DMA_BUFFER_SIZE;
    return byte;
}

void esp32_uart_clear_buffer() {
  dmaLastPosition = UART_DMA_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart_rx);
}

void esp32_uart_send_string(char * str)
{
  // can improve with some DMA or something later, right now assume blocking is fine
  HAL_UART_Transmit(&huart4, (uint8_t*) str, strlen(str), HAL_MAX_DELAY);
}

int esp32_uart_read_until(char *buffer, size_t buffer_size, const char *terminator, uint32_t timeout) 
{
  uint32_t start_time = millis();
  size_t terminator_len = strlen(terminator);
  size_t bytes_read = 0;
  esp32_uart_clear_buffer();
  while (1) {
    // Check if timeout has occurred
    if ((millis() - start_time) >= timeout) {
        return -1; // Timeout occurred
    }

    // Check if buffer is full
    if (bytes_read >= buffer_size - 1) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        return bytes_read;        // Buffer full
    }

    // Read a single byte from UART
    uint8_t byte;
    if ( esp32_uart_available() ) {
      byte = esp32_uart_read_char();
      buffer[bytes_read++] = byte;
      buffer[bytes_read] = '\0'; // Null-terminate for strstr()
      // Check if the terminator is detected
      if (bytes_read >= terminator_len && strstr(buffer, terminator) != NULL) {
          return bytes_read; // Terminator found
      }
    } else {
      rtos::ThisThread::sleep_for(1);
    }
  }
}


/*************************
**
*
* ESP32 interaction , not CLI
*
**********/

// Function to parse the Wi-Fi network details
int parse_wifi_networks(const char *rx_buffer, WiFiNetwork *networks, int max_networks) {
    char *line;
    char *buffer_copy;
    int network_count = 0;

    // Make a modifiable copy of rx_buffer
    buffer_copy = strdup(rx_buffer);
    if (!buffer_copy) {
        return -1; // Memory allocation failure
    }

    // Tokenize the buffer by newlines to process line by line
    line = strtok(buffer_copy, "\n");
    while (line != NULL) {
        // Skip the header line
        if (strstr(line, "Nr | SSID") != NULL) {
            line = strtok(NULL, "\n");
            continue;
        }

        // Parse the details for each line
        if (network_count < max_networks) {
            WiFiNetwork *current = &networks[network_count];

            // Tokenize the line by the | delimiter
            char *token = strtok(line, "|");
            if (token != NULL) current->number = atoi(token);

            token = strtok(NULL, "|");
            if (token != NULL) strncpy(current->ssid, token, sizeof(current->ssid) - 1);

            token = strtok(NULL, "|");
            if (token != NULL) current->rssi = atoi(token);

            token = strtok(NULL, "|");
            if (token != NULL) current->channel = atoi(token);

            token = strtok(NULL, "|");
            if (token != NULL) strncpy(current->encryption, token, sizeof(current->encryption) - 1);

            // Increment the network count
            network_count++;
        }

        // Get the next line
        line = strtok(NULL, "\n");
    }

    // Free the allocated memory for the buffer copy
    free(buffer_copy);

    return network_count;
}


int scan_wifi_networks(WiFiNetwork *networks, int max_networks)
{

}




/*************** Top level init and CLI 
*
*
*
*
*
***********/


// this is called from cli thread
// so it doesnt lock up other threads 
// just locks up CLI thread, which is what I want
void onESP32Passthrough(EmbeddedCli *cli, char *args, void *context)
{
  char read_char;
  char write_char;
  
  Serial.println("Entering ESP32 UART passthrough mode, do Ctrl+c to exit");
  //ESP32 UART is always receiving with DMA , just move this pointer to current 
  //dmaLastPosition = UART_DMA_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart_rx);
  // sometimes you want this and sometimes not, since this is debug passthrough will leave this out
  while ( 1 ) {
    // read user input to send to esp32
    while ( Serial.available() > 0) {
      // ctrl c is ascii 0x3
      read_char = Serial.read();
      if ( read_char == 0x3 ) {
        Serial.println("Got ctrl+C, ending ESP32 passthrough mode!");
        return;
      }
      HAL_UART_Transmit(&huart4, (uint8_t*) (&read_char), 1, HAL_MAX_DELAY);
    }

    // read esp32 output to send to user
    while ( esp32_uart_available() ) {
      Serial.print( (char)esp32_uart_read_char() );
    }
    rtos::ThisThread::sleep_for(10);
  }
}



void onESP32Scan(EmbeddedCli *cli, char *args, void *context)
{
  int num_chars = 0;
  // get back to top level 
  esp32_uart_send_string("cd ..\r\n");
  esp32_uart_read_until(rx_buffer, ESP32_RX_BUFFER_SIZE, ">", 10000);
  esp32_uart_send_string("wifi\r\n");
  esp32_uart_read_until(rx_buffer, ESP32_RX_BUFFER_SIZE, ">", 10000);
  esp32_uart_send_string("scan\r\n");

  num_chars = esp32_uart_read_until(rx_buffer, ESP32_RX_BUFFER_SIZE, "wifi>", 10000); // 10 second timeout -> eat the prompt from the echo
  num_chars = esp32_uart_read_until(rx_buffer, ESP32_RX_BUFFER_SIZE, "wifi>", 10000); // 10 second timeout
  if ( num_chars > 0 ) {
    Serial.println("Scan results from ESP32:");
    Serial.print(rx_buffer);
  }
  // this works

}


static Node esp32_passthrough_node = { .name = "esp32-passthrough", 
  .type = MY_FILE, 
  .cliBinding = {
    "passthrough",
    "Enter passthrough mode to ESP32 CLI, use Ctrl+c to quit",
    true,
    nullptr,
    onESP32Passthrough
  }
};

static Node esp32_wifi_scan_node = { .name = "esp32-scan",
  .type = MY_FILE, 
  .cliBinding = {
    "scan",
    "Get Wifi scan results",
    true,
    nullptr,
    onESP32Scan
  }

};

void esp32_dir_operation(EmbeddedCli *cli, char *args, void *context); // forward declaration

static Node * esp32_files[] = { &esp32_passthrough_node, &esp32_wifi_scan_node };

static Node esp32_dir = {
    .name = "esp32",
    .type = MY_DIRECTORY,
    .cliBinding = {"esp32",
          "esp32 mode",
          true,
          nullptr,
          esp32_dir_operation},
    .parent = 0,
    .children = esp32_files,
    .num_children = sizeof(esp32_files) / sizeof(esp32_files[0])
};

void esp32_dir_operation(EmbeddedCli *cli, char *args, void *context) {
  change_to_node(&esp32_dir);
}

// Initialize function to set the parent pointers if needed
void esp32_fs_init() {
  for (int i = 0; i < esp32_dir.num_children; i++) {
    esp32_files[i]->parent = &esp32_dir;
  }
  add_root_filesystem(&esp32_dir);
}




void init_esp32_cli()
{
  Serial.println("ESP32 Init start!");
  init_esp32();

  Serial.println("ESP32 add cli");

  esp32_fs_init();
}