#pragma once

#include <Arduino.h>

// todo: configurable tempo setter function

// tap tempo stuff
#define DEFAULT_CLOCK_TEMPO_HISTORY_MAX 8
#define CLOCK_TEMPO_RESTARTTHRESHOLD (3.f*1000000.f)



class PhaseLockedLoop {
public:
    PhaseLockedLoop(double initialFrequency) : frequency(initialFrequency), beatsPassed(0) {}

    double update(uint32_t ticks) {
        // Calculate the phase based on ticks
        // 24 PPQN means 24 ticks per quarter note (beat)
        double phase = (ticks % 24) / 24.0;
        if (phase == 0) {
            beatsPassed++;
            //std::cout << "Beat Passed! Total Beats Passed: " << beatsPassed << ", Current BPM: " << currentBPM << std::endl; // Debugging output for beats passed
        }
        return phase;
    }

    double getFrequency() const {
        return frequency;
    }

    int getBeatsPassed() const {
        return beatsPassed;
    }

    double getCurrentBPM() const {
        return currentBPM;
    }

    void setFrequency(double newFrequency) {
        frequency = newFrequency; // Directly set frequency
        currentBPM = newFrequency * 60; // Update BPM
    }

private:
    double frequency;
    int beatsPassed;
    double currentBPM;
};


class TapTempoTracker {
  uint32_t *clock_tempo_history = nullptr;
  uint32_t clock_last_tap;
  uint32_t next_expected_tap;
  int max_sample_size = DEFAULT_CLOCK_TEMPO_HISTORY_MAX;
  int clock_tempo_history_pos;
  bool clock_tempo_tracking = false;
  bool tempo_setter = true;

  PhaseLockedLoop pll = PhaseLockedLoop(2.0);

  float internal_phase_history[2000];
  float tap_phase_history[2000];

  int internal_phase_cursor = 0, tap_phase_cursor = 0;

  public:
  TapTempoTracker(int max_sample_size = DEFAULT_CLOCK_TEMPO_HISTORY_MAX, bool should_set_tempo = true) {
    set_tempo_setter(should_set_tempo);
    clock_tempo_history = (uint32_t*)calloc(sizeof(uint32_t), max_sample_size);
    this->max_sample_size = max_sample_size;
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

  uint32_t get_max_sample_size() {
    return this->max_sample_size;
  }

  uint32_t get_history_gap(int_fast8_t i) {
    return clock_tempo_history[i];
  }
  uint32_t get_last_tap_micros() {
    return clock_last_tap;
  }
  int_fast8_t get_num_samples() {
    return min(clock_tempo_history_pos,max_sample_size);
  }

  void clock_tempo_tap() {
    uint32_t now = micros();
    if (clock_last_tap==0 || now - clock_last_tap >= CLOCK_TEMPO_RESTARTTHRESHOLD) {
      memset(clock_tempo_history, 0, sizeof(uint32_t)*max_sample_size);
      clock_tempo_history_pos = 0;
      clock_tempo_tracking = true;
      clock_last_tap = now; 
      messages_log_add("started tempo tracking");
    } else if (clock_tempo_tracking) {
      clock_tempo_history[clock_tempo_history_pos++] = now - clock_last_tap;
      clock_tempo_history_pos %= max_sample_size;
      clock_last_tap = now;
      messages_log_add("added tempo value");

      if (is_tempo_setter()) {
        pll.setFrequency( clock_tempo_estimate() / 60.0);
        set_bpm(clock_tempo_estimate());
      }

      tap_phase = 0.0;
      tap_ticks = 0;
      last_tap_ticked = now;
    }
  }

  float get_gap_average() {
    int total = 0;
    for (int i = 0 ; i < clock_tempo_history_pos && i < max_sample_size ; i++) {
      total += clock_tempo_history[i];
    }

    float t = (float)total / (float)get_num_samples();

    return t;
  }

  float clock_tempo_estimate() {
    if (clock_tempo_tracking && clock_tempo_history_pos>=1) {  // at least 2 taps
      float t = get_gap_average();

      //next_expected_tap = clock_last_tap + t;
      tap_tick_duration = t / (float)PPQN; // / (float)PPQN;

      // at this point, t should be equal to the average micros between beats
      float beats_per_second = 1000000.0 / t;
      float beats_per_minute = beats_per_second * 60.0;

      return beats_per_minute;
    }
    return 0.0f;
  }

  uint32_t tap_ticks = 0;
  float tap_phase = 0.0;
  float beat_phase = 0.0;
  int32_t tap_tick_duration = -1;
  uint32_t last_tap_ticked = 0;
  float last_temp_bpm;

  float phase_diff, phase_diff_pc;

  void clock_tempo_update() {
    uint32_t now = micros();

    beat_phase = (float)(ticks % PPQN) / (float)PPQN;
    add_internal_phase(beat_phase, now);

    if (is_tracking() && now - clock_last_tap >= CLOCK_TEMPO_RESTARTTHRESHOLD) {
      clock_tempo_tracking = false;
      tap_tick_duration = -1;
      tap_ticks = 0;
      clock_last_tap = 0;
      set_bpm(clock_tempo_estimate());
    } /*else if (is_tracking() && is_tempo_setter()) {
        float estimate = clock_tempo_estimate();
        if (get_bpm() != estimate) {
            set_bpm(estimate);
        }
    }*/
    //else if (is_tracking()) { 
      float estimate = clock_tempo_estimate();

      // recalculate the tap tempo phase
      if (tap_tick_duration > 0 && now >= last_tap_ticked + tap_tick_duration) {
        tap_ticks++;
        tap_ticks %= PPQN;
        tap_phase = ((float)tap_ticks / (float)PPQN); // / PPQN;
        last_tap_ticked = now;
        // recalculate real beat phase

        phase_diff = beat_phase - tap_phase;
        phase_diff_pc = (beat_phase / phase_diff);// * 100.0;
        if (is_tracking()) {
          if (phase_diff_pc >= 0.01f) {
            //last_temp_bpm = estimate + (estimate * phase_diff_pc);
            last_temp_bpm = estimate - (estimate * (phase_diff_pc*5.0));
            set_bpm(last_temp_bpm, true);
          } else if (phase_diff_pc <= -0.01f) {
            last_temp_bpm = estimate + (estimate * (phase_diff_pc*5.0));
            set_bpm(last_temp_bpm, true);
          } else {
            set_bpm(estimate);
          }
        }
      } /*else if (is_tracking()) {
        set_bpm(estimate);
      }*/
    //}

    this->add_tap_phase(tap_phase, now);

  }

  void tick(uint32_t ticks) {
    return;

    pll.update(ticks);

    clock_tempo_estimate();

    uint32_t now = micros();
    if (/*is_bpm_on_beat(ticks) &&*/ next_expected_tap>0 && now >= next_expected_tap) {
      uint32_t diff = now - next_expected_tap;
      uint32_t avg = (now + next_expected_tap) / 2;
      phase_diff_pc = diff/avg;
      if (abs(phase_diff_pc)>=0.05)
        set_bpm(get_bpm() * phase_diff_pc, true);
      else
        set_bpm(get_bpm(), false);
    }
  }

  float get_internal_phase_for_x_ago(int i) {
    //return internal_phase_history[abs(i - internal_phase_cursor) % 2000];
    //return internal_phase_history[i % 2000]; //abs(i - internal_phase_cursor) % 2000];
    i = 2000 - i;
    int c = internal_phase_cursor - i;
    if (c>=2000) 
      c-=2000;
    else if (c<0) {
      c+=2000;
    }
    return internal_phase_history[c % 2000];
  }
  void add_internal_phase(float v, uint32_t now) {
    static uint32_t last_now;
    now /= 1000;
    if (now > last_now) {
      internal_phase_history[internal_phase_cursor] = v;
      internal_phase_cursor++;
      if (internal_phase_cursor>=2000) 
        internal_phase_cursor = 0;
      last_now = now;
    }
  }

  float get_tap_phase_for_x_ago(int i) {
    //return internal_phase_history[abs(i - internal_phase_cursor) % 2000];
    //return internal_phase_history[i % 2000]; //abs(i - internal_phase_cursor) % 2000];
    i = 2000 - i;
    int c = tap_phase_cursor - i;
    if (c>=2000) 
      c-=2000;
    else if (c<0) {
      c+=2000;
    }
    return tap_phase_history[c % 2000];
  }
  void add_tap_phase(float v, uint32_t now) {
    static uint32_t last_now;
    now /= 1000;
    if (now > last_now) {
      tap_phase_history[tap_phase_cursor] = v;
      tap_phase_cursor++;
      if (tap_phase_cursor>=2000) 
        tap_phase_cursor = 0;
      last_now = now;
    }
  }

};


extern TapTempoTracker *tapper;