#ifdef ENABLE_SCALES

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <cinttypes>
#endif

#include "scales.h"

// Change array declaration to use pointers
const scale_pattern_t *scale_patterns[] = {
  make_scale_pattern_t_from_string("w w h w w w h", "0. Natural"),
  make_scale_pattern_t_from_string("w h w w w w h", "1. Melodic"),
  make_scale_pattern_t_from_string("w w h w h + h", "2. Harmonic Major"),
  make_scale_pattern_t_from_string("w h w w h + h", "3. Harmonic Minor"),
  make_scale_pattern_t_from_string("h + h w h + h", "4. Byzantine"),            // " this scale is commonly represented with the first and last half step each being represented with quarter tones:"
};

const scale_t* scales[] = {
  make_scale_t_from_pattern(scale_patterns[0], "Ionian", 0),
  make_scale_t_from_pattern(scale_patterns[0], "Dorian", 1),
  make_scale_t_from_pattern(scale_patterns[0], "Phrygian", 2),
  make_scale_t_from_pattern(scale_patterns[0], "Lydian", 3),
  make_scale_t_from_pattern(scale_patterns[0], "Mixolydian", 4),
  make_scale_t_from_pattern(scale_patterns[0], "Aeolian", 5),
  make_scale_t_from_pattern(scale_patterns[0], "Locrian", 6),

  make_scale_t_from_pattern(scale_patterns[1], "Melodic Minor", 0),
  make_scale_t_from_pattern(scale_patterns[1], "Dorian.b2", 1),
  make_scale_t_from_pattern(scale_patterns[1], "Lydian.aug", 2),
  make_scale_t_from_pattern(scale_patterns[1], "Lydian.dom", 3),
  make_scale_t_from_pattern(scale_patterns[1], "Mixolydian.b6", 4),
  make_scale_t_from_pattern(scale_patterns[1], "Locrian.#2", 5),
  make_scale_t_from_pattern(scale_patterns[1], "Superlocrian", 6),

  make_scale_t_from_pattern(scale_patterns[2], "Harm. Major", 0),
  make_scale_t_from_pattern(scale_patterns[2], "Dorian.b5", 1),
  make_scale_t_from_pattern(scale_patterns[2], "Phrygian.b4", 2),
  make_scale_t_from_pattern(scale_patterns[2], "Lydian.b3", 3),
  make_scale_t_from_pattern(scale_patterns[2], "Mixolydian.b2", 4),
  make_scale_t_from_pattern(scale_patterns[2], "Lydian.#2", 5),
  make_scale_t_from_pattern(scale_patterns[2], "Locrian.bb7", 6),

  make_scale_t_from_pattern(scale_patterns[3], "Harm. Minor", 0),
  make_scale_t_from_pattern(scale_patterns[3], "Locrian.#6", 1),
  make_scale_t_from_pattern(scale_patterns[3], "Ionian.#5", 2),
  make_scale_t_from_pattern(scale_patterns[3], "Dorian.#4", 3),
  make_scale_t_from_pattern(scale_patterns[3], "Phrygian dom", 4),
  make_scale_t_from_pattern(scale_patterns[3], "Lydian.Aug.#2", 5),
  make_scale_t_from_pattern(scale_patterns[3], "Superlocrian.bb7", 6),

  make_scale_t_from_pattern(scale_patterns[4], "Double harm.maj", 0),
  make_scale_t_from_pattern(scale_patterns[4], "Lydian.#2.#6", 1),
  make_scale_t_from_pattern(scale_patterns[4], "Ultraphrygian", 2),
  make_scale_t_from_pattern(scale_patterns[4], "Hungarian minor", 3),
  make_scale_t_from_pattern(scale_patterns[4], "Oriental", 4),
  make_scale_t_from_pattern(scale_patterns[4], "Ionian.#2.#5", 5),
  make_scale_t_from_pattern(scale_patterns[4], "Locrian.b3.bb7", 6),
};

//#define NUMBER_SCALES ((sizeof(scales)/sizeof(scale_t)))    // uses size of real scales[] array, ie existant scales, rather than the SCALE enum (which has an extra value to represent GLOBAL)
const size_t NUMBER_SCALES = sizeof(scales)/sizeof(scale_t*);

//#define DEBUG_CHORDS

/*SCALE& operator++(SCALE& orig) {
  if (orig==static_cast<SCALE>(NUMBER_SCALES-1))
    orig = static_cast<SCALE>(0);
  else
    orig = static_cast<SCALE>(orig+1);
  return orig;
}
SCALE operator++(SCALE& orig, int) {
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
SCALE operator--(SCALE& orig, int) {
    SCALE rVal = orig;
    --orig;
    return rVal;
}*/

scale_identity_t *global_scale_identity = nullptr;
chord_identity_t *global_chord_identity = nullptr;

void set_global_scale_identity_target(scale_identity_t *scale) {
  global_scale_identity = scale;
}

int8_t get_global_scale_root() {
  if (global_scale_identity!=nullptr)
    return global_scale_identity->root_note;
  return SCALE_GLOBAL_ROOT;
}
scale_index_t get_global_scale_type() {
  if (global_scale_identity!=nullptr)
    return global_scale_identity->scale_number;
  return SCALE_GLOBAL;
}

void set_global_chord_identity_target(chord_identity_t *chord_identity) {
  global_chord_identity = chord_identity;
}

chord_identity_t get_global_chord_identity() {
  if (global_chord_identity!=nullptr)
    return *global_chord_identity;
  return {CHORD::TRIAD, -1, 0};
}

int8_t get_global_chord_degree() {
  if (global_chord_identity!=nullptr)
    return global_chord_identity->degree;
  return -1;
}
int8_t get_effective_chord_degree(int8_t chord_degree) {
  if (chord_degree==-1) // CHORD_GLOBAL_DEGREE 
    return get_global_chord_degree();
  if (chord_degree==0) // no chord
    return 0;
  chord_degree %= (PITCHES_PER_SCALE+1);
  return chord_degree;
}
CHORD::Type get_effective_chord_type(CHORD::Type chord_type) {
  if (chord_type==CHORD::GLOBAL)
    chord_type = get_global_chord_identity().type;
  if (chord_type==CHORD::GLOBAL)
    chord_type = CHORD::TRIAD;
  return chord_type;
}
int8_t get_effective_chord_inversion(int8_t inversion) {
  if (inversion==-1) // CHORD_GLOBAL_INVERSION 
    return get_global_chord_identity().inversion;
  return inversion;
}
  

// check the passed-in root note to see if we should use global setting - return global setting if so, otherwise return passed-in root note
int8_t get_effective_scale_root(int8_t scale_root) {
  if (scale_root==SCALE_GLOBAL_ROOT)
    scale_root = get_global_scale_root();
  return scale_root;
}

// check the passed-in scale type to see if we should use global setting - return global setting if so, otherwise return passed-in scale type
scale_index_t get_effective_scale_type(scale_index_t scale_number) {
  if (scale_number==SCALE_GLOBAL)
    scale_number = get_global_scale_type();
  if (scale_number==SCALE_GLOBAL)
    scale_number = SCALE_FIRST;
  return scale_number;
}

/*int8_t get_effective_chord_degree(int8_t chord_degree) {
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
}*/

int8_t quantise_get_root_pitch_for_degree(int8_t degree, int8_t scale_root, scale_index_t scale_number) {
  if (degree<1 || degree>PITCHES_PER_SCALE)
    return -1;

  scale_root = get_effective_scale_root(scale_root);
  scale_number = get_effective_scale_type(scale_number);
  if(scale_number==SCALE_GLOBAL_ROOT || scale_number==SCALE_GLOBAL) {
    //Serial.printf("quantise_get_root_pitch_for_degree(%i, %i, %s) - scale_number is global but get_effective_scale_type also returned global so only returning -1?\n", degree, scale_root, scales[scale_number].label);
    return -1;
  }

  int8_t result = scale_root + scales[scale_number]->valid_chromatic_pitches[degree-1];

  //Serial.printf("quantise_get_root_pitch_for_degree(%i, %i, %s) - scale_number=%i and scale_root=%i (%s) => %i (%s)\n", degree, scale_root, scales[scale_number].label, scale_number, scale_root, get_note_name_c(scale_root), result, get_note_name_c(result));

  return result;
}

PROGMEM
int8_t quantise_pitch_to_scale(int8_t pitch, int8_t scale_root, scale_index_t scale_number, bool debug) {
  if (!is_valid_note(pitch))
    return NOTE_OFF;
    
  //debug = true;

  // check whether the scale_root and scale_number should be replaced with global versions...
  // and if no value is set for global, return unquantised note and/or default to major scale
  scale_root = get_effective_scale_root(scale_root);
  scale_number = get_effective_scale_type(scale_number);
  
  if (debug) Serial.printf("quantise_pitch_to_scale(%s,\t%s,\t%s)\n", get_note_name_c(pitch), get_note_name_c(scale_root), scales[scale_number]->label);
  if(scale_number==SCALE_GLOBAL_ROOT || scale_number==SCALE_GLOBAL)  // todo: check that pitch is in the chord...?
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
    int8_t interval = scales[scale_number]->valid_chromatic_pitches[index];
    if (!is_valid_note(interval)) 
      continue;
    if (interval == relative_pitch) {
      return_value = pitch;
      if (debug) Serial.printf("\tbranch#1: pitch %s (%i) is in scale %s (%i) at index %i\n", get_note_name_c(pitch), pitch, scales[scale_number]->label, scale_number, index);
      break;
    }
    if (relative_pitch < interval) {
      last_interval = interval;
      int8_t v = last_interval + (octave*12) + scale_root;
      if (v - pitch > 7) // if we've crossed octave boundary, step down an octave
        v -= 12;
      if (debug) Serial.printf("\tbranch#2: Quantised \t%s (%i)\tto\t%s (%i), scale_number=%i and scale_root=%i (%s)\n", get_note_name_c(pitch), pitch, get_note_name_c(v), v, scale_number, scale_root, get_note_name_c(scale_root));
      return_value = v;
      break;
    }
    last_interval = interval;
  }
  if (debug) Serial.printf("\tbranch#3: Quantised \t%s (%i)\tto\t%s (%i), scale_number=%i and scale_root=%i (%s)\n", get_note_name_c(pitch), pitch, get_note_name_c(last_interval), last_interval, scale_number, scale_root, get_note_name_c(scale_root));

  return return_value;
}

PROGMEM
int8_t quantise_pitch_to_chord(int8_t pitch, int8_t quantised_to_nearest_tolerance, int8_t scale_root, scale_index_t scale_number, chord_identity_t chord_identity, bool debug) {
  if (!is_valid_note(pitch))
    return NOTE_OFF;

  //debug = true;

  // check whether the scale_root and scale_number should be replaced with global versions...
  // and if no value is set for global, return unquantised note and/or default to major scale
  scale_root = get_effective_scale_root(scale_root);
  scale_number = get_effective_scale_type(scale_number);
  chord_identity.degree = get_effective_chord_degree(chord_identity.degree);
  chord_identity.type = get_effective_chord_type(chord_identity.type);
  chord_identity.inversion = get_effective_chord_inversion(chord_identity.inversion);

  int octave = pitch/12;
  int chromatic_pitch = pitch % 12;
  int relative_pitch = chromatic_pitch - scale_root;
  if (relative_pitch<0) {
    relative_pitch += 12;
    relative_pitch %= 12;
  }

  // just return the last valid pitch found if we don't need to filter by chord
  if (chord_identity.degree<=0) {
    return pitch;
  } else { //if (chord_identity.chord_degree>0) {
    if (debug) Serial.printf("quantise_chord(%s, %s, %s, %i) - chord_degree>0\n", get_note_name_c(pitch), get_note_name_c(scale_root), scales[scale_number]->label, chord_identity.degree);
    // find nearest pitch for the current scale, scale_root, and chord_degree combination
    //int last_interval = -1;
    int8_t n = -1;
    bool DEBUG_QUANTISE = debug; // todo: remove this
    int8_t chord_root_pitch = quantise_get_root_pitch_for_degree(chord_identity.degree, scale_root, scale_number);

    if (debug) {
      Serial.printf("\tchord_root_pitch=%3s (%i)\t", get_note_name_c(chord_root_pitch), chord_root_pitch);
      //Serial.printf("\tchord type=%s\t", chords[chord_identity.chord_type].label);
      Serial.printf("\tchord notes (chord_type=%s %i): ", chords[chord_identity.type].label, chord_identity.type);
      for (size_t i = 0 ; i < PITCHES_PER_CHORD && ((n = get_quantise_pitch_chord_note(chord_root_pitch, chord_identity.type, i, scale_root, scale_number, chord_identity.inversion, DEBUG_QUANTISE)) >= 0) ; i++) {
        Serial.printf("%i = %s, \t", n, get_note_name_c(n));
      }
      Serial.println();
    }

    int8_t nearest_found = -1;
    int8_t nearest_distance = quantised_to_nearest_tolerance;

    for (size_t i = 0 ; i < PITCHES_PER_CHORD && ((n = get_quantise_pitch_chord_note(chord_root_pitch, chord_identity.type, i, scale_root, scale_number, chord_identity.inversion, DEBUG_QUANTISE)) >= 0) ; i++) {
      n %= 12;
      if (debug)   Serial.printf("\t\t%i: matches chord note %i (%s)?\n", i, n, get_note_name_c(n));
      if ((octave*12)+n==pitch) {
        if (debug) Serial.printf("\t\t%i: yes, found quantised pitch 3%s\t(pitch=%i)  in chord!\n",      i, get_note_name_c(pitch), pitch, n);
        return pitch;
      } else if (quantised_to_nearest_tolerance>0) {
        int diff = abs(((octave*12)+n) - pitch);
        if (debug) Serial.printf("\t\t\ttesting pitch %i vs %i: diff=%i against quantised_to_nearest_tolerance=%i and nearest_distance=%i\n", pitch, (octave*12)+n, diff, quantised_to_nearest_tolerance, nearest_distance);
        if (diff <= quantised_to_nearest_tolerance && diff <= nearest_distance) {
          nearest_distance = diff;
          nearest_found = (octave*12)+n;
          if (debug) Serial.printf("\t\tgot new nearest - %i vs %i: diff=%i, nearest_distance=%i\n", pitch, (octave*12)+n, diff, nearest_distance);
        }
      }
    }
    if (nearest_distance<=quantised_to_nearest_tolerance) {
      if (debug) Serial.printf("\tnearest distance is %i, returning nearest note %i (%s)\n", nearest_distance, nearest_found, get_note_name_c(nearest_found));
      return nearest_found;
    }
    if (debug) Serial.println("\tno chord note found, returning no note!");
    return NOTE_OFF;
  }
}

// find the Nth note of a given chord in a given scale
int8_t get_quantise_pitch_chord_note(int8_t chord_root, CHORD::Type chord_number, int8_t note_of_chord, int8_t scale_root, scale_index_t scale_number, int inversion, bool debug) {
  //bool debug = false; //true;

  if (note_of_chord>=PITCHES_PER_CHORD || chords[chord_number].degree_number[note_of_chord]==-1)
    return -1;

  scale_root = get_effective_scale_root(scale_root);
  scale_number = get_effective_scale_type(scale_number);
  if(scale_number==SCALE_GLOBAL_ROOT || scale_number==SCALE_GLOBAL)
    return -1;

  // get pointers to the selected scale and chord 
  const scale_t *sc = scales[scale_number];
  const chord_t *ch = &chords[chord_number];

  #ifdef DEBUG_CHORDS
    if (debug) {
      Serial.printf("get_quantise_pitch_chord_note(%3s,", get_note_name_c(chord_root));
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
    if (debug) Serial.printf("got chord_target_octave (%i), \t", chord_target_octave);
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


void print_scale(int8_t root_note, scale_index_t scale_number) {
  root_note = get_effective_scale_root(root_note);
  scale_number = get_effective_scale_type(scale_number);
  //Serial.printf("Scale %s (%i) starting at %s (%i):\t", scales[scale_number]->label, scale_number, get_note_name_c(root_note), root_note);
  Serial.printf("Pattern %s (%s) in mode #%i (%s), starting on note %s (%i):\t", 
    scales[scale_number]->pattern->label, 
    scales[scale_number]->pattern->interval_pattern,
    scales[scale_number]->rotation+1, 
    scales[scale_number]->label, 
    get_note_name_c(root_note), 
    root_note
  );
  for (int i = 0 ; i < PITCHES_PER_SCALE ; i++) {
    int note = root_note + scales[scale_number]->valid_chromatic_pitches[i];
    Serial.printf("%-3s ", get_note_name_c(note));
  }
  Serial.println();
}

void print_scale(int8_t root_note, scale_t scale) {
  //Serial.printf("Scale %s based on pattern %s:\t", scale.label, scale.pattern->label);
  Serial.printf("Pattern %s (%s) in mode #%i (%s), starting on note %s (%i):\t", 
    scale.pattern->label, 
    scale.pattern->interval_pattern,
    scale.rotation+1, 
    scale.label, 
    get_note_name_c(root_note), 
    root_note
  );
  for (int i = 0 ; i < PITCHES_PER_SCALE ; i++) {
    int note = root_note + scale.valid_chromatic_pitches[i];
    Serial.printf("%-3s ", get_note_name_c(note));
  }
  Serial.println();
}

// Change return type and implementation
scale_pattern_t *make_scale_pattern_t_from_string(const char *scale_signature, const char *name) {
  scale_pattern_t* return_scale = new scale_pattern_t();
  return_scale->label = name;
  return_scale->interval_pattern = scale_signature;

  int idx = 1;
  int total_steps = 0;
  for (char *scale = (char*)scale_signature ; scale < scale_signature + strlen(scale_signature) ; scale++) {
    if (idx > PITCHES_PER_SCALE) {
      Serial.printf("make_scale_t_from_string: scale_signature[%i] = %c is too long!\n", scale - scale_signature, *scale);
      break;
    }
    if (*scale == ' ') continue;
    *scale = tolower(*scale);

    int next_step = 0;
    if (*scale == 'w') {
        next_step = 2;
    } else if (*scale == 'h') {
        next_step = 1;
    } else if (*scale == '+') {
        next_step = 3;
    } else {
        Serial.printf("make_scale_t_from_string: scale_signature[%i] = %c is invalid!\n", scale - scale_signature, *scale);
    }
    if (Serial) Serial.printf("index %i: input %c => step %i\n", idx, *scale, next_step);
    total_steps += next_step;
    if (idx==PITCHES_PER_SCALE && total_steps != 12) {
      Serial.printf("WARNING: scale does not correctly meet itself!  Reached %i, so it is off by %i.\n", total_steps, 12 - total_steps);
    } 
    //if (idx<PITCHES_PER_SCALE) {
      return_scale->steps[idx-1] = next_step;
    //}
    idx++;
  }

  /*if (rotation>0) {
    for (int i = 0 ; i < PITCHES_PER_SCALE ; i++) {
      return_scale->steps[i] = steps[(i + rotation) % PITCHES_PER_SCALE];
    }
  }*/

  /*for (int i = 1 ; i < PITCHES_PER_SCALE ; i++) {
    return_scale->valid_chromatic_pitches[i] = return_scale->valid_chromatic_pitches[i-1] + steps[i];
  }*/
  
  /*
  if (rotation>0) {
      scale_t tmp_scale;
      for (int i = 0 ; i < PITCHES_PER_SCALE ; i++) {
          int idx = (i + rotation) % PITCHES_PER_SCALE;
          tmp_scale.valid_chromatic_pitches[i] = return_scale->valid_chromatic_pitches[idx];
          Serial.printf("Rotating %i to %i\n", i, idx);
      }
      for (int i = 0 ; i < PITCHES_PER_SCALE ; i++) {
          return_scale->valid_chromatic_pitches[i] = (tmp_scale.valid_chromatic_pitches[i] - rotation);
      }
  }
  */

  return return_scale;
}

scale_t *make_scale_t_from_pattern(const scale_pattern_t *scale_signature, const char *name, int rotation) {
  scale_t *return_scale = new scale_t { name, scale_signature, rotation };

  // Initialize first pitch
  return_scale->valid_chromatic_pitches[0] = 0;

  for (int i = 1 ; i < PITCHES_PER_SCALE ; i++) {
      return_scale->valid_chromatic_pitches[i] = return_scale->valid_chromatic_pitches[i-1] + scale_signature->steps[(i - 1 + rotation) % PITCHES_PER_SCALE];
  }

  return return_scale;
}


void dump_all_scales_and_chords(bool all_inversions, bool debug) {
  Serial.printf("Outputting all scales and chords...\n");
  for (unsigned int i = 0 ; i < NUMBER_SCALES ; i++) {
      for (unsigned int root = 0 ; root < 12 ; root++) {
          const scale_t *scale = scales[(scale_index_t)i];

          //Serial.printf("For %s with root %s: ", scale->pattern->label, get_note_name_c(root));
          print_scale(root, *scale);

          for (int j = 0 ; j < PITCHES_PER_SCALE ; j++) {
              chord_identity_t chord;
              chord.degree = j+1;
              chord.inversion = 0;
              for (CHORD::Type t = 0 ; t < CHORD::NONE ; t++) {
                  chord.type = t;

                  for (unsigned int inversion = 0 ; inversion <= all_inversions ? MAX_INVERSIONS : 0 ; inversion++) {
                      chord.inversion = inversion;

                      chord_instance_t instance;
                      instance.set_from_chord_identity(chord, root, (scale_index_t)i);

                      int n;
                      for (size_t x = 0 ; x < PITCHES_PER_CHORD && ((n = get_quantise_pitch_chord_note(instance.chord_root, instance.chord.type, x, root, (scale_index_t)i, instance.chord.inversion, debug)) >= 0) ; x++) {
                          instance.set_pitch(x, n);
                      }

                      Serial.printf("\tChord %i:\t %s, inversion %i\t", j+1, chords[instance.chord.type].label, chord.inversion);
                      Serial.printf("\t%s\n", instance.get_pitch_string());
                  }
              }
          }
          Serial.println("----");
      }
      Serial.printf("----\n");
  }
  Serial.printf("Done.");
}

void dump_all_scales() {
  Serial.printf("Outputting all scales...\n");
  for (unsigned int root = 0 ; root < 12 ; root++) {
      Serial.printf("For root %s:\n", get_note_name_c(root));
      for (unsigned int i = 0 ; i < NUMBER_SCALES ; i++) {
          const scale_t *scale = scales[(scale_index_t)i];
          print_scale(root, *scale);
      }
      Serial.printf("----\n");
  }
  Serial.printf("Done.");
}

void dump_all_chords(bool all_inversions, bool debug) {
  Serial.printf("Outputting all chords in the current scale %s - %s...\n", get_note_name_c(get_global_scale_root()), scales[get_global_scale_type()]->label);
  for (int i = 0 ; i < PITCHES_PER_SCALE ; i++) {
      chord_identity_t chord;
      chord.degree = i+1;
      chord.inversion = 0;
      Serial.printf("For scale degree %i: %s\n", i+1, get_note_name_c(scales[get_global_scale_type()]->valid_chromatic_pitches[i]));
      for (CHORD::Type t = 0 ; t < CHORD::NONE ; t++) {
          chord.type = t;

          for (unsigned int inversion = 0 ; inversion <= all_inversions ? MAX_INVERSIONS : 0 ; inversion++) {
              chord.inversion = inversion;

              chord_instance_t instance;
              instance.set_from_chord_identity(chord, get_global_scale_root(), get_global_scale_type());

              int n;
              for (size_t x = 0 ; x < PITCHES_PER_CHORD && ((n = get_quantise_pitch_chord_note(instance.chord_root, instance.chord.type, x, get_global_scale_root(), get_global_scale_type(), instance.chord.inversion, debug)) >= 0) ; x++) {
                  instance.set_pitch(x, n);
              }

              Serial.printf("\tChord %i:\t %s, inversion %i\t", i+1, chords[instance.chord.type].label, chord.inversion);
              Serial.printf("\tNotes:\t%s\n", instance.get_pitch_string());
          }
      }
      Serial.println("----");
  }
}

#endif