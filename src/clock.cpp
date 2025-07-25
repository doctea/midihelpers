#include "clock.h"
//#include "midi/midi_outs.h"
#include <Arduino.h>

#if defined(USE_UCLOCK) 
    #include "uClock.h"
    #if defined(CORE_TEENSY)
        #include <util/atomic.h>
        #define USE_ATOMIC
    #elif defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
        #include "SimplyAtomic.h"
        #define ATOMIC_BLOCK(X) ATOMIC()
      #define USE_ATOMIC
    #endif
#endif

#if __has_include("debug.h")
  #include "debug.h"
#endif

#ifndef CORE_TEENSY
    #define FLASHMEM
#endif

volatile int missed_micros; // for tracking how many microseconds late we are processing a tick

volatile ClockMode clock_mode = DEFAULT_CLOCK_MODE;

void (*__global_restart_callback)();

void set_global_restart_callback(void(*global_restart_callback)()) {
    __global_restart_callback = global_restart_callback;
}

/// use cheapclock clock
volatile uint32_t last_ticked_at_micros = micros();
#ifdef USE_UCLOCK
  FLASHMEM void setup_uclock(void(*do_tick)(uint32_t), umodular::clock::uClockClass::PPQNResolution uclock_internal_ppqn) {
    /*//uClock 1.5.1 version
    uClock.init();
    uClock.setClock96PPQNOutput(do_tick);
    uClock.setTempo(bpm_current);*/
    //uClock <=2.2.0 version
    uClock.setInputPPQN(uclock_internal_ppqn);
    uClock.init();
    //uClock.setPPQN(uClock.PPQN_96);
    uClock.setOnSync24(do_tick);  // tick at PPQN // TODO: tick faster than this rate and then clock divide in do_tick so that we can implement clock multiplying!
    uClock.setTempo(bpm_current);
    
    clock_reset();
  }
#else
  FLASHMEM void setup_cheapclock() {
    clock_reset();
    set_bpm(bpm_current);
  }
#endif

void messages_log_add(String msg);

volatile bool usb_midi_clock_ticked = false;
volatile unsigned long last_usb_midi_clock_ticked_at;
void pc_usb_midi_handle_clock() {
  if(!playing)
    return;

  if (clock_mode==CLOCK_EXTERNAL_USB_HOST && usb_midi_clock_ticked) {
      if (Serial) Serial.printf("WARNING: received a usb midi clock tick at %u, but last one from %u was not yet processed (didn't process within gap of %u)!\n", millis(), last_usb_midi_clock_ticked_at, millis()-last_usb_midi_clock_ticked_at);
      #if defined(ENABLE_SCREEN) && __has_include("menu_messages.h")
        messages_log_add("WARNING: received a usb midi clock tick, but last one was not yet processed!");
      #endif
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
  #if defined(ENABLE_SCREEN) && __has_include("menu_messages.h")
    messages_log_add("pc_usb_midi_handle_start()!");
  #endif
  // see function "auto_handle_start" when you wanna make this automatically change clock mode when receiving a start message

  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    //tap_tempo_tracker.reset();
    #ifdef USE_ATOMIC
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    #endif
    {
      //clock_reset();
      if (!playing)
        clock_start();
      if (__global_restart_callback!=nullptr)
          __global_restart_callback();
    }
  }
}
void pc_usb_midi_handle_stop() {
  //if (Serial) Serial.println("pc_usb_midi_handle_stop()"); Serial.flush();
  #if defined(ENABLE_SCREEN) && __has_include("menu_messages.h")
    messages_log_add("pc_usb_midi_handle_stop()!");
  #endif
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    if (!playing) {
      clock_reset();
      //if (__global_restart_callback!=nullptr)
      //  __global_restart_callback();
    }
    //if (playing)
      clock_stop();
  }
}
void pc_usb_midi_handle_continue() {
  //if (Serial) Serial.println("pc_usb_midi_handle_continue()"); Serial.flush();
  #if defined(ENABLE_SCREEN) && __has_include("menu_messages.h")
    messages_log_add("pc_usb_midi_handle_continue()!");
  #endif

  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    if (!playing)
      clock_continue();
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
      clock_reset();
      clock_start();
      if (__global_restart_callback!=nullptr)
          __global_restart_callback();
    }
  }
  void din_midi_handle_stop() {
    if (clock_mode==CLOCK_EXTERNAL_MIDI_DIN) {
      if (!playing) {
        clock_reset();
        if (__global_restart_callback!=nullptr)
          __global_restart_callback();
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
  #ifdef USE_UCLOCK
    static unsigned long last_reported_tick = -1;
  #endif
  static volatile unsigned long last_ticked = 0;
  __UINT_FAST32_TYPE__ mics = micros();
  if (!playing) 
    return false;

  if (clock_mode==CLOCK_EXTERNAL_USB_HOST && /*playing && */check_and_unset_pc_usb_midi_clock_ticked()) {
    #ifdef USE_UCLOCK
      // don't do anything -- ticks is set by uClock's callback
      //if (ticks==last_processed_tick) // don't process the same tick twice?
      //  return false;
    #else
      ticks++;
    #endif
    return true;
  }
  #ifdef ENABLE_CLOCK_INPUT_MIDI_DIN
    else if (clock_mode==CLOCK_EXTERNAL_MIDI_DIN && check_and_unset_din_midi_clock_ticked()) {
      ticks++;
      return true;
    }
  #endif
  #ifdef ENABLE_CLOCK_INPUT_CV
    else if (clock_mode==CLOCK_EXTERNAL_CV && check_and_unset_cv_clock_ticked()) {
      ticks += external_cv_ticks_per_pulse;
      return true;
    }
  #endif
  #ifdef USE_UCLOCK
    else if (clock_mode==CLOCK_INTERNAL && playing && ticks != last_reported_tick) {
      missed_micros = (mics - last_ticked - micros_per_tick);
      last_reported_tick = ticks;
      last_ticked = mics;
      last_ticked_at_micros = mics;
      return true;
    }
  #else
    else if (clock_mode==CLOCK_INTERNAL && playing && mics - last_ticked >= micros_per_tick) {
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
  #endif

  return false;
}

void clock_set_playing(bool p = true) {
  playing = p;
}

void clock_start() {
  //if (Serial) Serial.println("clock_start()"); Serial.flush();

  #ifdef USE_ATOMIC
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
  #endif
  {
    #ifdef USE_UCLOCK
      if (!playing)
        uClock.start();
    #endif

    clock_set_playing(true);
  }
}
void clock_stop() {
  //if (Serial) Serial.println("clock_stop()"); Serial.flush();

  #ifdef USE_ATOMIC
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
  #endif
  //Serial.println("clock_stop()"); Serial.flush();
  {
    bool should_reset_clock = !playing;

    clock_set_playing(false);

    #ifdef USE_UCLOCK
      uClock.pause();
    #endif

    if (should_reset_clock) {
      //Serial.println("not playing - calling clock_reset()"); Serial.flush();
      clock_reset();
    }
    //Serial.println("about to call clock_set_playing()"); Serial.flush();
    //clock_set_playing(false);
    //Serial.println("called clock_set_playing()"); Serial.flush();
  }
}
void clock_continue() {
  //if (Serial) Serial.println("clock_continue()"); Serial.flush();
  #ifdef USE_ATOMIC
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
  #endif
  {
    #ifdef USE_UCLOCK
      if (playing) {
        uClock.pause();
        //Serial.printf("uClock paused at %u\n", millis());
      } else {
        uClock.continue_playing();
        //Serial.printf("uClock continued at %u\n", millis());
      }
      clock_set_playing(!playing);
    #else
      clock_set_playing(true);
    #endif
  }
};

void clock_reset() {
  //if (Serial) { Serial.println("clock_reset()"); Serial.flush(); }
  #ifdef USE_ATOMIC
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
  #endif
  {
    #ifdef USE_UCLOCK
      uClock.resetCounters();
    #endif
    
    ticks = 0;
  }
}

void change_clock_mode(ClockMode new_mode) {
  if(clock_mode!=new_mode) {
    if(__clock_mode_changed_callback!=nullptr)
      __clock_mode_changed_callback(clock_mode, new_mode);
    
    #ifdef USE_UCLOCK
      #ifdef USE_ATOMIC
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
      #endif
      {
        bool was_playing = playing;
        //uClock.stop();
        if (new_mode==ClockMode::CLOCK_INTERNAL) {
          uClock.setClockMode(uClock.ClockMode::INTERNAL_CLOCK);
        } else {
          bool was_started = playing;
          if (was_started) uClock.stop();
          uClock.setClockMode(uClock.ClockMode::EXTERNAL_CLOCK);
          if (was_started) uClock.continue_playing();
        }
        //if (was_playing) uClock.start();
      }
    #endif 

    clock_mode = new_mode;
  }
}

