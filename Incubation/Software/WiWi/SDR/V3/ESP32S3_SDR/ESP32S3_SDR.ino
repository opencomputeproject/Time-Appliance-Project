
//#include <Adafruit_NeoPixel.h>
#include <HardwareSerial.h>

#include "menu_cli.h"
#include "wifi_cli.h"
#include "espnow_cli.h"
#include "driver/gpio.h"


#if CONFIG_FREERTOS_UNICORE
#define TASK_RUNNING_CORE 0
#else
#define TASK_RUNNING_CORE 1
#endif


/***** Top level freertos tasks, others can be started in different levels *****/


void debug_task(void * parameter)
{
  // Configure GPIO as output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << 38),  // Bit mask for GPIO38
        .mode = GPIO_MODE_OUTPUT,                 // Set as output mode
        .pull_up_en = GPIO_PULLUP_DISABLE,        // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,    // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE            // No interrupt
    };
    gpio_config(&io_conf);


    // Toggle GPIO in a loop
    while (1) {
        gpio_set_level(GPIO_NUM_38, 1);  // Set GPIO38 high
        vTaskDelay(pdMS_TO_TICKS(500));     // Delay for 500ms

        gpio_set_level(GPIO_NUM_38, 0);  // Set GPIO38 low
        vTaskDelay(pdMS_TO_TICKS(500));     // Delay for 500ms
    }
}


void setup() {
  Serial.begin(115200);
  Serial0.begin(1000000); 
  /*
  while (!Serial) { // debug code, wait until user connects to usb 
    delay(1);
  }
  */
  delay(1000);
  //Serial.println("Test start!");
  SERIAL_PRINTLN("Hello from ESP32S3");


  init_menu_cli();
  init_wifi_cli();
  init_espnow_cli();

  // a bit of a hack, call this after everything else is initialized
  // makes sure the root directory on start up has everything properly
  addMenuCLI_current_directory();

  sprintf(print_buffer, "Detected baud rate is %lu\r\n", Serial0.baudRate() );
  SERIAL_PRINT(print_buffer);

  //xTaskCreate(debug_task, "top level debug task", 4096, NULL, 0, NULL);
}

// don't put stuff in loop for esp32s3
void loop() { 
}
