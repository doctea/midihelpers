
#include "scales.h"

//#define DEBUG_CHORDS

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
int8_t quantise_pitch_chord_note(int8_t chord_root, CHORD::Type chord_number, int8_t note_of_chord, int8_t scale_root, SCALE scale_number, bool debug) {
  //bool debug = false; //true;

  if (note_of_chord>=4 || chords[chord_number].degree_number[note_of_chord]==-1)
    return -1;

  // get pointers to the selected scale and chord 
  const scale_t *sc = &scales[scale_number];
  const chord_t *ch = &chords[chord_number];

  #ifdef DEBUG_CHORDS
    if (debug) {
      Serial.printf("quantise_pitch_chord_note(%3s,", get_note_name_c(chord_root));
      Serial.printf("\t%s, %i, %s, %s):\t", ch->label, note_of_chord, get_note_name_c(scale_root), sc->label);
      Serial.printf("chord_root%12=%s,\t", get_note_name_c(chord_root%12));
    }
  #endif

  // need to find the scale degree of the pitch in the chosen scale...
  // eg if pitch is a C and scale_root is C Major, then degree number should be 0
  //    if pitch is a D and scale_root is C Major, then degree number should be 1
  //    if pitch is a D and scale_root is D Major, then degree number should be 2...
  int root_pitch_degree = -1;
  int root_pitch_offset = -1;
  for (int i = 0 ; i < PITCHES_PER_SCALE ; i++) {
    if ((scale_root+sc->valid_chromatic_pitches[i])%12==chord_root%12) {
      root_pitch_degree = i;
      root_pitch_offset = sc->valid_chromatic_pitches[i];
      break;
    }
  }

  // pitch isn't in the scale, return nothing
  if (root_pitch_degree==-1) {
    #ifdef DEBUG_CHORDS
      if (debug) Serial.printf("got root_pitch_degree of -1, so %s isn't in scale?  returning nothing\t\t!!!!\n", get_note_name_c(chord_root%12));
    #endif
    return -1;
  }

  // find the degree number and octave offset from the chord spelling
  int8_t chord_root_pitch = chord_root - root_pitch_offset;
  int8_t chord_target_degree = (root_pitch_degree + ch->degree_number[note_of_chord]) % PITCHES_PER_SCALE;
  int8_t chord_target_octave = (root_pitch_degree + ch->degree_number[note_of_chord]) / PITCHES_PER_SCALE;

  // no note at this position in the chord, return nothing
  if (chord_target_degree==-1) {
    #ifdef DEBUG_CHORDS
      if (debug) Serial.printf("got chord_target_degree of -1, returning nothing\t\t!!!!\n");
    #endif
    return -1;
  }

  #ifdef DEBUG_CHORDS
    if (debug) Serial.printf("got chord_root_pitch %3s (%i), \t", get_note_name_c(chord_root_pitch), chord_root_pitch);
    if (debug) Serial.printf("got chord_target_degree (%i), \t", chord_target_degree);
  #endif

  // then when we know the degree number and octave offset, we can use note_of_chord as on offset on it to discover the real pitch to use
  int8_t actual_pitch = (chord_target_octave*12) + sc->valid_chromatic_pitches[chord_target_degree];
  actual_pitch += chord_root_pitch;

  #ifdef DEBUG_CHORDS
    if (debug) Serial.printf("got note %3s (%i)", get_note_name_c(actual_pitch), actual_pitch);
    if (debug) Serial.println();
  #endif

  if (!is_valid_note(actual_pitch))
    return -1;

  return actual_pitch;
}