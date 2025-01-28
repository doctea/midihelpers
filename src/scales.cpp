#ifdef ENABLE_SCALES

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <cinttypes>
#endif

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

int8_t *global_scale_root = nullptr;
SCALE  *global_scale_type = nullptr;
int8_t *global_chord_degree = nullptr;

void set_global_scale_root_target(int8_t *root_note) {
    global_scale_root = root_note;
}
void set_global_scale_type_target(SCALE *scale_type) {
    global_scale_type = scale_type;
}
void set_global_chord_degree_target(int8_t *chord_degree) {
    global_chord_degree = chord_degree;
}

int8_t get_global_scale_root() {
  if (global_scale_root!=nullptr)
    return *global_scale_root;
  return SCALE_GLOBAL_ROOT;
}
SCALE get_global_scale_type() {
  if (global_scale_type!=nullptr)
    return *global_scale_type;
  return SCALE::GLOBAL;
}

int8_t get_global_chord_degree() {
  if (global_chord_degree!=nullptr)
    return *global_chord_degree;
  return -1;
}

// check the passed-in root note to see if we should use global setting - return global setting if so, otherwise return passed-in root note
int8_t get_effective_scale_root(int8_t scale_root) {
  if (scale_root==SCALE_GLOBAL_ROOT)
    scale_root = get_global_scale_root();
  return scale_root;
}

// check the passed-in scale type to see if we should use global setting - return global setting if so, otherwise return passed-in scale type
SCALE get_effective_scale_type(SCALE scale_number) {
  if (scale_number==SCALE::GLOBAL)
    scale_number = get_global_scale_type();
  if (scale_number==SCALE::GLOBAL)
    scale_number = SCALE::MAJOR;
  return scale_number;
}

int8_t get_effective_chord_degree(int8_t chord_degree) {
  //Serial.printf("get_effective_chord_degree(%i): ", chord_degree);
  if (chord_degree==-1) {// CHORD_GLOBAL_DEGREE 
    chord_degree = get_global_chord_degree();
    //Serial.printf("get_global_chord_degree()=%i\n", chord_degree);
  }
  if (chord_degree==0) {  // no chord
    //Serial.println("no chord");
    return 0;
  }

  chord_degree %= (PITCHES_PER_SCALE+1);
  //Serial.printf("=> calculated to be %i\n", chord_degree);

  return chord_degree;
}

int8_t quantise_get_root_pitch_for_degree(int8_t degree, int8_t scale_root, SCALE scale_number) {
  if (degree<1 || degree>PITCHES_PER_SCALE)
    return -1;

  scale_root = get_effective_scale_root(scale_root);
  scale_number = get_effective_scale_type(scale_number);
  if(scale_number==SCALE_GLOBAL_ROOT || scale_number==SCALE::GLOBAL)
    return -1;

  return scale_root + scales[scale_number].valid_chromatic_pitches[degree-1];
}

int8_t quantise_pitch(int8_t pitch, int8_t scale_root, SCALE scale_number) {
  if (!is_valid_note(pitch))
    return NOTE_OFF;

  bool debug = true;

  // check whether the scale_root and scale_number should be replaced with global versions...
  // and if no value is set for global, return unquantised note and/or default to major scale
  scale_root = get_effective_scale_root(scale_root);
  scale_number = get_effective_scale_type(scale_number);
  
  if (debug) Serial.printf("quantise_pitch(%s,\t%s,\t%s)\n", get_note_name_c(pitch), get_note_name_c(scale_root), scales[scale_number].label);
  if(scale_number==SCALE_GLOBAL_ROOT || scale_number==SCALE::GLOBAL)  // todo: check that pitch is in the chord...?
    return pitch;

  int octave = pitch/12;
  int chromatic_pitch = pitch % 12;
  int relative_pitch = chromatic_pitch - scale_root;
  if (relative_pitch<0) {
    relative_pitch += 12;
    relative_pitch %= 12;
  }

  int8_t return_value = -1;

  // find the closest note in the scale to the pitch
  int last_interval = -1;
  for (int index = 0 ; index < PITCHES_PER_SCALE ; index++) {
    int8_t interval = scales[scale_number].valid_chromatic_pitches[index];
    if (!is_valid_note(interval)) 
      continue;
    if (interval == relative_pitch) {
      return_value = pitch;
      //Serial.printf("branch#1: pitch %s (%i) is in scale %s (%i) at index %i\n", get_note_name_c(pitch), pitch, scales[scale_number].label, scale_number, index);
      break;
    }
    if (relative_pitch < interval) {
      last_interval = interval;
      int8_t v = last_interval + (octave*12) + scale_root;
      if (v - pitch > 7) // if we've crossed octave boundary, step down an octave
        v -= 12;
      //Serial.printf("branch#2: Quantised \t%s (%i)\tto\t%s (%i), scale_number=%i and scale_root=%i (%s)\n", get_note_name_c(pitch), pitch, get_note_name_c(v), v, scale_number, scale_root, get_note_name_c(scale_root));
      return_value = v;
      break;
    }
    last_interval = interval;
  }
  if (debug) Serial.printf("branch#3: Quantised \t%s (%i)\tto\t%s (%i), scale_number=%i and scale_root=%i (%s)\n", get_note_name_c(pitch), pitch, get_note_name_c(last_interval), last_interval, scale_number, scale_root, get_note_name_c(scale_root));

  return return_value;
}

int8_t quantise_chord(int8_t pitch, int8_t scale_root, SCALE scale_number, chord_identity_t chord_identity) {
  if (!is_valid_note(pitch))
    return NOTE_OFF;

  bool debug = true;

  // check whether the scale_root and scale_number should be replaced with global versions...
  // and if no value is set for global, return unquantised note and/or default to major scale
  scale_root = get_effective_scale_root(scale_root);
  scale_number = get_effective_scale_type(scale_number);
  chord_identity.chord_degree = get_effective_chord_degree(chord_identity.chord_degree);

  int octave = pitch/12;
  int chromatic_pitch = pitch % 12;
  int relative_pitch = chromatic_pitch - scale_root;
  if (relative_pitch<0) {
    relative_pitch += 12;
    relative_pitch %= 12;
  }

  // just return the last valid pitch found if we don't need to filter by chord
  if (chord_identity.chord_degree<=0) {
    return pitch;
  } else { //if (chord_identity.chord_degree>0) {
    if (debug) Serial.printf("quantise_pitch(%s, %s, %s, %i) - chord_degree>0\n", get_note_name_c(pitch), get_note_name_c(scale_root), scales[scale_number].label, chord_identity.chord_degree);
    // find nearest pitch for the current scale, scale_root, and chord_degree combination
    //int last_interval = -1;
    int8_t n = -1;
    bool DEBUG_QUANTISE = true; // todo: remove this
    int8_t chord_root_pitch = quantise_get_root_pitch_for_degree(chord_identity.chord_degree, scale_root, scale_number);
    if (debug) Serial.printf("\tchord_root_pitch=%s (%i)\n", get_note_name_c(chord_root_pitch), chord_root_pitch);

    if (debug) {
      Serial.printf("\tchord notes (chord_type=%i): ", chord_identity.chord_type);
      for (size_t i = 0 ; i < PITCHES_PER_CHORD && ((n = quantise_pitch_chord_note(chord_root_pitch, chord_identity.chord_type, i, scale_root, scale_number, chord_identity.inversion, DEBUG_QUANTISE)) >= 0) ; i++) {
        Serial.printf("%i = %s, \t", n, get_note_name_c(n));
      }
      Serial.println();
    }

    for (size_t i = 0 ; i < PITCHES_PER_CHORD && ((n = quantise_pitch_chord_note(chord_root_pitch, chord_identity.chord_type, i, scale_root, scale_number, chord_identity.inversion, DEBUG_QUANTISE)) >= 0) ; i++) {
      n %= 12;
      if (debug) Serial.printf("\t\t%i: checking if unquantised pitch %s\t(return_value=%i) aka %s\t(pitch=%i), n=%i) is in chord?\n", i, get_note_name_c(pitch), pitch, get_note_name_c(pitch), pitch, n);
      if ((octave*12)+/*chord_root_pitch+*/n==pitch) {
        if (debug) Serial.printf("\t\t%i: yes, found quantised pitch %s\t(pitch=%i) aka %s\t(pitch=%i), n=%i) in chord!\n", i, get_note_name_c(pitch), pitch, get_note_name_c(pitch), pitch, n);
        return pitch;
      }
    }
    //Serial.println("no chord note found, returning no note!");
    return NOTE_OFF;
  }
}

// find the Nth note of a given chord in a given scale
int8_t quantise_pitch_chord_note(int8_t chord_root, CHORD::Type chord_number, int8_t note_of_chord, int8_t scale_root, SCALE scale_number, int inversion, bool debug) {
  //bool debug = false; //true;

  if (note_of_chord>=PITCHES_PER_CHORD || chords[chord_number].degree_number[note_of_chord]==-1)
    return -1;

  scale_root = get_effective_scale_root(scale_root);
  scale_number = get_effective_scale_type(scale_number);
  if(scale_number==SCALE_GLOBAL_ROOT || scale_number==SCALE::GLOBAL)
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

  // C E G                                                                  C E G
  // To invert a chord, move the bottom note up an octave.  yields E G C    E G C   first+1
  // To get a second inversion triad, move the third up an octave. G C E    G C E   first+1, second+1
  // A triad with the 5th of the chord in the bass is called a triad in second inversion.

  // For example, the notes of the C dominant 7th chord are C, E, G and Bb. 
  // In root position, the notes of Cdom7 are C-E-G-Bb. 
  // In its 1st inversion, it’s               E-G-Bb-C.   first+1                       so when inversion = 1, 0 should be raised
  // In its 2nd inversion, it’s               G-Bb-C-E.   first+1, second+1             so when inversion = 2, 0 and 1 should be raised
  // In its 3rd inversion, it’s               Bb-C-E-G.   first+1, second+1, third+1    so when inversion = 3, 0 and 1 and 2 should be raised

  if (/*inversion>0 &&*/ inversion > note_of_chord) {
    if (debug) Serial.printf("inversion is %i and note_of_chord is %i, so increasing chord_target_octave + 1 to %i\n", inversion, note_of_chord, chord_target_octave+1);
    chord_target_octave += 1;
  }
  chord_target_octave = constrain(chord_target_octave, 0, MAXIMUM_OCTAVE);

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

#endif