#ifndef BPM__INCLUDED
#define BPM__INCLUDED

#include <Arduino.h>

#ifndef PPQN
  #define PPQN  24
#endif
#define BEATS_PER_BAR   4
#define BARS_PER_PHRASE 4
#define STEPS_PER_BEAT  4
#define STEPS_PER_BAR   (BEATS_PER_BAR*STEPS_PER_BEAT)
#define STEPS_PER_PHRASE (STEPS_PER_BAR * BARS_PER_PHRASE)

#define BPM_MINIMUM   60.0
#define BPM_MAXIMUM   180.0

#define LOOP_LENGTH_TICKS (PPQN*4*4)    // how many ticks does the loop last?
#define LOOP_LENGTH_STEP_SIZE 1         // resolution of loop TODO: problems when this is set >1; reloaded sequences (or maybe its converted-from-bitmap stage?) are missing note-offs
#define LOOP_LENGTH_STEPS (LOOP_LENGTH_TICKS/LOOP_LENGTH_STEP_SIZE) // how many steps are recorded per loop

volatile extern bool playing;
extern bool single_step;

extern bool restart_on_next_bar;

volatile extern uint32_t ticks; // = 0;
volatile extern long last_processed_tick;

// tracking which beat we're on
volatile extern float bpm_current; //BPM_MINIMUM; //60.0f;
#ifndef USE_UCLOCK
  //extern double ms_per_tick; // = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
  volatile extern float micros_per_tick; // = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
#endif

#define BPM_CURRENT_PHRASE          (ticks / (PPQN*BEATS_PER_BAR*BARS_PER_PHRASE))
#define BPM_CURRENT_BAR_OF_PHRASE   (ticks % (PPQN*BEATS_PER_BAR*BARS_PER_PHRASE) / (PPQN*BEATS_PER_BAR))
#define BPM_CURRENT_BEAT_OF_BAR     (ticks % (PPQN*BEATS_PER_BAR) / PPQN)
#define BPM_CURRENT_STEP_OF_BAR     (ticks % (PPQN*BEATS_PER_BAR) / (PPQN/STEPS_PER_BEAT))
#define BPM_CURRENT_STEP_OF_PHRASE  (ticks % (PPQN*BEATS_PER_BAR*BARS_PER_PHRASE) / (PPQN/STEPS_PER_BEAT))

inline bool is_bpm_on_phrase(uint32_t ticks,      unsigned long offset = 0) { return ticks==offset || ticks%(PPQN*BEATS_PER_BAR*BARS_PER_PHRASE) == offset; }
inline bool is_bpm_on_bar(uint32_t    ticks,      unsigned long offset = 0) { return ticks==offset || ticks%(PPQN*BEATS_PER_BAR)   == offset; }
inline bool is_bpm_on_half_bar(uint32_t  ticks,   unsigned long offset = 0) { return ticks==offset || ticks%(PPQN*(BEATS_PER_BAR/2))   == offset; }
inline bool is_bpm_on_beat(uint32_t  ticks,       unsigned long offset = 0) { return ticks==offset || ticks%(PPQN)     == offset; }
inline bool is_bpm_on_eighth(uint32_t  ticks,     unsigned long offset = 0) { return ticks==offset || ticks%(PPQN/(STEPS_PER_BEAT/2))   == offset; }
inline bool is_bpm_on_sixteenth(uint32_t  ticks,  unsigned long offset = 0) { return ticks==offset || ticks%(PPQN/STEPS_PER_BEAT)   == offset; }

inline bool is_bpm_on_multiplier(unsigned long ticks, float multiplier, unsigned long offset = 0) {
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

void set_bpm(float new_bpm);
float get_bpm();
void set_restart_on_next_bar(bool v = true);
void set_restart_on_next_bar_on();
bool is_restart_on_next_bar();

#endif
