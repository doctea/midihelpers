#include "bpm.h"
#include "clock.h"

volatile uint32_t ticks = 0;
volatile float bpm_current = 120.0f; //BPM_MINIMUM; //60.0f;
//double ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
volatile float micros_per_tick = (float)1000000.0 * ((float)60.0 / (float)(bpm_current * (float)PPQN));

volatile long last_processed_tick = -1;

#ifdef USE_UCLOCK
  volatile bool playing = false;
#else
  volatile bool playing = true;
#endif
bool single_step = false;
bool restart_on_next_bar = false;


bool is_bpm_on_phrase(uint32_t ticks,      unsigned long offset) { return ticks==offset || ticks%(PPQN*BEATS_PER_BAR*BARS_PER_PHRASE) == offset; }
bool is_bpm_on_half_phrase(uint32_t ticks, unsigned long offset) { return ticks==offset || ticks%(PPQN*BEATS_PER_BAR*(BARS_PER_PHRASE/2)) == offset; }
bool is_bpm_on_bar(uint32_t    ticks,      unsigned long offset) { return ticks==offset || ticks%(PPQN*BEATS_PER_BAR)   == offset; }
bool is_bpm_on_half_bar(uint32_t  ticks,   unsigned long offset) { return ticks==offset || ticks%(PPQN*(BEATS_PER_BAR/2))   == offset; }
bool is_bpm_on_beat(uint32_t  ticks,       unsigned long offset) { return ticks==offset || ticks%(PPQN)     == offset; }
bool is_bpm_on_eighth(uint32_t  ticks,     unsigned long offset) { return ticks==offset || ticks%(PPQN/(STEPS_PER_BEAT/2))   == offset; }
bool is_bpm_on_sixteenth(uint32_t  ticks,  unsigned long offset) { return ticks==offset || ticks%(PPQN/STEPS_PER_BEAT)   == offset; }
bool is_bpm_on_thirtysecond(uint32_t  ticks,  unsigned long offset) { return ticks==offset || ticks%(PPQN/STEPS_PER_BEAT*2)   == offset; }

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

void set_bpm(float new_bpm, bool temporary) {
  if (bpm_current!=new_bpm) {
    if (!temporary) 
      bpm_current = new_bpm;
    #ifdef USE_UCLOCK
      //ATOMIC(
          //uClock.setTempo(new_bpm); //bpm_current * 24);
      //)
      //set_new_bpm(bpm_current);
      uClock.setTempo(new_bpm);
      micros_per_tick = (float)1000000.0 * ((float)60.0 / (float)(new_bpm * (float)PPQN));
    #else
      //ms_per_tick = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
      micros_per_tick = (float)1000000.0 * ((float)60.0 / (float)(new_bpm * (float)PPQN));
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

int beat_number_from_ticks(signed long ticks) {  // TODO: move this into midihelpers + make it a macro?
  return (ticks / PPQN) % BEATS_PER_BAR;
}
int step_number_from_ticks(signed long ticks) {  // TODO: move this into midihelpers + make it a macro?
  return (ticks / (PPQN)) % (STEPS_PER_BAR/2);
}
