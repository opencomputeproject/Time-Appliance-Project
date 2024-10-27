

#include "at86rf215iq.h"




SPI_HandleTypeDef * at86_spi;
void at86_init(SPI_HandleTypeDef * spi)
{
  // top level init
  // using same SPI as SX1276, just keep a reference to that handler
  at86_spi = spi;

  wwvb_gpio_pinmode(AT86_SPI_SEL,OUTPUT);
  wwvb_digital_write(AT86_SPI_SEL, 1); // spi select high for idle

  wwvb_gpio_pinmode(AT86_IRQ,INPUT);

  wwvb_gpio_pinmode(AT86_RST,OUTPUT);
  wwvb_digital_write(AT86_RST, 1); // reset high for normal operation

  // do short reset just in beginning
  delayMicroseconds(10);
  wwvb_digital_write(AT86_RST, 0);
  delayMicroseconds(50);
  wwvb_digital_write(AT86_RST, 1);
  delayMicroseconds(10);
}

bool at86_write_single_reg(uint16_t address, uint8_t val)
{
  uint8_t bytes_to_send[3];
  uint8_t response[3];
  HAL_StatusTypeDef retval;

  bytes_to_send[0] = (uint8_t)(((address >> 8) & 0x3f) | 0x80); // address[13:8] and write mode
  bytes_to_send[1] = (uint8_t)(address & 0xff); 
  bytes_to_send[2] = val; 
  response[0] = 0;
  response[1] = 0;
  response[2] = 0;
  // slave select low
  wwvb_digital_write(AT86_SPI_SEL, 0);
  delayMicroseconds(1); // small delay just for safety, not sure needed

  // just use STM HAL 
  retval = HAL_SPI_TransmitReceive(at86_spi, bytes_to_send, response, 3, HAL_MAX_DELAY);  //ignore receive data

  wwvb_digital_write(AT86_SPI_SEL, 1);
  delayMicroseconds(1);

  if ( retval != HAL_OK) {
    Serial.println("AT86 write single reg not ok");
    return 0;
  }

  return 1;

  // think that's it, pretty simple
}
bool at86_read_single_reg(uint16_t address, uint8_t * val)
{
  uint8_t bytes_to_send[3];
  uint8_t response[3];
  HAL_StatusTypeDef retval;

  bytes_to_send[0] = (uint8_t)(((address >> 8) & 0x3f) | 0x00); // address[13:8] and read mode
  bytes_to_send[1] = (uint8_t)(address & 0xff); 
  bytes_to_send[2] = 0x0; //ignored
  response[0] = 0;
  response[1] = 0;
  response[2] = 0;
  // slave select low
  wwvb_digital_write(AT86_SPI_SEL, 0);
  delayMicroseconds(1); // small delay just for safety, not sure needed

  // just use STM HAL 
  retval = HAL_SPI_TransmitReceive(at86_spi, bytes_to_send, response, 3, HAL_MAX_DELAY);  //ignore receive data

  wwvb_digital_write(AT86_SPI_SEL, 1);
  delayMicroseconds(1);

  if ( retval != HAL_OK) {
    Serial.println("AT86 read single reg not ok");
    return 0;
  } 
  *val = response[2]; // last byte is the read value 

  return 1;



}






/*************** Top level init and CLI ***********/

void onAT86Write(EmbeddedCli *cli, char *args, void *context)
{
  uint16_t addr = 0;
  uint8_t writeval = 0;
  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("AT86 write no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 2 ) {
    Serial.println("AT86 write needs 2 arguments!");
    return;
  } 

  if ( !try_parse_hex_uint16t( embeddedCliGetToken(args, 1), &addr ) ) {
    Serial.println("Failed to parse first argument for uint16_t");
    return;
  }
  if ( !try_parse_hex_uint8t( embeddedCliGetToken(args, 2), &writeval ) ) {
    Serial.println("Failed to parse second argument for uint8_t");
    return;
  }


  if ( at86_write_single_reg(addr, writeval) ) {
    Serial.println("AT86 register write done");
  } else {
    Serial.println("AT86 register write failed!");
  }
}

void onAT86Read(EmbeddedCli *cli, char *args, void *context)
{
  uint16_t addr = 0;
  uint8_t read_val = 0;
  if (embeddedCliGetTokenCount(args) == 0) {
    Serial.println("AT86 read no arguments!");
    return;
  } else if ( embeddedCliGetTokenCount(args) != 1 ) {
    Serial.println("AT86 read needs 1 argument!");
    return;
  } 

  if ( !try_parse_hex_uint16t( embeddedCliGetToken(args, 1), &addr ) ) {
    Serial.println("Failed to parse first argument for uint16_t");
    return;
  }


  if ( at86_read_single_reg(addr, &read_val) ) {
    sprintf(print_buffer, "AT86 addr=0x%x, read back 0x%x\r\n", 
      addr, read_val);
    Serial.print(print_buffer);
  } else {
    Serial.println("AT86 register read failed!");
  }
}



void init_at86_cli(SPI_HandleTypeDef * spi)
{
  Serial.println("AT86 Init start!");
  at86_init(spi);

  Serial.println("AT86 add cli");

  // expose sx1276 CLI
  
  embeddedCliAddBinding(cli, {
          "at86-write-reg",
          "Write a AT86RF215IQ register, pass address (13-bit) / value (8-bit) ex: at86-write-reg 0x6 0x6c",
          true,
          nullptr,
          onAT86Write
  });

  embeddedCliAddBinding(cli, {
          "at86-read-reg",
          "Read a AT86RF215IQ register, pass address (13-bit)  ex: at86-read-reg 0x6",
          true,
          nullptr,
          onAT86Read
  });

}