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

static bool is_external_clock_mode(ClockMode mode) {
  return mode != CLOCK_INTERNAL && mode != CLOCK_NONE;
}

void (*__global_restart_callback)();
void (*__global_stop_callback)();
void (*__external_clock_stall_callback)(bool stalled) = nullptr;

void set_global_restart_callback(void(*global_restart_callback)()) {
    __global_restart_callback = global_restart_callback;
}
void set_global_stop_callback(void(*global_stop_callback)()) {
    __global_stop_callback = global_stop_callback;
}
void set_external_clock_stall_callback(void(*callback)(bool stalled)) {
  __external_clock_stall_callback = callback;
}

/// use cheapclock clock
volatile uint32_t last_ticked_at_micros = micros();
#ifdef USE_UCLOCK
  static void (*uclock_sync_callback)(uint32_t) = nullptr;
  static volatile uint32_t external_clock_received_pulses = 0;
  static volatile uint32_t external_clock_delivered_sync_ticks = 0;
  static volatile uint32_t external_clock_rejected_pulses = 0;
  static volatile uint32_t external_clock_adapter_overflow = 0;

  static void counted_uclock_sync(uint32_t tick) {
    if (uClock.getClockMode() == umodular::clock::uClockClass::EXTERNAL_CLOCK)
      external_clock_delivered_sync_ticks++;
    if (uclock_sync_callback != nullptr)
      uclock_sync_callback(tick);
  }

  FLASHMEM void setup_uclock(void(*do_tick)(uint32_t), umodular::clock::uClockClass::PPQNResolution uclock_internal_ppqn) {

    #ifdef UCLOCK_HAS_STRICT_EXTERNAL_MODE
      uClock.setStrictExternalMode(true); // set strict external mode to true by default
    #endif

    // TODO: remove this hardcoded *4 and make it consistent with the rest of the code; including making shuffleeditor respect the actual PPQN setting
    uClock.setOutputPPQN((umodular::clock::uClockClass::PPQNResolution)(uclock_internal_ppqn * 4));
    #ifdef ENABLE_CLOCK_INPUT_CV
      if (clock_mode == CLOCK_EXTERNAL_CV)
        uClock.setInputPPQN(DEFAULT_CV_PPQN);
      else
    #endif
        uClock.setInputPPQN(umodular::clock::uClockClass::PPQN_24);
    uClock.setClockMode(is_external_clock_mode(clock_mode)
      ? umodular::clock::uClockClass::EXTERNAL_CLOCK
      : umodular::clock::uClockClass::INTERNAL_CLOCK);
    uClock.setExtIntervalBuffer(16); // 16 is the default size
    uclock_sync_callback = do_tick;
    uClock.setOnSync(umodular::clock::uClockClass::PPQNResolution::PPQN_24, counted_uclock_sync);
    // uClock.setOnOutputPPQN(do_tick);
    uClock.init();
    uClock.setTempo(bpm_current);
    
    clock_reset();
  }
#else
  FLASHMEM void setup_cheapclock() {
    clock_reset();
    set_bpm(bpm_current);
  }
#endif

void messages_log_add(const char* msg);

volatile bool usb_midi_clock_ticked = false;
#ifdef ENABLE_CLOCK_INPUT_CV
  volatile bool cv_clock_ticked = false;
  volatile bool cv_clock_reset = false;
#endif

// Armed-but-waiting flag: set when the user presses Start/Play in an external
// clock mode but no pulse has arrived yet.  Cleared when the first incoming
// clock pulse triggers actual playback (or when stopped / mode changed).
volatile bool waiting_for_external_clock = false;

static bool is_active_external_source(ClockMode source) {
  if (!playing || clock_mode != source)
    return false;

  return source != CLOCK_INTERNAL && source != CLOCK_NONE;
}

static bool receive_external_pulse(ClockMode source, uint32_t observed_at_us,
                                   bool has_observed_timestamp) {
  if (!is_active_external_source(source)) {
    #ifdef USE_UCLOCK
      external_clock_rejected_pulses++;
    #endif
    return false;
  }

  waiting_for_external_clock = false;
  if (source == CLOCK_EXTERNAL_USB_HOST)
    usb_midi_clock_ticked = true;
  #ifdef ENABLE_CLOCK_INPUT_CV
    if (source == CLOCK_EXTERNAL_CV)
      cv_clock_ticked = true;
  #endif

  #ifdef USE_UCLOCK
    external_clock_received_pulses++;
    if (has_observed_timestamp)
      uClock.clockMeAt(observed_at_us);
    else
      uClock.clockMe();
  #endif
  return true;
}

bool clock_receive_external_pulse(ClockMode source) {
  return receive_external_pulse(source, 0, false);
}

bool clock_receive_external_pulse_at(ClockMode source, uint32_t observed_at_us) {
  // Serial.printf("clock_receive_external_pulse_at() called with source %d and observed_at_us %lu\n", source, observed_at_us);
  return receive_external_pulse(source, observed_at_us, true);
}

bool clock_receive_external_reset(ClockMode source) {
  // Serial.printf("clock_receive_external_reset() called with source %d\n", source);
  if (clock_mode != source)
    return false;

  clock_reset();
  return true;
}

void clock_report_external_event_overflow() {
  #ifdef USE_UCLOCK
    external_clock_adapter_overflow++;
  #endif
}

void pc_usb_midi_handle_clock() {
  clock_receive_external_pulse(CLOCK_EXTERNAL_USB_HOST);
}

ExternalClockDiagnostics get_external_clock_diagnostics() {
  ExternalClockDiagnostics diagnostics;
  diagnostics.source = clock_mode;
  #ifdef USE_UCLOCK
    diagnostics.input_ppqn = uClock.input_ppqn;
  #else
    diagnostics.input_ppqn = 0;
  #endif
  #if defined(USE_UCLOCK) && defined(USE_ATOMIC)
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      diagnostics.received_pulses = external_clock_received_pulses;
      diagnostics.delivered_sync_ticks = external_clock_delivered_sync_ticks;
      diagnostics.rejected_pulses = external_clock_rejected_pulses;
      diagnostics.adapter_overflow = external_clock_adapter_overflow;
    }
  #elif defined(USE_UCLOCK)
    diagnostics.received_pulses = external_clock_received_pulses;
    diagnostics.delivered_sync_ticks = external_clock_delivered_sync_ticks;
    diagnostics.rejected_pulses = external_clock_rejected_pulses;
    diagnostics.adapter_overflow = external_clock_adapter_overflow;
  #else
    diagnostics.received_pulses = 0;
    diagnostics.delivered_sync_ticks = 0;
    diagnostics.rejected_pulses = 0;
    diagnostics.adapter_overflow = 0;
  #endif
  diagnostics.expected_sync_ticks = diagnostics.input_ppqn > 0
    ? (uint32_t)(((uint64_t)diagnostics.received_pulses * 24) /
                 diagnostics.input_ppqn)
    : 0;
  return diagnostics;
}

void pc_usb_midi_handle_start() {
  #if defined(ENABLE_SCREEN) && __has_include("menu_messages.h")
    messages_log_add("pc_usb_midi_handle_start()!");
  #endif
  // see function "auto_handle_start" when you wanna make this automatically change clock mode when receiving a start message

  Serial.println("==== pc_usb_midi_handle_start() called");

  if (clock_mode!=CLOCK_EXTERNAL_USB_HOST) {
    // automatically switch to using external USB clock if we receive a START message from the usb host
    change_clock_mode(CLOCK_EXTERNAL_USB_HOST);
  }

  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    // MIDI spec: START always means "go to position 0 and play", even if already playing.
    // Stop cleanly first, then reset position, then start.
    // clock_stop();
    clock_reset();
    clock_start();
    if (__global_restart_callback!=nullptr)
        __global_restart_callback();
  }
}
void pc_usb_midi_handle_stop() {
  //if (Serial) Serial.println("pc_usb_midi_handle_stop()"); Serial.flush();
  #if defined(ENABLE_SCREEN) && __has_include("menu_messages.h")
    messages_log_add("pc_usb_midi_handle_stop()!");
  #endif
  #ifdef SWITCH_TO_USB_CLOCK_ON_STOP
    if (clock_mode!=CLOCK_EXTERNAL_USB_HOST) {
      // automatically switch to using external USB clock if we receive a STOP message from the usb host
      change_clock_mode(CLOCK_EXTERNAL_USB_HOST);
    }
  #endif
  if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
    if (playing) {
      // MIDI spec: STOP freezes at current position; do not reset.
      clock_stop();
      if (__global_stop_callback!=nullptr)
          __global_stop_callback();
    } else {
      // Already stopped: optionally rewind to position 0.
      // This mirrors the behaviour of Ableton Live / hardware sequencers where a
      // second STOP press rewinds to bar 1.
      #ifdef STOP_WHILE_STOPPED_REWINDS
        uClock.stop();
        clock_reset();
        // Notify UI so panels like LoopMarkerPanel redraw at the new (zero) position.
        if (__global_stop_callback!=nullptr)
            __global_stop_callback();
      #endif
    }
  }
}
void pc_usb_midi_handle_continue() {
  //if (Serial) Serial.println("pc_usb_midi_handle_continue()"); Serial.flush();
  #if defined(ENABLE_SCREEN) && __has_include("menu_messages.h")
    messages_log_add("pc_usb_midi_handle_continue()!");
  #endif

  if (clock_mode!=CLOCK_EXTERNAL_USB_HOST) {
    // automatically switch to using external USB clock if we receive a START message from the usb host
    change_clock_mode(CLOCK_EXTERNAL_USB_HOST);
  }

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

  void din_midi_handle_clock() {
    /*if (CLOCK_EXTERNAL_USB_HOST) {  // TODO: figure out why tempo estimation isn't working and fix
        tap_tempo_tracker.push_beat();
    }*/
    if (clock_mode==CLOCK_EXTERNAL_MIDI_DIN) {
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
      if (playing) {
        // MIDI spec: STOP freezes at current position; do not reset.
        clock_stop();
      } else {
        // Already stopped: optionally rewind to position 0.
        #ifdef STOP_WHILE_STOPPED_REWINDS
          uClock.stop();
          clock_reset();
          // Notify UI so panels like LoopMarkerPanel redraw at the new (zero) position.
          if (__global_stop_callback!=nullptr)
              __global_stop_callback();
        #endif
      }
    }
  }
  void din_midi_handle_continue() {
    if (clock_mode==CLOCK_EXTERNAL_MIDI_DIN) {
      // MIDI spec: CONTINUE resumes from current position; do not reset.
      clock_continue();
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
  uint32_t external_cv_ticks_per_pulse = PPQN;
  bool check_and_unset_cv_clock_ticked() {
    if (check_cv_clock_ticked_callback==nullptr)
      return false;

    if (cv_clock_reset) {
      cv_clock_reset = false;
      clock_receive_external_reset(CLOCK_EXTERNAL_CV);
      return false;
    }

    if (check_cv_clock_ticked_callback())
      clock_receive_external_pulse(CLOCK_EXTERNAL_CV);

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
    static bool last_external_clock_stalled = false;
    bool external_clock_stalled = is_external_clock_mode(clock_mode) && playing &&
      uClock.isExternalClockStalled();
    if (external_clock_stalled != last_external_clock_stalled) {
      last_external_clock_stalled = external_clock_stalled;
      if (__external_clock_stall_callback != nullptr)
        __external_clock_stall_callback(external_clock_stalled);
    }
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
      //ticks += external_cv_ticks_per_pulse;
      // Serial.println("CV clock ticked!");
      // uClock.clockMe();
      // ticks++;

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
      // uClock.start() resets counters and transitions to STARTING (EXTERNAL_CLOCK)
      // or STARTED (INTERNAL_CLOCK).  For CLOCK_EXTERNAL_USB_HOST we are now in
      // EXTERNAL_CLOCK mode, so the ISR returns early until clockMe() drives the
      // state through SYNCING -> STARTED after enough pulses arrive.
      if (is_external_clock_mode(clock_mode))
        waiting_for_external_clock = true;
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
  {
    #ifdef STOP_WHILE_STOPPED_REWINDS
      if (!playing) {
        clock_reset();
      }
    #endif
    waiting_for_external_clock = false; // cancel any armed-but-waiting state
    clock_set_playing(false);

    #ifdef USE_UCLOCK
      // STARTED -> PAUSED preserves song position so that clock_continue() can resume.
      // STARTING/SYNCING have no meaningful position yet; use stop() for a clean abort.
      // Do not call pause() on PAUSED (it would toggle back to STARTING).
      auto cs = uClock.clock_state;
      if (cs == umodular::clock::uClockClass::ClockState::STARTED) {
        uClock.pause(); // STARTED -> PAUSED (preserves position)
      } else if (cs == umodular::clock::uClockClass::ClockState::STARTING ||
                 cs == umodular::clock::uClockClass::ClockState::SYNCING) {
        uClock.stop(); // abort sync-in-progress -> STOPPED
      }
    #endif
  }
}
void clock_continue() {
  //if (Serial) Serial.println("clock_continue()"); Serial.flush();
  #ifdef USE_ATOMIC
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
  #endif
  {
    #ifdef USE_UCLOCK
      if (is_external_clock_mode(clock_mode))
        waiting_for_external_clock = true;
      // uClock.pause() toggles PAUSED -> STARTING (EXTERNAL_CLOCK) or STARTED (INTERNAL_CLOCK).
      // If somehow in STOPPED state (e.g. CONTINUE before first START), use start() instead.
      if (uClock.clock_state == umodular::clock::uClockClass::ClockState::PAUSED) {
        uClock.pause(); // PAUSED -> STARTING/STARTED
      } else if (uClock.clock_state == umodular::clock::uClockClass::ClockState::STOPPED) {
        uClock.start();
      }
      // If already STARTED/STARTING/SYNCING: nothing to do.
      clock_set_playing(true);
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
    waiting_for_external_clock = false;
    #ifdef USE_UCLOCK
      uClock.resetCounters();
      external_clock_received_pulses = 0;
      external_clock_delivered_sync_ticks = 0;
      external_clock_rejected_pulses = 0;
      external_clock_adapter_overflow = 0;
    #endif
    
    ticks = 0;
    #ifdef ENABLE_TIME_SIGNATURE
      ts_phase_offset = 0;
    #endif
  }
}


#if defined(USE_UCLOCK) 
  umodular::clock::uClockClass::PPQNResolution external_cv_ppqn = DEFAULT_CV_PPQN;
  umodular::clock::uClockClass::PPQNResolution internal_ppqn    = DEFAULT_INTERNAL_PPQN;
#endif

void change_clock_mode(ClockMode new_mode) {
  if(clock_mode!=new_mode) {
    if(__clock_mode_changed_callback!=nullptr)
      __clock_mode_changed_callback(clock_mode, new_mode);
    waiting_for_external_clock = false; // cancel any armed-but-waiting state from the old mode
    
    #ifdef USE_UCLOCK
      bool was_playing = playing;
      #ifdef USE_ATOMIC
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
      #endif
      {
        if (new_mode==ClockMode::CLOCK_INTERNAL) {
          internal_ppqn = DEFAULT_INTERNAL_PPQN;
          uClock.setInputPPQN(internal_ppqn); //umodular::clock::uClockClass::PPQNResolution::PPQN_24);
          uClock.setClockMode(uClock.ClockMode::INTERNAL_CLOCK);
        } else {
          #ifdef ENABLE_CLOCK_INPUT_CV
            if (new_mode==ClockMode::CLOCK_EXTERNAL_CV) {
              external_cv_ppqn = DEFAULT_CV_PPQN;
              uClock.setInputPPQN(external_cv_ppqn);
            } else
          #endif
          {
            // MIDI Clock is always 24 PPQN. Do not inherit a previous CV or menu value.
            internal_ppqn = umodular::clock::uClockClass::PPQN_24;
            uClock.setInputPPQN(umodular::clock::uClockClass::PPQN_24);
          }
          uClock.setClockMode(umodular::clock::uClockClass::ClockMode::EXTERNAL_CLOCK);
        } 
      }
    #endif 

    clock_mode = new_mode;
    
    #ifdef USE_UCLOCK
      if (was_playing) {
        if (uClock.clock_state == umodular::clock::uClockClass::ClockState::PAUSED)
          uClock.pause();
        else if (uClock.clock_state == umodular::clock::uClockClass::ClockState::STOPPED)
          uClock.start();

        if (is_external_clock_mode(clock_mode))
          waiting_for_external_clock = true;
      }
      if (clock_mode==CLOCK_INTERNAL) 
        uClock.setTempo(bpm_current);
    #endif
  }
}

