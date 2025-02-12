#include "midi_helpers.h"

#include <functional-vlpp.h>

// class to track notes that are currently playing
class NoteTracker {
  public:
    int8_t held_notes[MIDI_NUM_NOTES] = {0};

    bool held_note_on(int note) {
      if (is_valid_note(note)) {
        held_notes[note]++;
        return held_notes[note] > 0;
      }
      return false;
    }

    bool held_note_off(int note) {
      if (is_valid_note(note)) {
        held_notes[note] = 0;
        /*held_notes[note]--;
        if (held_notes[note] < 0)
          held_notes[note] = 0;*/
        return held_notes[note] == 0;
      }
      return false;
    }

    bool is_note_held(int note) {
      if (is_valid_note(note)) {
        return held_notes[note] > 0;
      }
      return false;
    }

    void clear_held() {
      for (int i = 0; i < MIDI_NUM_NOTES; i++) {
        held_notes[i] = 0;
      }
    }

    using foreach_func_def = vl::Func<void(int8_t note)>;

    void foreach_note(foreach_func_def func) {
      for (int i = 0; i < MIDI_NUM_NOTES; i++) {
        if (is_note_held(i)) {
          func(i);
        }
      }
    }

    bool is_note_held_any_octave(int note) {
        if (is_valid_note(note)) {
            note %= 12;
            for (int i = note; i < MIDI_NUM_NOTES+12; i+=12) {
                if (is_note_held(i)) {
                    return true;
                }
            }
        }
        return false;
    }

};