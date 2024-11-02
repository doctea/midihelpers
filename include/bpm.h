#ifndef BPM__INCLUDED
#define BPM__INCLUDED

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <cinttypes>
#endif

#ifndef PPQN
  #define PPQN  24
#endif
#define BEATS_PER_BAR   4
#define BARS_PER_PHRASE 4
#define BEATS_PER_PHRASE  (BEATS_PER_BAR * BARS_PER_PHRASE)
#define STEPS_PER_BEAT  4
#define STEPS_PER_BAR     (BEATS_PER_BAR * STEPS_PER_BEAT)
#define STEPS_PER_PHRASE  (STEPS_PER_BAR * BARS_PER_PHRASE)
#define TICKS_PER_STEP    (PPQN / STEPS_PER_BEAT)
#define TICKS_PER_BAR     (PPQN * BEATS_PER_BAR)
#define TICKS_PER_PHRASE  (TICKS_PER_BAR * BARS_PER_PHRASE)

#define BPM_MINIMUM   60.0
#define BPM_MAXIMUM   180.0

#define LOOP_LENGTH_TICKS (PPQN * BEATS_PER_BAR * BARS_PER_PHRASE)    // how many ticks does the loop last?
#define LOOP_LENGTH_STEP_SIZE 1         // resolution of loop TODO: problems when this is set >1; reloaded sequences (or maybe its converted-from-bitmap stage?) are missing note-offs
#define LOOP_LENGTH_STEPS (LOOP_LENGTH_TICKS/LOOP_LENGTH_STEP_SIZE) // how many steps are recorded per loop

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

#define BPM_CURRENT_PHRASE          (ticks / (TICKS_PER_PHRASE))
#define BPM_CURRENT_BAR_OF_PHRASE   (ticks % (PPQN*BEATS_PER_BAR*BARS_PER_PHRASE) / (PPQN*BEATS_PER_BAR))
#define BPM_CURRENT_BEAT_OF_BAR     (ticks % (PPQN*BEATS_PER_BAR) / PPQN)
#define BPM_CURRENT_STEP_OF_BAR     (ticks % (PPQN*BEATS_PER_BAR) / (PPQN/STEPS_PER_BEAT))
#define BPM_CURRENT_STEP_OF_PHRASE  (ticks % (PPQN*BEATS_PER_BAR*BARS_PER_PHRASE) / (PPQN/STEPS_PER_BEAT))
#define BPM_CURRENT_BEAT_OF_PHRASE  (ticks % (PPQN*BEATS_PER_BAR*BARS_PER_PHRASE) / (PPQN))
#define BPM_CURRENT_TICK_OF_BEAT    (ticks % PPQN)

int beat_number_from_ticks(signed long ticks);
int step_number_from_ticks(signed long ticks);

bool is_bpm_on_phrase(uint32_t ticks,      unsigned long offset = 0);
bool is_bpm_on_half_phrase(uint32_t ticks, unsigned long offset = 0);
bool is_bpm_on_bar(uint32_t    ticks,      unsigned long offset = 0);
bool is_bpm_on_half_bar(uint32_t  ticks,   unsigned long offset = 0);
bool is_bpm_on_beat(uint32_t  ticks,       unsigned long offset = 0);
bool is_bpm_on_eighth(uint32_t  ticks,     unsigned long offset = 0);
bool is_bpm_on_sixteenth(uint32_t  ticks,  unsigned long offset = 0);
bool is_bpm_on_thirtysecond(uint32_t  ticks,  unsigned long offset = 0);

bool is_bpm_on_multiplier(unsigned long ticks, float multiplier, unsigned long offset = 0);

void set_bpm(float new_bpm, bool temporary = false);
float get_bpm();
void set_restart_on_next_bar(bool v = true);
void set_restart_on_next_bar_on();
bool is_restart_on_next_bar();

#endif
