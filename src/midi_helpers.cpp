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

/*byte quantise_pitch(byte pitch, byte root_note = SCALE_ROOT_A, byte scale_number = 0) {
  byte octave = pitch / 12;
  byte chromatic_pitch = pitch % 12;

  static const byte valid_chromatic_pitches[][PITCHES_PER_SCALE] = {
    { 0, 2, 4, 5, 7, 9, 11 },
    { 0, 2, 3, 5, 6, 9, 10 },
    { 0, 1, 3, 4, 7, 9, 11 },
  };
  byte root_note_mod = root_note % 12;
  
  byte nearest_below_index = 0;
  for (int i = 0 ; i < PITCHES_PER_SCALE ; i++) {
    if( (root_note_mod + valid_chromatic_pitches[scale_number][i]) % 12 == chromatic_pitch)
      return pitch;
    if ((root_note_mod + valid_chromatic_pitches[scale_number][i]) % 12 < chromatic_pitch)
      nearest_below_index = i;
      //return (octave*12) + ((root_note + valid_chromatic_pitches[scale_number][i]) % 12);
      //nearest_below = root_note + valid_chromatic_pitches[scale_number][i-1];
    //if ((root_note + chromatic_pitch)%12 == valid_chromatic_pitches[scale_number][i])
      //nearest_below_index = i;
  }
  return (octave*12) + root_note_mod + valid_chromatic_pitches[scale_number][nearest_below_index];
  //return (octave*12) + root_note + nearest_below;
}*/

const byte valid_chromatic_pitches[][PITCHES_PER_SCALE] = {
  { 0, 2, 4, 5, 7, 9, 11 },     // major scale
  { 0, 2, 3, 5, 7, 8, 10 },     // natural minor scale
  { 0, 2, 3, 5, 7, 9, 11 },     // melodic minor scale 
  { 0, 2, 3, 5, 7, 8, 11 },     // harmonic minor scale
  { 0, 2, 4, 6, 7, 9, 11 },     // lydian
  { 0, 2, 4, 6, 8, 10, (12) },  // whole tone - 6 note scale - flavours for matching melody to chords
  { 0, 3, 5, 6, 7, 10, (12) },  // blues - flavours for matching melody to chords
  { 0, 2, 3, 6, 7, 8, 11 },     // hungarian minor scale
};

int8_t quantise_pitch(int8_t pitch, int8_t root_note, int8_t scale_number) {
  if (!is_valid_note(pitch))
    return -1;

  int octave = pitch/12;
  int chromatic_pitch = pitch % 12;
  int relative_pitch = chromatic_pitch - root_note;
  if (relative_pitch<0) {
    relative_pitch += 12;
    relative_pitch %= 12;
  }

  int last_interval = -1;
  for (int index = 0 ; index < PITCHES_PER_SCALE ; index++) {
    int interval = valid_chromatic_pitches[scale_number][index];
    if (interval == relative_pitch) 
      return pitch;
    if (relative_pitch < interval) {
      return last_interval + (octave*12) + root_note;
    }
    last_interval = interval;
  }
  return -1;
}
