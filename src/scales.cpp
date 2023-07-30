
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

SCALE& operator++(SCALE& orig) {
  if (orig==static_cast<SCALE>(NUMBER_SCALES-1))
    orig = static_cast<SCALE>(0);
  else
    orig = static_cast<SCALE>(orig+1);
  return orig;
}
SCALE& operator++(SCALE& orig, int) {
    SCALE rVal = orig;
    ++orig;
    return rVal;
}
SCALE& operator--(SCALE& orig) {
  if (orig==static_cast<SCALE>(0))
    orig = static_cast<SCALE>(NUMBER_SCALES-1);
  else
    orig = static_cast<SCALE>(orig-1);
  return orig;
}
SCALE& operator--(SCALE& orig, int) {
    SCALE rVal = orig;
    --orig;
    return rVal;
}

int8_t quantise_pitch(int8_t pitch, int8_t scale_root, SCALE scale_number) {
  if (!is_valid_note(pitch))
    return -1;

  int octave = pitch/12;
  int chromatic_pitch = pitch % 12;
  int relative_pitch = chromatic_pitch - scale_root;
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
      int v = last_interval + (octave*12) + scale_root;
      if (v - pitch > 7) // if we've crossed octave boundary, step down an octave
        v-=12;
      return v;
    }
    last_interval = interval;
  }
  return last_interval;
}

// find the Nth note of a given chord in a given scale
int8_t quantise_pitch_chord_note(int8_t chord_root, CHORD_ID::Type chord_number, int8_t note_of_chord, int8_t scale_root, SCALE scale_number) {
  // need to find the scale degree of the pitch in the chosen scale...
  // eg if pitch is a C and scale_root is C Major, then degree number should be 0
  //    if pitch is a D and scale_root is C Major, then degree number should be 1
  //    if pitch is a D and scale_root is D Major, then degree number should be 2...
  // then when we know that degree number, we can use note_of_chord as on offset on it to discover the note to use
  bool debug = true;

  if (note_of_chord>=4 || chords[chord_number].degree_number[note_of_chord]==-1)
    return -1;

  if (debug) Serial.printf("quantise_pitch_chord_note(%3s\t, %s, %i, %i, %s):\t", get_note_name_c(chord_root), chords[chord_number].label, note_of_chord, scale_root, scales[scale_number].label);

  // get pointers to the selected scale and chord 
  scale_t *sc = &scales[scale_number];
  chord_t *ch = &chords[chord_number];

  // find what degree the chord root is in the selected scale
  int root_pitch_degree = -1;
  int root_pitch_offset = -1;
  for (int i = 0 ; i < PITCHES_PER_SCALE ; i++) {
    if (sc->valid_chromatic_pitches[i]==chord_root%12) {
      root_pitch_degree = i;
      root_pitch_offset = sc->valid_chromatic_pitches[i];
      break;
    }
  }

  // if didn't find pitch in the scale, return nothing
  if (root_pitch_degree==-1)
    return -1;

  int8_t chord_root_pitch = chord_root - root_pitch_offset;
  int8_t chord_target_degree = (root_pitch_degree + ch->degree_number[note_of_chord]) % PITCHES_PER_SCALE;
  int8_t chord_target_octave = (root_pitch_degree + ch->degree_number[note_of_chord]) / PITCHES_PER_SCALE;

  if (chord_target_degree==-1)
    return -1;

  if (debug) Serial.printf("got chord_root_pitch %3s (%i), \t", get_note_name_c(chord_root_pitch), chord_root_pitch);
  if (debug) Serial.printf("got chord_target_degree (%i), \t", chord_target_degree);

  int8_t actual_pitch = (chord_target_octave*12) + sc->valid_chromatic_pitches[chord_target_degree];
  actual_pitch += chord_root_pitch;

  if (debug) Serial.printf("got note %3s (%i)", get_note_name_c(actual_pitch), actual_pitch);
  if (debug) Serial.println();

  return actual_pitch;
}