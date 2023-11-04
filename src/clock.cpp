#include "clock.h"
//#include "midi/midi_outs.h"

#ifndef CORE_TEENSY
    #define FLASHMEM
#endif


int missed_micros; // for tracking how many microseconds late we are processing a tick

volatile ClockMode clock_mode = DEFAULT_CLOCK_MODE;

void (*__global_restart_callback)();

void set_global_restart_callback(void(*global_restart_callback)()) {
    __global_restart_callback = global_restart_callback;
}

/// use cheapclock clock
volatile uint32_t last_ticked_at_micros = micros();
#ifdef USE_UCLOCK
  FLASHMEM void setup_uclock(void(*do_tick)(uint32_t)) {
    ticks = 0;
    uClock.init();
    uClock.setClock96PPQNOutput(do_tick);
    uClock.setTempo(bpm_current);
    uClock.start();
  }
#else
  FLASHMEM void setup_cheapclock() {
    ticks = 0;
    set_bpm(bpm_current);
  }
#endif

volatile bool usb_midi_clock_ticked = false;
volatile unsigned long last_usb_midi_clock_ticked_at;
void pc_usb_midi_handle_clock() {
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST && usb_midi_clock_ticked) {
        Serial.printf("WARNING: received a usb midi clock tick at %u, but last one from %u was not yet processed (didn't process within gap of %u)!\n", millis(), last_usb_midi_clock_ticked_at, millis()-last_usb_midi_clock_ticked_at);
    }
    /*if (CLOCK_EXTERNAL_USB_HOST) {  // TODO: figure out why tempo estimation isn't working and fix
        tap_tempo_tracker.push_beat();
    }*/
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
      last_usb_midi_clock_ticked_at = millis();
      usb_midi_clock_ticked = true;
      #ifdef USE_UCLOCK
        uClock.clockMe();
      #endif
    }
}

void pc_usb_midi_handle_start() {
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    //tap_tempo_tracker.reset();
    clock_start();
    if (__global_restart_callback!=nullptr)
        __global_restart_callback();
    ticks = 0;
  }
}
void pc_usb_midi_handle_stop() {
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    if (!playing) {
      if (__global_restart_callback!=nullptr)
        __global_restart_callback();
      ticks = 0;
    }
    clock_stop();
  }
}
void pc_usb_midi_handle_continue() {
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    clock_start();
  }
}


bool check_and_unset_pc_usb_midi_clock_ticked() {
    bool v = usb_midi_clock_ticked;
    usb_midi_clock_ticked = false;
    /*if(clock_mode==CLOCK_EXTERNAL_USB_HOST && ticks%PPQN==0) {  // TODO: figure out why this isn't working and fix
        set_bpm(tap_tempo_tracker.bpm_calculate_current());
    }*/
    return v;
}


#ifdef ENABLE_CLOCK_INPUT_MIDI_DIN
  volatile bool din_midi_clock_ticked = false;
  volatile unsigned long last_din_midi_clock_ticked_at;

  void din_midi_handle_clock() {
    if (clock_mode==CLOCK_EXTERNAL_MIDI_DIN && usb_midi_clock_ticked) {
        Serial.printf("WARNING: received a usb midi clock tick at %u, but last one from %u was not yet processed (didn't process within gap of %u)!\n", millis(), last_usb_midi_clock_ticked_at, millis()-last_usb_midi_clock_ticked_at);
    }
    /*if (CLOCK_EXTERNAL_USB_HOST) {  // TODO: figure out why tempo estimation isn't working and fix
        tap_tempo_tracker.push_beat();
    }*/
    if (clock_mode==CLOCK_EXTERNAL_MIDI_DIN) {
      last_din_midi_clock_ticked_at = millis();
      din_midi_clock_ticked = true;
    }
  }

  void din_midi_handle_start() {
    if (clock_mode==CLOCK_EXTERNAL_MIDI_DIN) {
      //tap_tempo_tracker.reset();
      clock_start();
      if (__global_restart_callback!=nullptr)
          __global_restart_callback();
      ticks = 0;
    }
  }
  void din_midi_handle_stop() {
    if (clock_mode==CLOCK_EXTERNAL_MIDI_DIN) {
      if (!playing) {
        if (__global_restart_callback!=nullptr)
          __global_restart_callback();
        ticks = 0;
      }
      clock_stop();
    }
  }
  void din_midi_handle_continue() {
    if (clock_mode==CLOCK_EXTERNAL_MIDI_DIN) {
      clock_start();
    }
  }

bool check_and_unset_din_midi_clock_ticked() {
    bool v = din_midi_clock_ticked;
    din_midi_clock_ticked = false;
    /*if(clock_mode==CLOCK_EXTERNAL_USB_HOST && ticks%PPQN==0) {  // TODO: figure out why this isn't working and fix
        set_bpm(tap_tempo_tracker.bpm_calculate_current());
    }*/
    return v;
}
#endif




void(*__clock_mode_changed_callback)(ClockMode old_mode, ClockMode new_mode) = nullptr;
void set_clock_mode_changed_callback(void(*callback)(ClockMode old_mode, ClockMode new_mode)) {
  __clock_mode_changed_callback = callback;
}


#ifdef ENABLE_CLOCK_INPUT_CV
  bool(*check_cv_clock_ticked_callback)(void) = nullptr;
  bool cv_clock_ticked = false;
  uint32_t external_cv_ticks_per_pulse = PPQN;
  bool check_and_unset_cv_clock_ticked() {
    if (check_cv_clock_ticked_callback==nullptr)
      return false;

    // use a callback to do the actual check on whether input is high
    bool v = check_cv_clock_ticked_callback();
    if (v)
      cv_clock_ticked = true;

    bool retval = cv_clock_ticked;
    cv_clock_ticked = false;

    return retval;
  }
  void set_check_cv_clock_ticked_callback(bool(*check_cv_clock_ticked_callback_to_set)(void)) {
    check_cv_clock_ticked_callback = check_cv_clock_ticked_callback_to_set;
  }
#endif

bool update_clock_ticks() {
  static unsigned long last_ticked = 0;
  __UINT_FAST32_TYPE__ mics = micros();
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST && /*playing && */check_and_unset_pc_usb_midi_clock_ticked()) {
    ticks++;
    return true;
  #ifdef ENABLE_CLOCK_INPUT_MIDI_DIN
  } else if (clock_mode==CLOCK_EXTERNAL_MIDI_DIN && check_and_unset_din_midi_clock_ticked()) {
    ticks++;
    return true;
  #endif
  #ifdef ENABLE_CLOCK_INPUT_CV
  } else if (clock_mode==CLOCK_EXTERNAL_CV && check_and_unset_cv_clock_ticked()) {
    ticks += external_cv_ticks_per_pulse;
    return true;
  #endif
  } else if (clock_mode==CLOCK_INTERNAL && playing && mics - last_ticked >= micros_per_tick) {
    ticks++;
    missed_micros = (mics - last_ticked - micros_per_tick);
    last_ticked = mics;
    last_ticked_at_micros = mics;

    /*if (is_bpm_on_beat(ticks)) {
        Serial.printf("beat %i!\n", ticks / PPQN);
        Serial.flush();
    }*/
    return true;
  }
  return false;
}

void clock_set_playing(bool p = true) {
  playing = p;
}

void clock_start() {
  #ifdef USE_UCLOCK
    uClock.start();
  #endif

  clock_set_playing(true);
}
void clock_stop() {
  #ifdef USE_UCLOCK
    uClock.pause();
  #endif

  if (!playing)
    ticks = 0;
  clock_set_playing(false);
}
void clock_continue() {
  #ifdef USE_UCLOCK
    uClock.pause();
  #endif

  clock_set_playing(true);
};