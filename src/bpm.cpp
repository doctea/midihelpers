#include "bpm.h"
#include "clock.h"

volatile uint32_t ticks = 0;
float bpm_current = 120.0f; //BPM_MINIMUM; //60.0f;
//double ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
float micros_per_tick = (float)1000000.0 * ((float)60.0 / (float)(bpm_current * (float)PPQN));

volatile long last_processed_tick = -1;

volatile bool playing = true;
bool single_step = false;
bool restart_on_next_bar = false;

void set_bpm(float new_bpm) {
  if (bpm_current!=new_bpm) {
    bpm_current = new_bpm;
    #ifdef USE_UCLOCK
      //ATOMIC(
          //uClock.setTempo(new_bpm); //bpm_current * 24);
      //)
      set_new_bpm(bpm_current);
    #else
      //ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
      micros_per_tick = (float)1000000.0 * ((float)60.0 / (float)(bpm_current * (float)PPQN));
      //Serial.printf("%i bpm is %0.4f beats per second?\n", bpm_current, bpm_current/60.0f);
      //Serial.printf("set ms_per_tick to %f\n", ms_per_tick);
      Serial.printf("set micros_per_tick to %f\n", micros_per_tick);
    #endif
    Serial.print(F("set bpm to "));
    Serial.println(bpm_current);
  }
}


void set_restart_on_next_bar(bool v) {
  restart_on_next_bar = v;
}
void set_restart_on_next_bar_on() {
  set_restart_on_next_bar(true);
}
bool is_restart_on_next_bar() {
  return restart_on_next_bar;
}