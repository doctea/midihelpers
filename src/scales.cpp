
#include "scales.h"

/*const byte valid_chromatic_pitches[][PITCHES_PER_SCALE] = {
  { 0, 2, 4, 5, 7, 9, 11 },     // major scale
  { 0, 2, 3, 5, 7, 8, 10 },     // natural minor scale
  { 0, 2, 3, 5, 7, 9, 11 },     // melodic minor scale 
  { 0, 2, 3, 5, 7, 8, 11 },     // harmonic minor scale
  { 0, 2, 4, 6, 7, 9, 11 },     // lydian
  { 0, 2, 4, 6, 8, 10, (12) },  // whole tone - 6 note scale - flavours for matching melody to chords
  { 0, 3, 5, 6, 7, 10, (12) },  // blues - flavours for matching melody to chords
  { 0, 2, 3, 6, 7, 8, 11 },     // hungarian minor scale
};*/

//extern const scale_t[] scales;

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
    int interval = scales[scale_number].valid_chromatic_pitches[index];
    if (interval == relative_pitch) 
      return pitch;
    if (relative_pitch < interval) {
      return last_interval + (octave*12) + root_note;
    }
    last_interval = interval;
  }
  return -1;
}
