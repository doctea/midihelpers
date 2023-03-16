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

// todo: version that will return GM drum name
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

/*#define SCALE_ROOT_A       21
#define SCALE_ROOT_A_SHARP 22
#define SCALE_ROOT_B       23
#define SCALE_ROOT_C       24
#define SCALE_ROOT_C_SHARP 25
#define SCALE_ROOT_D       26
#define SCALE_ROOT_D_SHARP 27
#define SCALE_ROOT_E       28
#define SCALE_ROOT_F       29
#define SCALE_ROOT_F_SHARP 30
#define SCALE_ROOT_G       31
#define SCALE_ROOT_G_SHARP 32*/

#define PITCHES_PER_SCALE 7

byte quantise_pitch(byte pitch, byte root_note = SCALE_ROOT_A, byte scale_number = 0) {
  byte octave = pitch / 12;
  byte chromatic_pitch  = pitch % 12;

  static const byte valid_chromatic_pitches[][PITCHES_PER_SCALE] = {
    { 0, 2, 4, 5, 7, 9, 11 },
    { 0, 2, 3, 5, 6, 9, 10 },
    { 0, 1, 3, 4, 7, 9, 11 },
  };
  byte root_note_mod = root_note % 12;
  
  byte nearest_lowest = 0;
  for (int i = 0 ; i < PITCHES_PER_SCALE ; i++) {
    if( (root_note + valid_chromatic_pitches[scale_number][i]) % 12==chromatic_pitch)
      return pitch;
    if (chromatic_pitch < root_note + valid_chromatic_pitches[scale_number][i])
      nearest_lowest = root_note + valid_chromatic_pitches[scale_number][i-1];
  }
  return nearest_lowest;
}