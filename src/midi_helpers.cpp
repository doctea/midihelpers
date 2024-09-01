#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <string.h>
#endif  
#include "midi_helpers.h"

#include "Drums.h"

String get_note_name(int pitch) {
  if (!is_valid_note(pitch)) {
    String s = "_"; 
    return s;
  }
  int octave = pitch / 12;
  int chromatic_degree = pitch % 12; 
  const String note_names[] = {
    F("C"), F("C#"), F("D"), F("D#"), F("E"), F("F"), F("F#"), F("G"), F("G#"), F("A"), F("A#"), F("B")
  };
  
  String s = note_names[chromatic_degree] + String(octave);
  return s;
}

const char *note_names[] = {
  "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

#define NOTE_NAME_LENGTH 4  // 3 characters + \0 terminator. eg "C#5\0", "D3\0"

// optional second argument 'channel' defaults to 1 if not provided
const char *get_note_name_c(int pitch, int channel) {
  if (!is_valid_note(pitch)) {
    return "_";
  }
  
  if (channel==GM_CHANNEL_DRUMS && pitch>=GM_NOTE_MINIMUM && pitch<=GM_NOTE_MAXIMUM) {
    return gm_drum_names[pitch-GM_NOTE_MINIMUM];
  }

  int octave = pitch / 12;
  int chromatic_degree = pitch % 12;

  // to allow using multiple note_names in long printf's, store the last 5
  const int_fast8_t NUMBER_BUFFERS = 5; // how many different note names we allow to render at a time
  static char note_name[NUMBER_BUFFERS][NOTE_NAME_LENGTH];
  static int_fast8_t count = 0;
  count++;
  if (count>=NUMBER_BUFFERS) count = 0;
  snprintf(note_name[count], NOTE_NAME_LENGTH, "%s%i", note_names[chromatic_degree], octave);

  return note_name[count];
}

bool is_valid_note(int8_t note) {
    return note >= 0 && note <= MIDI_MAX_NOTE;
}
