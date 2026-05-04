#include "bpm.h"
#include "clock.h"

volatile uint32_t ticks = 0;
volatile float bpm_current = 120.0f; //BPM_MINIMUM; //60.0f;
//double ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
volatile float micros_per_tick = (float)1000000.0f * ((float)60.0f / (float)(bpm_current * (float)PPQN));

volatile long last_processed_tick = -1;

#ifdef USE_UCLOCK
  volatile bool playing = false;
#else
  volatile bool playing = true;
#endif
bool single_step = false;
bool restart_on_next_bar = false;


// is_bpm_on_* are now inline in bpm.h

bool is_bpm_on_multiplier(unsigned long ticks, float multiplier, unsigned long offset) {
  unsigned long p = ((float)PPQN*multiplier);
  #ifdef DEBUG_BPM
    Serial.print(F("is_bpm_on_multiplier(ticks="));
    Serial.print(ticks);
    Serial.print(F(", multiplier="));
    Serial.print(multiplier);
    Serial.print(F(", offset="));
    Serial.print(offset);
    Serial.print(F(") checking ticks "));
    Serial.print(ticks);
    Serial.print(F(" with PPQN*multiplier "));
    Serial.print(p);
    Serial.print(F(" against offset "));
    Serial.print(offset);
    Serial.print(F(" == ticks%p = "));
    Serial.print(ticks%p);
    Serial.print(F(" ? ="));
  #endif

  bool v = (ticks==offset || ticks%p == offset);  

  #ifdef DEBUG_BPM
    Serial.print(v ? F("true!") : F("false!"));
    Serial.println();
  #endif
  return v;
}

void set_bpm(float new_bpm) {
  if (bpm_current!=new_bpm) {
    bpm_current = new_bpm;
    #ifdef USE_UCLOCK
      uClock.setTempo(bpm_current);
      micros_per_tick = (float)1000000.0f * ((float)60.0f / (float)(bpm_current * (float)PPQN));
    #else
      //ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
      micros_per_tick = (float)1000000.0f * ((float)60.0f / (float)(bpm_current * (float)PPQN));
      //Serial.printf("%i bpm is %0.4f beats per second?\n", bpm_current, bpm_current/60.0f);
      //Serial.printf("set ms_per_tick to %f\n", ms_per_tick);
      //printf("set micros_per_tick to %f\n", micros_per_tick);
    #endif
    //Serial.print(F("set bpm to "));
    //Serial.println(bpm_current);
  }
}
float get_bpm() {
  return bpm_current;
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

// beat number of bar, counting from 0, for given tick number
int beat_number_from_ticks(signed long ticks) {
    return BPM_GLOBAL_BEAT_FROM_TICKS(ticks) % BEATS_PER_BAR;
}

// step number of bar, counting from 0, for given tick number
int step_number_from_ticks(signed long ticks) {
    return BPM_GLOBAL_STEP_FROM_TICKS(ticks) % STEPS_PER_BAR;
}

#ifdef ENABLE_TIME_SIGNATURE
  // time signature stuff..
  // first/top number ie how many beats in a bar, second/bottom number is what kind of note gets the beat (e.g. 4 for quarter note, 8 for eighth note etc)

  time_sig_t current_time_signature = {
    DEFAULT_TIME_SIGNATURE_NUMERATOR, 
    DEFAULT_TIME_SIGNATURE_DENOMINATOR
  };
  uint8_t time_signature_numerator = DEFAULT_TIME_SIGNATURE_NUMERATOR;

  // Tick count at the moment the current time signature started.
  // All modulo-based bar/beat/phrase calculations use (ticks - ts_phase_offset)
  // so the grid resets cleanly when a new time signature is applied mid-stream.
  uint32_t ts_phase_offset = 0;

#endif