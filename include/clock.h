#ifndef CLOCK_H__INCLUDED
#define CLOCK_H__INCLUDED

#include "bpm.h"
//#include "midi/midi_outs.h"

/*#define CLOCK_INTERNAL          0
#define CLOCK_EXTERNAL_USB_HOST 1
#define CLOCK_CV                2
#define CLOCK_NONE              3
#define NUM_CLOCK_SOURCES       4*/

enum ClockMode {
  CLOCK_INTERNAL,
  CLOCK_EXTERNAL_USB_HOST,
  #ifdef ENABLE_CLOCK_INPUT_MIDI_DIN
    CLOCK_EXTERNAL_MIDI_DIN,
  #endif
  #ifdef ENABLE_CLOCK_INPUT_CV
    CLOCK_EXTERNAL_CV,
  #endif
  CLOCK_NONE,
  NUM_CLOCK_SOURCES
};

#ifndef DEFAULT_CLOCK_MODE
  #define DEFAULT_CLOCK_MODE CLOCK_INTERNAL
#endif

volatile extern ClockMode clock_mode;

volatile extern uint32_t last_ticked_at_micros;

#ifdef USE_UCLOCK
  #include <uClock.h>
  void setup_uclock(void(*do_tick)(uint32_t), umodular::clock::uClockClass::PPQNResolution uclock_internal_ppqn = uClock.PPQN_96);
#else 
  /// use cheapclock clock
  void setup_cheapclock();
#endif

void change_clock_mode(ClockMode new_mode);
void pc_usb_midi_handle_clock();
bool check_and_unset_pc_usb_midi_clock_ticked();

void pc_usb_midi_handle_start();
void pc_usb_midi_handle_stop();
void pc_usb_midi_handle_continue();

#ifdef ENABLE_CLOCK_INPUT_MIDI_DIN
  void din_midi_handle_clock();
  bool check_and_unset_din_midi_clock_ticked();

  void din_midi_handle_start();
  void din_midi_handle_stop();
  void din_midi_handle_continue();
#endif

#ifdef ENABLE_CLOCK_INPUT_CV
  extern uint32_t external_cv_ticks_per_pulse;
  bool check_and_unset_cv_clock_ticked();
  void set_check_cv_clock_ticked_callback(bool(*)(void));
#endif

extern void(*__clock_mode_changed_callback)(ClockMode old_mode, ClockMode new_mode);
void set_clock_mode_changed_callback(void(*)(ClockMode old_mode, ClockMode new_mode));

bool update_clock_ticks();

void set_global_restart_callback(void(*global_restart_callback)());

void clock_reset();
void clock_start();
void clock_stop();
void clock_continue();
void clock_set_playing(bool p);

// tap tempo stuff
#define CLOCK_TEMPO_HISTORY_MAX 4
#define CLOCK_TEMPO_RESTARTTHRESHOLD (3.f*1000000.f)
extern uint32_t clock_tempo_history[4];
extern uint32_t clock_last_tap;
extern int clock_tempo_history_pos;
void clock_tempo_tap();
float clock_tempo_estimate();
void clock_tempo_update();

#endif