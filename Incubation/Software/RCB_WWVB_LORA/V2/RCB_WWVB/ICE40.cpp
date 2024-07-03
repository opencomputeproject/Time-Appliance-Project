

#include "ICE40.h"





void hold_ice40_reset() {
  //wwvb_gpio_pinmode(ICE_SPARE4, OUTPUT);
  //wwvb_gpio_pinmode(ICE_SPARE1, OUTPUT);
  wwvb_digital_write(ICE_SPARE4, 1); // using SPARE4 as reset
  wwvb_digital_write(ICE_SPARE1, 1); // using SPARE1 as mux control to sx1257, 0 to connect direct to sx1257
}

void release_ice40_reset() {
  //wwvb_gpio_pinmode(ICE_SPARE4, OUTPUT);
  //wwvb_gpio_pinmode(ICE_SPARE1, OUTPUT);
  wwvb_digital_write(ICE_SPARE4, 0); // using SPARE4 as reset
  wwvb_digital_write(ICE_SPARE1, 0); // connect SPI6 to SX1257
}

void ice40_stop_stream() {
  //wwvb_gpio_pinmode(ICE_SPARE5, OUTPUT);

  //Serial.println("*******ICE40 STOP STREAM********");
  wwvb_digital_write(ICE_SPARE5, 0);
}
void ice40_start_stream() {
  //wwvb_gpio_pinmode(ICE_SPARE5, OUTPUT);
  //Serial.println("******ICE40 START STREAM********");
  wwvb_digital_write(ICE_SPARE5, 1);
}


// implemented a stream stop logic in the FPGA based on DIO0
// clear it with rising edge / falling edge on spare3
void ice40_clear_stream_gate() {
  wwvb_digital_write(ICE_SPARE3, 0);
  delayMicroseconds(5);
  wwvb_digital_write(ICE_SPARE3, 1);
  delayMicroseconds(5);
  wwvb_digital_write(ICE_SPARE3, 0);
  delayMicroseconds(5);
}



// following https://github.com/mwelling/ICE-V-Wireless/blob/main/Firmware/main/ice.c
// and https://github.com/noscene/ice40_esp32_bitstream_upload/blob/master/ice40_bitstream/ice40_bitstream.ino

#define ICE_SPI_SCK_PIN ICE_CFG_SCK
#define ICE_SPI_MISO_PIN ICE_CFG_MISO
#define ICE_SPI_MOSI_PIN ICE_CFG_MOSI
#define ICE_SPI_CS_PIN ICE_CFG_SS
#define ICE_CDONE_PIN ICE_CDONE
#define ICE_CRST_PIN ICE_CRESET


void reset_inout() {
  //wwvb_gpio_pinmode(ICE_SPI_SCK_PIN, INPUT);
  //wwvb_gpio_pinmode(ICE_SPI_MISO_PIN, INPUT);
  //wwvb_gpio_pinmode(ICE_SPI_MOSI_PIN, INPUT);
  //wwvb_gpio_pinmode(ICE_SPI_CS_PIN, INPUT);
  wwvb_gpio_pinmode(ICE_CDONE_PIN, INPUT);
  wwvb_gpio_pinmode(ICE_CRST_PIN, INPUT);

}

void digitalSync(int usec_delay) {
  delayMicroseconds(usec_delay);
}

void iceClock() {
  wwvb_digital_write(ICE_SPI_SCK_PIN, LOW);
  digitalSync(1);
  wwvb_digital_write(ICE_SPI_SCK_PIN, HIGH);
  digitalSync(1);  
}





bool prog_bitstream_start() {
  wwvb_gpio_pinmode(ICE_SPARE4, OUTPUT);
  wwvb_gpio_pinmode(ICE_SPARE1, OUTPUT);
  wwvb_gpio_pinmode(ICE_SPI_SCK_PIN,     OUTPUT);
  wwvb_gpio_pinmode(ICE_SPI_MOSI_PIN,    OUTPUT);
  wwvb_gpio_pinmode(ICE_CRST_PIN,  OUTPUT);
  wwvb_gpio_pinmode(ICE_SPI_CS_PIN,      OUTPUT);
  // enable reset
  wwvb_digital_write(ICE_CRST_PIN, LOW);
  // start clock high
  wwvb_digital_write(ICE_SPI_SCK_PIN, HIGH);

  // select SRAM programming mode
  wwvb_digital_write(ICE_SPI_CS_PIN, LOW);
  digitalSync(100);

  // release reset
  wwvb_digital_write(ICE_CRST_PIN, HIGH);
  digitalSync(2000);

  // send start clks
  for (int i = 0; i < 8; i++) {
    iceClock();
  }
  
}

// stream buffer to FPGA
bool prog_bitstream_send(unsigned char * buf, long size) {
 for (int k = 0; k < size; k++) {
    byte d = buf[k]; // getchar();
    for (int i = 7; i >= 0; i--) {
      wwvb_digital_write(ICE_SPI_MOSI_PIN, ((d >> i) & 1) ? HIGH : LOW);
      iceClock();
    }
  }
}


// finish upload to FPGA
bool prog_bitstream_finish() {
  for (int i = 0; i < 49; i++) {
      iceClock();
  }
  reset_inout();
}


// Bitstream send to FPGA from static Array
// use xxd -i to generate this header File from xxx.bin and set it to const byte      
bool prog_bitstream(bool reset_only) {
//  assert(enable_prog_port);

  wwvb_gpio_pinmode(ICE_SPI_SCK_PIN,     OUTPUT);
  wwvb_gpio_pinmode(ICE_SPI_MOSI_PIN,    OUTPUT);
  wwvb_gpio_pinmode(ICE_CRST_PIN,  OUTPUT);
  wwvb_gpio_pinmode(ICE_SPI_CS_PIN,      OUTPUT);

  //fprintf(stderr, "reset..\n");

  // enable reset
  wwvb_digital_write(ICE_CRST_PIN, LOW);

  // start clock high
  wwvb_digital_write(ICE_SPI_SCK_PIN, HIGH);

  // select SRAM programming mode
  wwvb_digital_write(ICE_SPI_CS_PIN, LOW);
  digitalSync(100);

  // release reset
  wwvb_digital_write(ICE_CRST_PIN, HIGH);
  digitalSync(2000);

  // fprintf(stderr, "cdone: %s\n", digitalRead(ICE_CDONE_PIN) == HIGH ? "high" : "low");

  if (reset_only)
    return true;

  // fprintf(stderr, "programming..\n");

  for (int i = 0; i < 8; i++) {
    iceClock();
  }


  for (int k = 0; k< fpgaFirmware_size; k++) {
    byte d = fpgaFirmware[k]; // getchar();

    for (int i = 7; i >= 0; i--) {
      wwvb_digital_write(ICE_SPI_MOSI_PIN, ((d >> i) & 1) ? HIGH : LOW);
      iceClock();
    }

    // if (verbose && !(k % 1024) && k)  printf("%3d kB written.\n", k / 1024);
  }

  for (int i = 0; i < 49; i++) {
      iceClock();
  }


  bool cdone_high = digitalRead(ICE_CDONE_PIN) == HIGH;
  // fprintf(stderr, "cdone: %s\n", cdone_high ? "high" : "low");

  reset_inout();
  wwvb_digital_write(ICE_SPI_CS_PIN, HIGH);

  
  if (!cdone_high) return false;

  return true;
}