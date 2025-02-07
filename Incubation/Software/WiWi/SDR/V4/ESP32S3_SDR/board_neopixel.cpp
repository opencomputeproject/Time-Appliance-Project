

#include "board_neopixel.h"


#define PIN 38
#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


void neopixel_set_value(EmbeddedCli *cli, char *args, void *context) 
{
  Serial.println("Neopixel set value");
  uint8_t red=0;
  uint8_t green=0;
  uint8_t blue=0;
  uint8_t brightness=0;

  if ( embeddedCliGetTokenCount(args) >= 1 ) { // 1 arg
    red = atoi( embeddedCliGetToken(args, 1) );
  } 
  if ( embeddedCliGetTokenCount(args) >= 2 ) { // 2 arg
    green = atoi( embeddedCliGetToken(args, 2) );
  } 
  if ( embeddedCliGetTokenCount(args) >= 3 ) { // 3 arg
    blue = atoi( embeddedCliGetToken(args, 3) );
  } 
  if ( embeddedCliGetTokenCount(args) >= 4 ) { // 4 arg
    brightness = atoi( embeddedCliGetToken(args, 4) );
  } 
  sprintf(print_buffer, "Setting Neopixel RGB Brightness = %d / %d / %d / %d\r\n",
    red, green, blue, brightness);
  SERIAL_PRINT(print_buffer);

  pixels.clear();

  pixels.setBrightness(brightness);

  pixels.setPixelColor(0, pixels.Color(red,green,blue) );
  

  pixels.show();




}



static Node neopixel_start_node = { .name = "", 
  .type = MY_FILE, 
  .cliBinding = {"set",
    "Set neopixel value, takes up to 4 args, R G B Brightness, all uint8_t values",
    true,
    nullptr,
    neopixel_set_value}
};








/********************** Boiler plate code for CLI ****************/


static Node * neopixel_files[] = { &neopixel_start_node };



void neopixel_dir_operation(EmbeddedCli *cli, char *args, void *context);

static Node neopixel_dir = {
    .name = "neopixel",
    .type = MY_DIRECTORY,
    .cliBinding = {"neopixel",
          "neopixel operations",
          true,
          nullptr,
          neopixel_dir_operation},
    .parent = 0,
    .children = neopixel_files,
    .num_children = sizeof(neopixel_files) / sizeof(neopixel_files[0])
};



void neopixel_dir_operation(EmbeddedCli *cli, char *args, void *context)
{
  //SERIAL_PRINTLN("Want to change into SPI mode!");
  change_to_node(&neopixel_dir);
}

void neopixel_init() {
  for (int i = 0; i < neopixel_dir.num_children; i++) {
    neopixel_files[i]->parent = &neopixel_dir;
  }
  add_root_filesystem(&neopixel_dir);
}



void init_neopixel_cli()
{
  neopixel_init();
  pixels.begin();
}