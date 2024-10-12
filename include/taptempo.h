#pragma once

#include <Arduino.h>

// tap tempo stuff
#define CLOCK_TEMPO_HISTORY_MAX 4
#define CLOCK_TEMPO_RESTARTTHRESHOLD (3.f*1000000.f)

class TapTempoTracker {
  uint32_t clock_tempo_history[CLOCK_TEMPO_HISTORY_MAX];
  uint32_t clock_last_tap;
  int clock_tempo_history_pos;
  bool clock_tempo_tracking = false;
  bool tempo_setter = true;

  public:
  TapTempoTracker(bool should_set_tempo = true) {
    set_tempo_setter(should_set_tempo);
  }

  void set_tempo_setter(bool setter = true) {
    this->tempo_setter = setter;
  }
  bool is_tempo_setter() {
    return tempo_setter;
  }

  bool is_tracking() {
    return clock_tempo_tracking && get_num_samples()>1;
  }

  uint32_t get_history_gap(int_fast8_t i) {
    return clock_tempo_history[i];
  }
  uint32_t get_last_tap_micros() {
    return clock_last_tap;
  }
  int_fast8_t get_num_samples() {
    return min(clock_tempo_history_pos,CLOCK_TEMPO_HISTORY_MAX);
  }

  void clock_tempo_tap() {
    uint32_t now = micros();
    if (now - clock_last_tap == now || now - clock_last_tap >= CLOCK_TEMPO_RESTARTTHRESHOLD) {
      memset(clock_tempo_history, 0, sizeof(uint32_t)*CLOCK_TEMPO_HISTORY_MAX);
      clock_tempo_history_pos = 0;
      clock_tempo_tracking = true;
      clock_last_tap = now; 
      messages_log_add("started tempo tracking");
    } else if (clock_tempo_tracking) {
      clock_tempo_history[(clock_tempo_history_pos++) % CLOCK_TEMPO_HISTORY_MAX] = now - clock_last_tap;
      clock_last_tap = now;
      messages_log_add("added tempo value");
    }
  }

  float clock_tempo_estimate() {
    if (clock_tempo_tracking && clock_tempo_history_pos>=1) {  // at least 2 taps
      int total = 0;
      for (int i = 0 ; i < clock_tempo_history_pos && i < CLOCK_TEMPO_HISTORY_MAX ; i++) {
        total += clock_tempo_history[i];
      }

      float t = (float)total / (float)get_num_samples();

      // at this point, t should be equal to the average micros between beats
      float beats_per_second = 1000000.0 / t;
      float beats_per_minute = beats_per_second * 60.0;

      return beats_per_minute;
    }
    return 0.0f;
  }

  void clock_tempo_update() {
    uint32_t now = micros();
    if (now - clock_last_tap >= CLOCK_TEMPO_RESTARTTHRESHOLD) {
      clock_tempo_tracking = false;
    } else if (is_tracking() && is_tempo_setter()) {
        float estimate = clock_tempo_estimate();
        if (get_bpm() != estimate) {
            set_bpm(estimate);
        }
    }
  }

};


extern TapTempoTracker *tapper;