
#include <Adafruit_NeoPixel.h>

#define PIN 38
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }
  Serial.println("Hello from ESP32S3");

  pixels.begin();
  pixels.setBrightness(10);

}

void loop() {
  
  delay(1000);
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0,150,0));
  pixels.show();
  Serial.println("ESP32S3 loop green");
  delay(1000);
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0,0,150));
  pixels.show();
  Serial.println("ESP32S3 loop blue");


}
