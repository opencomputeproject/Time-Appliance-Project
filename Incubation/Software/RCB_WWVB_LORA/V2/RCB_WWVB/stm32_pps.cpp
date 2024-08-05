
#include "stm32_pps.h"




art_timer_val pps_out_history[PPS_HISTORY_COUNT];
uint64_t pps_out_history_count = 0;
art_timer_val pps_in_history[PPS_HISTORY_COUNT];
uint64_t pps_in_history_count = 0;


void push_art_timer(art_timer_val * val, art_timer_val * history, int history_depth)
{
  for ( int i = 1; i < history_depth; i++)
  {
    memcpy( (void*)&history[i], (void*)&history[i-1], sizeof(art_timer_val));
  }
  memcpy((void*)&history[0], (void*)val, sizeof(art_timer_val));
}



/******************** PPS Output *******************/
uint64_t cur_high_level_count = ((uint64_t)(HW_TIMER_FREQUENCY / 2));
uint64_t cur_low_level_count = ((uint64_t)(HW_TIMER_FREQUENCY / 2));

int64_t phase_shift_amount = 0;

void loop_stm_pps_out()
{
  static uint64_t last_out_millis = 0;
  static bool last_level = 0;
  if ( millis() - last_out_millis > 5000 ) {
    last_out_millis = millis(); // this loop for periodic things
  }

  if ( last_level != get_out_level() )
  {
    //Serial.println("PPS output saw output level change!");
    last_level = get_out_level();
    if ( get_out_level() ) // currently on high level, adjust next low duration
    {
      uint64_t next_low_duration_ticks = 0;
      // rising edge occurred, I'm currently at high level, adjust next low level timing
      next_low_duration_ticks = cur_low_level_count;
      sprintf(print_buffer, "High level now, adjust next low level timing! dur=%" PRIu64 "\r\n", next_low_duration_ticks);
      //Serial.print(print_buffer);
      if ( phase_shift_amount != 0 ) {
        sprintf(print_buffer, "Phase shift by %" PRId64 "\r\n", phase_shift_amount);
        //Serial.print(print_buffer);
        // phase shift is input - output, negative value means output happens after input
        // negative value => need to shorten next level dont need to invert

        if ( phase_shift_amount < 0 ) {
          if ( ((uint64_t)phase_shift_amount*-1) >= next_low_duration_ticks )
          {
            //Serial.println("!!!!!!!!!!!PHASE SHIFT WILL MAKE DURATION NEGATIVE!!!!!!!!");
            next_low_duration_ticks -= ((uint64_t)phase_shift_amount*-1)/2;
            phase_shift_amount /= 2;
          }
          else {
            next_low_duration_ticks -= ((uint64_t)phase_shift_amount*-1);
            phase_shift_amount = 0;
          }
        } else 
        {
          next_low_duration_ticks += phase_shift_amount;
          phase_shift_amount = 0;
        }   
      }
      sprintf(print_buffer, "Next low level duration: %" PRIu64 " ticks\r\n", next_low_duration_ticks);
      //Serial.print(print_buffer);
      set_next_level_duration(next_low_duration_ticks, 0);
    } else { // currently on low level, change next high level duration
      uint64_t next_high_duration_ticks = 0;
      next_high_duration_ticks = cur_high_level_count;
      sprintf(print_buffer, "Low level now, adjust next high level timing! dur=%" PRIu64 "\r\n", next_high_duration_ticks);
      //Serial.print(print_buffer);
      if ( phase_shift_amount != 0 ) {
        sprintf(print_buffer, "Phase shift by %" PRId64 "\r\n", phase_shift_amount);
        //Serial.print(print_buffer);
        // phase shift is input - output, negative value means output happens after input
        // negative value => need to shorten next level dont need to invert
        if ( phase_shift_amount < 0 ) {
          if ( ((uint64_t)phase_shift_amount*-1) >= next_high_duration_ticks )
          {
            //Serial.println("!!!!!!!!!!!PHASE SHIFT WILL MAKE DURATION NEGATIVE!!!!!!!!");
            next_high_duration_ticks -= ((uint64_t)phase_shift_amount*-1)/2;
            phase_shift_amount /= 2;
          }
          else {
            next_high_duration_ticks -= ((uint64_t)phase_shift_amount*-1);
            phase_shift_amount = 0;
          }
        } else 
        {
          next_high_duration_ticks += phase_shift_amount;
          phase_shift_amount = 0;
        }   
      }      
      // falling edge occurred, I'm currently at low level, adjust next high level timing
      //Serial.println("Low level now, adjust next high level timing!");      
      sprintf(print_buffer, "Next high level duration: %" PRIu64 " ticks\r\n", next_high_duration_ticks);
      //Serial.print(print_buffer);
      set_next_level_duration(next_high_duration_ticks, 1);
    }
  }
}




/************************* PPS Input ************************/
bool enable_pps_in_disc = 0;

void enable_pps_in_discipline()
{
  enable_pps_in_disc = 1;
}
void disable_pps_in_discipline()
{
  enable_pps_in_disc = 0;
}


void pps_freq_adjust(int64_t ns_freq_adj) 
{
  uint64_t new_val = 0;  
  if ( ns_freq_adj < 0 ) {    
    new_val =  ((uint64_t)(HW_TIMER_FREQUENCY / 2)) - ( (uint64_t)(ns_freq_adj*-1) );
  } else {
    new_val =  ((uint64_t)(HW_TIMER_FREQUENCY / 2)) + ( (uint64_t)(ns_freq_adj) );
  }
  if ( get_out_level() ) {
    // currently high level , adjust next low level
    set_next_level_duration(new_val, 0);
    cur_low_level_count = new_val;
  } else {
    // currently low level, adjust next high level
    set_next_level_duration(new_val, 1);
    cur_high_level_count = new_val;
  }
}

void pps_step(int64_t step_val_ns)
{
  phase_shift_amount = step_val_ns;
}


void discipline_have_new_freq(int64_t error_signal)
{
  static double integral_error = 0;
  static bool skip_next = 0;
  const static double FREQ_KI = 0.3;
  const static double FREQ_KP = 0.7;
  double prop_error = 0;
  double full_error = 0;
  int64_t adj_tick_count = 0;

  int64_t freq_diff = 0;

  if ( skip_next ) {
    skip_next = 0;
    Serial.println("Skipping one discipline frequency adjustment");
    return;
  }

    // ignore error_signal passed in for now
  uint64_t pps_out_times[2];
  uint64_t pps_in_times[2];
  get_art_timer_val_absolute(&pps_in_times[0], &pps_in_history[0]);
  get_art_timer_val_absolute(&pps_in_times[1], &pps_in_history[1]);
  get_art_timer_val_absolute(&pps_out_times[0], &pps_out_history[0]);
  get_art_timer_val_absolute(&pps_out_times[1], &pps_out_history[1]);

  // compute difference, this is the frequency basically with respect to ART
  pps_out_times[0] = pps_out_times[0] - pps_out_times[1];
  pps_in_times[0] = pps_in_times[0] - pps_in_times[1];

  if ( pps_out_times[0] > pps_in_times[0] ) 
  {
    // pps out accumulated more time with respect to ART, pps out is too slow
    freq_diff = ((int64_t)(pps_out_times[0] - pps_in_times[0])) * -1;
  } else {
    // pps input accumulated more time with respect to ART, pps out is too fast
    freq_diff = ((int64_t)pps_in_times[0] - pps_out_times[0]);
  }
  prop_error = ((double)(freq_diff));
  integral_error += ((double)(freq_diff));

  full_error = FREQ_KI * integral_error + FREQ_KP * prop_error;
  adj_tick_count = (int64_t) (full_error); // keep it whole, only apply to next

  sprintf(print_buffer, "!!!!!!!Doing frequency adjustment, error=%" PRId64 " adj_tick_count=%" PRId64 "\r\n\r\n", freq_diff, adj_tick_count);
  Serial.print(print_buffer);
  pps_freq_adjust(adj_tick_count);

  skip_next = 1;

}


void discipline_with_1pps_in()
{
  static uint64_t last_pps_out_count = 0;
  static uint64_t last_pps_in_count = 0;
  static uint64_t disc_count = 0;
  static bool skip_next = 0;
  
  // make sure have new measurements of PPS input and output
  if ( last_pps_out_count < pps_out_history_count) 
  {
    if ( last_pps_in_count < pps_in_history_count ) {
      last_pps_out_count = pps_out_history_count;
      last_pps_in_count = pps_in_history_count;
      uint64_t out_abs_time = 0;
      uint64_t in_abs_time = 0;
      get_art_timer_val_absolute(&in_abs_time, &pps_in_history[0]);
      get_art_timer_val_absolute(&out_abs_time, &pps_out_history[0]);

      int64_t error_signal = ((int64_t)(in_abs_time)) - ((int64_t)(out_abs_time));
      sprintf(print_buffer, "discipline_with_1pps_in , initial error_signal = %" PRId64 "\r\n", error_signal);
      Serial.print(print_buffer);
      if ( error_signal > 240e6 || error_signal < -240e6 ) {
        // compare history values
        get_art_timer_val_absolute(&in_abs_time, &pps_in_history[1]);
        error_signal = ((int64_t)(in_abs_time)) - ((int64_t)(out_abs_time));
        sprintf(print_buffer, "discipline_with_1pps_in error was too large, in[1] error_signal = %" PRId64 "\r\n", error_signal);
        Serial.print(print_buffer);
      }




      if ( disc_count < PHASE_ADJUSTMENT_THRESHOLD )
      {
        // phase adjust period
        if ( phase_shift_amount == 0 ) {
          if ( skip_next ) {
            Serial.println("Skip next was set, not applying phase shift");
            skip_next = 0;
          } else {
            Serial.println("Phase shift amount is zero, not skipping next!");
            phase_shift_amount = error_signal;
            skip_next = 1;
          }
        } else {
          Serial.println("!!!!!!Phase shift not completely applied yet!!!!");
        }        
      } else {
        discipline_have_new_freq(error_signal);
        // frequency adjust period
      }
      disc_count++;

    }
  }


  //discipline_have_new_freq(ref_period,cur_period);

}







void loop_stm_pps_in()
{
  static uint64_t last_in_millis = 0;
  static uint32_t num_pps_seen = 0;

  if ( millis() - last_in_millis > 5000 ) {
    last_in_millis = millis();
  }

  if ( has_ppsout_timestamp() ) { // also handle PPS output timestamp here
    uint64_t abs_time = 0;

    push_art_timer(&last_pps_out_time, pps_out_history, PPS_HISTORY_COUNT); // push this history
    pps_out_history_count++;


    sprintf(print_buffer, "**PPS Output ");
    /* debug prints
    sprintf(print_buffer, "%s   art_val_ts=%" PRIu64 "\r\n", print_buffer, pps_out_history[0].art_val_ts);
    sprintf(print_buffer, "%s   Capture_TS=%" PRIu16 "\r\n", print_buffer, pps_out_history[0].capture_ts);
    sprintf(print_buffer, "%s   DMA_Remaining_TS=%" PRIu16 "\r\n", print_buffer, pps_out_history[0].dma_remaining_ts);
    sprintf(print_buffer, "%s   Typical_Count_TS=%" PRIu16 "\r\n", print_buffer, pps_out_history[0].typical_count_ts);
    sprintf(print_buffer, "%s   Millis_TS=%" PRIu64 "\r\n", print_buffer, pps_out_history[0].millis_ts);
    sprintf(print_buffer, "%s-------------------\r\n", print_buffer);
    */
    get_art_timer_val_absolute(&abs_time, &pps_out_history[0]);
    sprintf(print_buffer, "%s   Absolute ART timestamp=%" PRIu64 "\r\n", print_buffer, abs_time);
    Serial.println(print_buffer);
    clear_has_ppsout_timestamp();
  }


  if ( has_ppsin_timestamp() ) {
    uint64_t abs_time = 0;

    push_art_timer(&last_pps_in_time, pps_in_history, PPS_HISTORY_COUNT); // push this history
    pps_in_history_count++;
    sprintf(print_buffer, "**PPS Input!");
    /* debug prints
    sprintf(print_buffer, "%s   art_val_ts=%" PRIu64 "\r\n", print_buffer, pps_in_history[0].art_val_ts);
    sprintf(print_buffer, "%s   Capture_TS=%" PRIu16 "\r\n", print_buffer, pps_in_history[0].capture_ts);
    sprintf(print_buffer, "%s   DMA_Remaining_TS=%" PRIu16 "\r\n", print_buffer, pps_in_history[0].dma_remaining_ts);
    sprintf(print_buffer, "%s   Typical_Count_TS=%" PRIu16 "\r\n", print_buffer, pps_in_history[0].typical_count_ts);
    sprintf(print_buffer, "%s   Millis_TS=%" PRIu64 "\r\n", print_buffer, pps_in_history[0].millis_ts);
    sprintf(print_buffer, "%s-------------------\r\n", print_buffer);
    */
    get_art_timer_val_absolute(&abs_time, &pps_in_history[0]);
    sprintf(print_buffer, "%s   Absolute ART timestamp=%" PRIu64 "\r\n", print_buffer, abs_time);
    Serial.println(print_buffer);
    clear_has_ppsin_timestamp();

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
  for ( int i = 0; i < PPS_HISTORY_COUNT; i++ ) {
    pps_out_history[i].valid_ts = 0;
    pps_in_history[i].valid_ts = 0;
  }
}



