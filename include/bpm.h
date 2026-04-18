#pragma once

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <cinttypes>
#endif

#ifndef PPQN
  #define PPQN  24
#endif

#define BPM_MINIMUM   60.0
#define BPM_MAXIMUM   180.0

#ifdef ENABLE_TIME_SIGNATURE

  typedef struct {
    uint8_t numerator;
    uint8_t denominator;
  } time_sig_t;

  // Global cache — fast read for hot paths (TIME_NUMERATOR / TIME_DENOMINATOR macros).
  // Conductor is the source of truth and keeps this in sync via set_time_signature().
  // Do not write directly; always go through Conductor when available.
  extern time_sig_t current_time_signature;
  inline time_sig_t get_time_signature() {
      return current_time_signature;
  }
  inline void set_time_signature(time_sig_t ts) {
      current_time_signature = ts;
  }
  inline uint8_t get_time_signature_numerator(void) { return current_time_signature.numerator; }
  inline uint8_t get_time_signature_denominator(void) { return current_time_signature.denominator; }
  // Direct cache writers — bypass Conductor. Prefer conductor->set_numerator/denominator().
  inline void set_time_signature_numerator(uint8_t v)   { current_time_signature.numerator   = v; }
  inline void set_time_signature_denominator(uint8_t v) { current_time_signature.denominator = v; }

  #define TIME_SIG_MAX_STEPS_PER_BAR 64 // TODO: assess whether this value should be increased; used by CircleDisplays and pattern parameter limits?

  #define DEFAULT_TIME_SIGNATURE_NUMERATOR 4
  #define DEFAULT_TIME_SIGNATURE_DENOMINATOR 4

  #define TIME_NUMERATOR      (get_time_signature_numerator())   // e.g. 7 for 7/8
  #define TIME_DENOMINATOR    (get_time_signature_denominator())   // e.g. 8 for 7/8

  #define BARS_PER_PHRASE     4
  // Grid resolution (e.g. 4 = 16th notes)
  #define STEPS_PER_BEAT      4

  #define BEATS_PER_BAR       (TIME_NUMERATOR)
  #define BEATS_PER_PHRASE    (BEATS_PER_BAR * BARS_PER_PHRASE)

  #define STEPS_PER_BAR       (STEPS_PER_BEAT * BEATS_PER_BAR)
  #define STEPS_PER_PHRASE    (STEPS_PER_BAR * BARS_PER_PHRASE)

  // Tick math
  #define TICKS_PER_BEAT      (PPQN * STEPS_PER_BEAT / TIME_DENOMINATOR)
  #define TICKS_PER_STEP      (TICKS_PER_BEAT / STEPS_PER_BEAT)
  #define TICKS_PER_BAR       (TICKS_PER_BEAT * BEATS_PER_BAR)
  #define TICKS_PER_PHRASE    (TICKS_PER_BAR * BARS_PER_PHRASE)

  #define LOOP_LENGTH_TICKS (TICKS_PER_PHRASE)    // how many ticks does the loop last?
  #define LOOP_LENGTH_STEP_SIZE 1         // resolution of loop TODO: problems when this is set >1; reloaded sequences (or maybe its converted-from-bitmap stage?) are missing note-offs
  #define LOOP_LENGTH_STEPS (LOOP_LENGTH_TICKS/LOOP_LENGTH_STEP_SIZE) // how many steps are recorded per loop

  // MAX_LOOP_LENGTH_STEPS must be the maximum possible value of LOOP_LENGTH_STEPS (= TICKS_PER_PHRASE / LOOP_LENGTH_STEP_SIZE).
  // LOOP_LENGTH_STEP_SIZE=1 means each slot is 1 tick, so the slot count equals TICKS_PER_PHRASE.
  // At standard denominator (DEFAULT_TIME_SIGNATURE_DENOMINATOR) and maximum numerator
  //   (TIME_SIG_MAX_STEPS_PER_BAR / STEPS_PER_BEAT beats/bar):
  //   MAX = PPQN * (TIME_SIG_MAX_STEPS_PER_BAR / STEPS_PER_BEAT) * BARS_PER_PHRASE / LOOP_LENGTH_STEP_SIZE
  //       = 24 * 16 * 4 / 1 = 1536
  // (Old incorrect definition was just TIME_SIG_MAX_STEPS_PER_BAR * BARS_PER_PHRASE = 256,
  //  which was in musical steps not ticks, causing frames[] out-of-bounds access in 4/4.)
  #define MAX_LOOP_LENGTH_STEPS ((PPQN * (TIME_SIG_MAX_STEPS_PER_BAR / STEPS_PER_BEAT) * BARS_PER_PHRASE) / LOOP_LENGTH_STEP_SIZE)  // maximum possible steps per loop given time signature limits
#else
  // old style of fixed 4/4 time signature, with PPQN=24 and 16 steps per bar (i.e. 16th notes)

  #define TIME_SIG_MAX_STEPS_PER_BAR 32 // think this should be enough for the old style 4/4, allowing up to 32 steps per pattern essentially
  #define TIME_SIG_MAX_STEPS_PER_PHRASE (TIME_SIG_MAX_STEPS_PER_BAR * BARS_PER_PHRASE)

  // #define STEPS_PER_BEAT  4
  // #define BEATS_PER_BAR   4
  // #define BARS_PER_PHRASE 4
  
  // #define BEATS_PER_PHRASE  (BEATS_PER_BAR * BARS_PER_PHRASE)
  // #define STEPS_PER_BAR     (BEATS_PER_BAR * STEPS_PER_BEAT)
  // #define STEPS_PER_PHRASE  (STEPS_PER_BAR * BARS_PER_PHRASE)
  
  // #define TICKS_PER_BEAT    (PPQN)
  // #define TICKS_PER_STEP    (PPQN / STEPS_PER_BEAT)
  // #define TICKS_PER_BAR     (PPQN * BEATS_PER_BAR)
  // #define TICKS_PER_PHRASE  (TICKS_PER_BAR * BARS_PER_PHRASE)

  // #define LOOP_LENGTH_TICKS (PPQN * BEATS_PER_BAR * BARS_PER_PHRASE)    // how many ticks does the loop last?
  // #define LOOP_LENGTH_STEP_SIZE 1         // resolution of loop TODO: problems when this is set >1; reloaded sequences (or maybe its converted-from-bitmap stage?) are missing note-offs
  // #define LOOP_LENGTH_STEPS (LOOP_LENGTH_TICKS/LOOP_LENGTH_STEP_SIZE) // how many steps are recorded per loop

  #define BEATS_PER_BAR   4
  #define BARS_PER_PHRASE 4
  #define BEATS_PER_PHRASE  (BEATS_PER_BAR * BARS_PER_PHRASE)
  #define STEPS_PER_BEAT  4
  #define STEPS_PER_BAR     (BEATS_PER_BAR * STEPS_PER_BEAT)
  #define STEPS_PER_PHRASE  (STEPS_PER_BAR * BARS_PER_PHRASE)
  #define TICKS_PER_STEP    (PPQN / STEPS_PER_BEAT)
  #define TICKS_PER_BAR     (PPQN * BEATS_PER_BAR)
  #define TICKS_PER_PHRASE  (TICKS_PER_BAR * BARS_PER_PHRASE)

  #define LOOP_LENGTH_TICKS (PPQN * BEATS_PER_BAR * BARS_PER_PHRASE)    // how many ticks does the loop last?
  #define LOOP_LENGTH_STEP_SIZE 1         // resolution of loop TODO: problems when this is set >1; reloaded sequences (or maybe its converted-from-bitmap stage?) are missing note-offs
  #define LOOP_LENGTH_STEPS (LOOP_LENGTH_TICKS/LOOP_LENGTH_STEP_SIZE) // how many steps are recorded per loop

  #define TICKS_PER_BEAT    (PPQN)

#endif

volatile extern bool playing;
extern bool single_step;

extern bool restart_on_next_bar;

volatile extern uint32_t ticks; // = 0;
volatile extern long last_processed_tick;

// tracking which beat we're on
volatile extern float bpm_current; //BPM_MINIMUM; //60.0f;
///#ifndef USE_UCLOCK
  //extern double ms_per_tick; // = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
  volatile extern float micros_per_tick; // = 1000.0f * (60.0f / (double)(bpm_current * (double)PPQN));
//#endif

#define BPM_CURRENT_BEAT            (ticks / PPQN)
#define BPM_CURRENT_BAR             (BPM_CURRENT_BEAT / BEATS_PER_BAR)
#define BPM_CURRENT_PHRASE          (BPM_CURRENT_BAR / BARS_PER_PHRASE)

#define BPM_CURRENT_STEP_OF_SONG    (ticks / (TICKS_PER_STEP))
#define BPM_CURRENT_BEAT_OF_SONG    (ticks / (TICKS_PER_BEAT))
#define BPM_CURRENT_BAR_OF_PHRASE   (ticks % (PPQN*BEATS_PER_BAR*BARS_PER_PHRASE) / (PPQN*BEATS_PER_BAR))
#define BPM_CURRENT_BEAT_OF_BAR     (ticks % (PPQN*BEATS_PER_BAR) / PPQN)
#define BPM_CURRENT_STEP_OF_BAR     (ticks % (PPQN*BEATS_PER_BAR) / (PPQN/STEPS_PER_BEAT))
#define BPM_CURRENT_STEP_OF_PHRASE  (ticks % (PPQN*BEATS_PER_BAR*BARS_PER_PHRASE) / (PPQN/STEPS_PER_BEAT))
#define BPM_CURRENT_BEAT_OF_PHRASE  (ticks % (PPQN*BEATS_PER_BAR*BARS_PER_PHRASE) / (PPQN))
#define BPM_CURRENT_TICK_OF_BEAT    (ticks % PPQN)

#define BPM_GLOBAL_BEAT_FROM_TICKS(ticks) ((ticks) / (TICKS_PER_BEAT))
#define BPM_GLOBAL_STEP_FROM_TICKS(ticks) ((ticks) / TICKS_PER_STEP)

int beat_number_from_ticks(signed long ticks);
int step_number_from_ticks(signed long ticks);

// inline so the compiler sees through the macro constants and eliminates
// per-call function overhead — 10 of these fire per ISR tick in MultiSequencer
inline bool is_bpm_on_phrase(uint32_t ticks,        unsigned long offset = 0) { return ticks==offset || ticks%(TICKS_PER_PHRASE) == offset; }
inline bool is_bpm_on_half_phrase(uint32_t ticks,   unsigned long offset = 0) { return ticks==offset || ticks%(TICKS_PER_PHRASE/2) == offset; }
inline bool is_bpm_on_bar(uint32_t ticks,           unsigned long offset = 0) { return ticks==offset || ticks%(TICKS_PER_BAR) == offset; }
inline bool is_bpm_on_half_bar(uint32_t ticks,      unsigned long offset = 0) { return ticks==offset || ticks%(TICKS_PER_BAR/2) == offset; }
inline bool is_bpm_on_beat(uint32_t ticks,          unsigned long offset = 0) { return ticks==offset || ticks%(TICKS_PER_BEAT) == offset; }
inline bool is_bpm_on_eighth(uint32_t ticks,        unsigned long offset = 0) { return ticks==offset || ticks%(TICKS_PER_BEAT/2) == offset; }
inline bool is_bpm_on_sixteenth(uint32_t ticks,     unsigned long offset = 0) { return ticks==offset || ticks%(TICKS_PER_STEP) == offset; }
inline bool is_bpm_on_thirtysecond(uint32_t ticks,  unsigned long offset = 0) { return ticks==offset || ticks%(TICKS_PER_STEP/2) == offset; }

bool is_bpm_on_multiplier(unsigned long ticks, float multiplier, unsigned long offset = 0);

void set_bpm(float new_bpm);
float get_bpm();
void set_restart_on_next_bar(bool v = true);
void set_restart_on_next_bar_on();
bool is_restart_on_next_bar();

