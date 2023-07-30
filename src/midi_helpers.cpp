#include <Arduino.h>
#include "midi_helpers.h"

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

// todo: version that will return GM drum name
const char *get_note_name_c(int pitch) {
  if (!is_valid_note(pitch)) {
    return "_";
  }
  int octave = pitch / 12;
  int chromatic_degree = pitch % 12;

  const int_fast8_t number_buffers = 4;
  static char note_name[number_buffers][4];
  static int_fast8_t count = 0;
  count++;
  if (count>=number_buffers) count = 0;
  snprintf(note_name[count], 4, "%s%i", note_names[chromatic_degree], octave);
  return note_name[count];
}

bool is_valid_note(int8_t note) {
    return note >= 0 && note <= MIDI_MAX_NOTE;
}
