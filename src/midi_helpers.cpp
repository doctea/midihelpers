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

const char *get_note_name_c(int pitch) {
  if (!is_valid_note(pitch)) {
    return "_";
  }
  int octave = pitch / 12;
  int chromatic_degree = pitch % 12;
  const char *note_names[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
  };
  static char note_name[4];
  snprintf(note_name, 4, "%s%i", note_names[chromatic_degree], octave);
  return note_name;
}

bool is_valid_note(int8_t note) {
    return note >= 0 && note <= MIDI_MAX_NOTE;
}

// todo: version that will return GM drum name


byte quantise_pitch(byte pitch) {
  byte octave = pitch / 12;
  byte chromatic_pitch  = pitch % 12;

  byte valid_chromatic_pitches[] = {
    0, 2, 4, 5, 7, 9, 11
  };
  
  byte nearest_lowest = 0;
  for (int i = 0 ; i < sizeof(valid_chromatic_pitches) ; i++) {
    if(valid_chromatic_pitches[i]==chromatic_pitch)
      return pitch;
    if (chromatic_pitch < valid_chromatic_pitches[i])
      nearest_lowest = valid_chromatic_pitches[i-1];
  }
  return nearest_lowest;
}