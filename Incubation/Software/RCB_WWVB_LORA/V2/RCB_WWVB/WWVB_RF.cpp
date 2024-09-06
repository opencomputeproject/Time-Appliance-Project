#include "WWVB_RF.h"

// Define the ADC handle
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
uint32_t adcBuffer[1024];  // Buffer for storing 1024 samples

static bool conversion_running = 0;
static bool conversion_done = 0;
static bool conversion_error = 0;



void WWVB_RF_Init()
{
  return; // hacking these out, remove this return to enable ADC enabled and periodic reading
  Serial.println("Starting WWVB RF Init");

  for ( int i = 0; i < 1024; i++ ) {
    adcBuffer[i] = 0;
  }

  // Initialize RF switches, default to SMA / non amplifier
  // VCTL = 0 -> output2, VCTL = 1 -> output1
  wwvb_gpio_pinmode(WWVB_SMA_UFL_SEL, OUTPUT);
  wwvb_gpio_pinmode(WWVB_AMPLIFIER_SEL0, OUTPUT);
  wwvb_gpio_pinmode(WWVB_AMPLIFIER_SEL1, OUTPUT);

  wwvb_digital_write(WWVB_SMA_UFL_SEL, 1); // SMA
  wwvb_digital_write(WWVB_AMPLIFIER_SEL0, 0); // for non amplifier, first switch is output 2
  wwvb_digital_write(WWVB_AMPLIFIER_SEL1, 1); // for non amplifier, second switch is output 1


  
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // Configure PA0 (ADC1_INP0)
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);



  // Configure the DMA handle for ADC1
  hdma_adc1.Instance = WWVB_ADC_DMA_STREAM;
  hdma_adc1.Init.Request = DMA_REQUEST_ADC1;
  hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_adc1.Init.Mode = DMA_NORMAL; 
  hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;


  if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
  {
    Serial.println("WWVB DMA INIT FAILED");
  }


  // Link the initialized DMA handle to the ADC handle
  __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

  // Enable DMA Interrupt
  HAL_NVIC_SetPriority(WWVB_ADC_DMA_STREAM_IRQ, 3, 0);
  HAL_NVIC_EnableIRQ(WWVB_ADC_DMA_STREAM_IRQ);

  ADC_ChannelConfTypeDef sConfig = {0};

  // Configure ADC1 peripheral
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_16B;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  //hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT; // not applicable for this ADC
  hadc1.Init.NbrOfConversion = 1;
  //hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  hadc1.Init.OversamplingMode = DISABLE;

  // Initialize the ADC peripheral
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
      Serial.println("HAL_ADC_INIT Failed!");
  }

  // Configure the proper channel for PA0_C (replace ADC_CHANNEL_X with the proper one)
  sConfig.Channel = ADC_CHANNEL_16;  // Example: PA0_C could be on ADC_CHANNEL_16
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLE_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;

  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
      Serial.println("HAL_ADC_CONFIGCHANNEL FAILED");
  }

  if ( HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK ) {
    Serial.println("HAL ADCEx calibration start failed!");
  }

  Serial.println("WWVB ADC Init successful!");

  
}

extern "C" {
  // ADC conversion complete callback function
  void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
  {
    conversion_done = 1;

  }
  void WWVB_ADC_DMA_STREAM_HANDLER()
  {
    //HAL_ADC_IRQHandler(&hadc1);
    HAL_DMA_IRQHandler(&hdma_adc1);
    // Called when 1024 samples are transferred via DMA
    conversion_done = 1;
  }

  void HAL_ADC_ErrorCallback(ADC_HandleTypeDef* hadc) 
  {
    conversion_error = 1;
  }
}


unsigned long last_wwvb_loop_time = 0;
void WWVB_RF_Loop()
{
  return;
  if ( millis() - last_wwvb_loop_time > 5000 ) {
    Serial.println("WWVB RF Loop start");
    last_wwvb_loop_time = millis();
    if ( conversion_error == 1 ) {
      Serial.println("CONVERSION ERROR");
    }  
    //print_dma_registers("WWVB ADC1 DMA", WWVB_ADC_DMA_STREAM);
    sprintf(print_buffer, "ADC raw value %d\r\n", ADC1->DR);
    Serial.print(print_buffer);

    /**** ADC DMA example ******/
    if ( conversion_running == 0 ) { // first time case
      // Start ADC conversion in DMA mode
      if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcBuffer, 1024) != HAL_OK)
      {
          Serial.println("HAL ADC Start DMA failed try 1!!!!");
          return;
      }
      Serial.println("Started ADC DMA!");
      conversion_running = 1;
    }
    if ( conversion_running ) { // repeat case
      if ( conversion_done ) {
        Serial.println("WWVB ADC Conversion done!");

        for ( int i = 0; i < 10 ; i++ ) {
          sprintf(print_buffer,"ADC value %d = %d\r\n", i, adcBuffer[i]);
          Serial.print(print_buffer);
        }
        conversion_running = 0;
        conversion_done = 0;
        if ( HAL_ADC_Stop_DMA(&hadc1) != HAL_OK ) {
          Serial.println("HAL ADC STop dma failed!");
        }
        if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcBuffer, 1024) != HAL_OK)
        {
            Serial.println("HAL ADC Start DMA failed try 2!!!!");
            return;
        }
        conversion_running = 1;
      }
    }
  }
}