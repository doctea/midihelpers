#include "midi_helpers.h"

#include <functional-vlpp.h>

// class to track notes that are currently playing and to transpose them for quantisation
class NoteTracker {

  struct tracked_note_t {
    int8_t refcount = 0;
    int8_t transposed_note = -1;
  };

  int8_t held_note_count = 0;

  public:
    tracked_note_t held_notes[MIDI_NUM_NOTES] = {{0, -1}};
    bool held_notes_transposed[MIDI_NUM_NOTES] = {false};

    // track a note being held, with the actual incoming note ('note') and the note it was transposed to ('transposed_note')
    bool held_note_on(int8_t note, int8_t transposed_note) {
      if (is_valid_note(note)) {
        held_notes[note].refcount++;
        held_notes[note].transposed_note = transposed_note;
        held_notes_transposed[transposed_note] = true;
        held_note_count++;
        return held_notes[note].refcount > 0;
      }
      return false;
    }

    bool held_note_off(int8_t note) {
      if (is_valid_note(note)) {
        held_note_count--;
        held_notes[note].refcount--;// = 0;
        /*held_notes[note]--;
        if (held_notes[note] < 0)
          held_notes[note] = 0;*/
        held_notes_transposed[held_notes[note].transposed_note] = false;
        if (held_notes[note].refcount==0) 
          held_notes[note].transposed_note = -1;
        return held_notes[note].refcount == 0;
      }
      return false;
    }

    void clear_held() {
      if (held_note_count == 0) {
        return;
      }
      for (int i = 0; i < MIDI_NUM_NOTES; i++) {
        held_notes[i].transposed_note = -1;
        held_notes[i].refcount = 0;
      }
      held_note_count = 0;
    }

    inline int count_held() {
      return held_note_count;
    }
    int recalculate_count_held() {
      // todo: can probably pre-calculate this when notes are held or released, instead of looping through all notes every check
      int count = 0;
      for (int i = 0; i < MIDI_NUM_NOTES; i++) {
        if (is_note_held(i)) {
          count++;
        }
      }
      held_note_count = count;
      return count;
    }

    int8_t get_transposed_note_for(int8_t note) {
      if (count_held() == 0) {
        return false;
      }
      if (is_valid_note(note)) {
        return held_notes[note].transposed_note;
      }
      return -1;
    }

    inline bool is_note_held(int note) {
      if (count_held() == 0) {
        return false;
      }
      if (is_valid_note(note)) {
        return held_notes[note].refcount > 0;
      }
      return false;
    }
    
    inline bool is_note_held_transposed(int note) {
      if (count_held() == 0) {
        return false;
      }
      if (is_valid_note(note)) {
        //return held_notes[note].transposed_note != -1; // todo: hmm this appears to work but i'm not sure how it can do so, as it's not checking the correct transposed note..
        return held_notes_transposed[note];
      }
      return false;
    }

    inline bool is_note_held_any_octave(int note) {
      if (count_held() == 0) {
        return false;
      }
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
    // check if this transposed note is held (eg for when displaying what is actually playing)
    inline bool is_note_held_any_octave_transposed(int note) {
      if (count_held() == 0) {
        return false;
      }
      if (is_valid_note(note)) {
        note %= 12;
        for (int i = note; i < MIDI_NUM_NOTES+12; i+=12) {
          if (is_note_held_transposed(i)) {
            return true;
          }
        }
      }
      return false;
    }

    using foreach_func_def = vl::Func<void(int8_t note, int8_t transposed_note)>;

    // run a callback function for every note that is currently held
    void foreach_note(foreach_func_def func) {
      if (count_held() == 0) {
        return;
      }
      for (int i = 0; i < MIDI_NUM_NOTES; i++) {
        if (is_note_held(i)) {
          func(i, held_notes[i].transposed_note);
        }
      }
    }

    const char *get_held_notes_c() {
      static char buffer[256];
      buffer[0] = 0;
      if (count_held() == 0) {
        return buffer;
      }
      for (int i = 0; i < MIDI_NUM_NOTES; i++) {
        if (is_note_held(i)) {
          sprintf(buffer + strlen(buffer), "%-3s=>%-3s", get_note_name_c(i), get_note_name_c(held_notes[i].transposed_note));
        }
      }
      return buffer;
    }

    int get_held_note_index(int index) {
      if (count_held() == 0) {
        return NOTE_OFF;
      }
      int count = 0;
      index %= count_held();
      for (int i = 0; i < MIDI_NUM_NOTES; i++) {
        if (is_note_held(i)) {
          if (count == index) {
            return i;
          }
          count++;
        }
      }
      return NOTE_OFF;
    }

};