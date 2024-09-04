
#include "stm32_pps.h"


void loop_stm_pps_out()
{
  static uint64_t last_out_millis = 0;
  if ( millis() - last_out_millis > 5000 ) {
    last_out_millis = millis(); // this loop for periodic things
  }
}
void loop_stm_pps_in()
{
  static uint64_t last_in_millis = 0;

  if ( millis() - last_in_millis > 5000 ) {
    last_in_millis = millis();
  }
}



/************** Top level loop ***************/
uint64_t last_millis = 0;
void loop_stm_pps()
{
  if ( last_millis == 0 ) {
    return;
  }
  loop_stm_pps_out();
  loop_stm_pps_in();
}


void init_stm_pps()
{
  stm32_hrtimer_init();
  last_millis = millis();
}



