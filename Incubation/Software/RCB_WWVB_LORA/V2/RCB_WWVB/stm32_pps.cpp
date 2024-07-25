
#include "stm32_pps.h"


uint64_t last_input_timestamp[2] = {0,0};
uint64_t last_output_timestamp[2] = {0,0};
void push_output_timestamp(uint64_t pps_out_ts)
{
  last_output_timestamp[1] = last_output_timestamp[0];
  last_output_timestamp[0] = pps_out_ts;
}
void push_input_timestamp(uint64_t pps_in_ts)
{
  last_input_timestamp[1] = last_input_timestamp[0];
  last_input_timestamp[0] = pps_in_ts;
}




/******************** PPS Output *******************/
uint64_t cur_high_level_count = ((uint64_t)(HW_TIMER_FREQUENCY / 2));
uint64_t cur_low_level_count = ((uint64_t)(HW_TIMER_FREQUENCY / 2));

void loop_stm_pps_out()
{
  static uint64_t last_out_millis = 0;
  static bool last_out_level = 0;
  if ( millis() - last_out_millis > 5000 ) {
    last_out_millis = millis(); // this loop for periodic things
  }
  if ( get_out_level() != last_out_level ) {
    // PPS output edge occurred, update the next edge period
    last_out_level = get_out_level();
    if ( get_out_level() )
    {
      uint64_t next_low_duration_ticks = 0;
      // rising edge occurred, I'm currently at high level, adjust next low level timing
      Serial.println("Rising edge occurred, adjust next low level timing!");
      push_output_timestamp( get_last_rising_edge_timestamp() );


      next_low_duration_ticks = cur_low_level_count;

      sprintf(print_buffer, "Next low level duration: %" PRIu64 " ticks\r\n", next_low_duration_ticks);
      Serial.print(print_buffer);
      set_next_level_duration(next_low_duration_ticks, 0);
    } else {
      uint64_t next_high_duration_ticks = 0;
      next_high_duration_ticks = cur_high_level_count;
     
      // falling edge occurred, I'm currently at low level, adjust next high level timing
      Serial.println("Falling edge occurred, adjust next high level timing!");      
      sprintf(print_buffer, "Next high level duration: %" PRIu64 " ticks\r\n", next_high_duration_ticks);
      Serial.print(print_buffer);
      set_next_level_duration(next_high_duration_ticks, 1);
    }
  }
}




/************************* PPS Input ************************/
bool enable_pps_in_disc = 1;

void enable_pps_in_discipline()
{
  enable_pps_in_disc = 1;
}
void disable_pps_in_discipline()
{
  enable_pps_in_disc = 0;
}

void discipline_have_new_freq(int64_t ref_period, int64_t cur_period)
{
  const static uint64_t art_time_halfperiod_count = ((uint64_t)(HW_TIMER_FREQUENCY / 2));
  const static uint64_t art_time_period_count = ((uint64_t)HW_TIMER_FREQUENCY);
  static double integral_error = 0;
  const static double FREQ_KI = 0.1;
  const static double FREQ_KP = 0.6;
  double prop_error = 0;
  double full_error = 0;
  int64_t adj_tick_count = 0;
  

  prop_error = ((double)(ref_period - cur_period));
  integral_error += ((double)(ref_period - cur_period));

  full_error = FREQ_KI * integral_error + FREQ_KP * prop_error;
  adj_tick_count = (int64_t) (full_error/2); // divide by 2, adjust both rising and falling periods
  sprintf(print_buffer,"Doing frequency adjustment, refperiod=%" PRId64 
    ", cur_period=%" PRId64 ", prop_error=%f, integral_error=%f, full_error=%f, adj_tick_count=%" PRId64 "\r\n",
      ref_period, cur_period, prop_error, integral_error, full_error, adj_tick_count);
  Serial.print(print_buffer);

  if ( adj_tick_count < 0 ) {
    cur_high_level_count = cur_high_level_count - ( (uint64_t)(adj_tick_count*-1) );
    cur_low_level_count = cur_low_level_count - ( (uint64_t)(adj_tick_count*-1) );
  } else {
    cur_high_level_count = cur_high_level_count + ( (uint64_t)(adj_tick_count) );
    cur_low_level_count = cur_low_level_count + ( (uint64_t)(adj_tick_count) );
  }
}


void discipline_with_1pps_in()
{
  if ( last_input_timestamp[0] == 0 ||
    last_input_timestamp[1] == 0 ||
    last_output_timestamp[0] == 0 ||
    last_output_timestamp[1] == 0 )
  {
    Serial.println("Discipline pps in, haven't received two input edges and two output edges for frequency adjust");
    return;
  }

  // have history
  // do frequency adjustment, then phase

  int64_t ref_period = (int64_t)(last_input_timestamp[0] - last_input_timestamp[1]);
  int64_t cur_period = (int64_t)(last_output_timestamp[0] - last_output_timestamp[1]);

  discipline_have_new_freq(ref_period,cur_period);

  // reset these back to zero after using them
  last_input_timestamp[1] = 0;
  last_output_timestamp[1] = 0;

}


void loop_stm_pps_in()
{
  static uint64_t last_in_millis = 0;
  static uint32_t num_pps_seen = 0;

  if ( millis() - last_in_millis > 5000 ) {
    last_in_millis = millis();
  }
  if ( has_timestamp() ) {
    uint64_t art_time = get_pps_in_timestamp_art();
    push_input_timestamp(art_time);
    uint64_t last_1pps_out_time = get_last_rising_edge_timestamp();
    sprintf(print_buffer, "Got PPS Input! Timestamp info:\r\n");
    sprintf(print_buffer, "%s   art_val_ts=%" PRIu64 "\r\n", print_buffer, art_val_ts);
    sprintf(print_buffer, "%s   Capture_TS=%" PRIu16 "\r\n", print_buffer, capture_ts);
    sprintf(print_buffer, "%s   DMA_Remaining_TS=%" PRIu16 "\r\n", print_buffer, dma_remaining_ts);
    sprintf(print_buffer, "%s   Typical_Count_TS=%" PRIu16 "\r\n", print_buffer, typical_count_ts);
    sprintf(print_buffer, "%s   Millis_TS=%" PRIu64 "\r\n", print_buffer, millis_ts);
    sprintf(print_buffer, "%s-------------------\r\n", print_buffer);
    sprintf(print_buffer, "%s   1pps 64-bit art timestamp=%" PRIu64 "\r\n", print_buffer, art_time);
    sprintf(print_buffer, "%s-------------------\r\n", print_buffer);
    sprintf(print_buffer, "%s   last_1pps_out_time=%" PRIu64 "\r\n", print_buffer, last_1pps_out_time); 
    Serial.println(print_buffer);
    clear_has_timestamp();

    if ( enable_pps_in_disc && num_pps_seen > 6 ) {
      Serial.println("Doing 1pps input discipline");
      discipline_with_1pps_in();
      Serial.println("Done 1pps input discipline");
      Serial.println("");
    } else {
      num_pps_seen++;
    }
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
  for ( int i = 0; i < 2; i++ ) {
    last_input_timestamp[i] = 0;
    last_output_timestamp[i] = 0;
  }
}



