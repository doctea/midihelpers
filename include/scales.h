#pragma once

#ifdef ENABLE_SCALES

#if (defined __GNUC__) && (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 3) && (__GNUC_PATCHLEVEL__ >= 1)
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    #pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif


#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <cinttypes>
#endif

#include "midi_helpers.h"

#define SCALE_ROOT_C       0
#define SCALE_ROOT_C_SHARP 1
#define SCALE_ROOT_D       2
#define SCALE_ROOT_D_SHARP 3
#define SCALE_ROOT_E       4
#define SCALE_ROOT_F       5
#define SCALE_ROOT_F_SHARP 6
#define SCALE_ROOT_G       7
#define SCALE_ROOT_G_SHARP 8
#define SCALE_ROOT_A       9
#define SCALE_ROOT_A_SHARP 10
#define SCALE_ROOT_B       11

#define PITCHES_PER_SCALE 7

#define SCALE_GLOBAL_ROOT (-1)

#define MAX_INVERSIONS     4

#define MAXIMUM_OCTAVE     9

struct scale_pattern_t {
    const char *label;
    int8_t valid_chromatic_pitches[PITCHES_PER_SCALE];
};

struct scale_t {
    const char *label;
    const scale_pattern_t *pattern = nullptr;
    int rotation = 0;
};

//scale_t *make_scale_t_from_string(const char *scale_signature, const char *name, int rotation = 0);
const scale_pattern_t *make_scale_pattern_t_from_string(const char *scale_pattern_name, const char *name, int rotation = 0);

const scale_pattern_t scale_patterns[] = {
    *make_scale_pattern_t_from_string("w w h w w w h", "Natural"),
    *make_scale_pattern_t_from_string("w h w w w w h", "Melodic"),
    *make_scale_pattern_t_from_string("w w h w h + h", "Harmonic Major"),
    *make_scale_pattern_t_from_string("w h w w h + h", "Harmonic Minor"),
    *make_scale_pattern_t_from_string("h + h w h + h", "Byzantine"),            // " this scale is commonly represented with the first and last half step each being represented with quarter tones:"
};

const scale_t scales[] = {
    { "Ionian", &scale_patterns[0], 0 },
    { "Dorian", &scale_patterns[0], 1 },
    { "Phrygian", &scale_patterns[0], 2 },
    { "Lydian", &scale_patterns[0], 3 },
    { "Mixolydian", &scale_patterns[0], 4 },
    { "Aeolian", &scale_patterns[0], 5 },
    { "Locrian", &scale_patterns[0], 6 },

    { "Melodic Minor", &scale_patterns[1], 0 },
    { "Dorian.b2", &scale_patterns[1], 1 },
    { "Lydian.aug", &scale_patterns[1], 2 },
    { "Lydian.dom", &scale_patterns[1], 3 },
    { "Mixolydian.b6", &scale_patterns[1], 4 },
    { "Locrian.#2", &scale_patterns[1], 5 },
    { "Superlocrian", &scale_patterns[1], 6 },

    { "Harm. Major", &scale_patterns[2], 0 },
    { "Dorian.b5", &scale_patterns[2], 1 },
    { "Phrygian.b4", &scale_patterns[2], 2 },
    { "Lydian.b3", &scale_patterns[2], 3 },
    { "Mixolydian.b2", &scale_patterns[2], 4 },
    { "Lydian.#2", &scale_patterns[2], 5 },
    { "Locrian.bb7", &scale_patterns[2], 6 },

    { "Harm. Minor", &scale_patterns[3], 0 },
    { "Locrian.#6", &scale_patterns[3], 1 },
    { "Ionian.#5", &scale_patterns[3], 2 },
    { "Dorian.#4", &scale_patterns[3], 3 },
    { "Phrygian dom", &scale_patterns[3], 4 },
    { "Lydian.#2", &scale_patterns[3], 5 },
    { "Superlocrian.bb7", &scale_patterns[3], 6 },

    { "Double harm.maj", &scale_patterns[4], 0 },
    { "Lydian.#2.#6", &scale_patterns[4], 1 },
    { "Ultraphrygian", &scale_patterns[4], 2 },
    { "Hungarian minor", &scale_patterns[4], 3 },
    { "Oriental", &scale_patterns[4], 4 },
    { "Ionian.#2.#5", &scale_patterns[4], 5 },
    { "Locrian.b3.bb7", &scale_patterns[4], 6 },
};
    
enum SCALE {
    MAJOR,
    MINOR_NATURAL,
    MINOR_MELODIC,
    MINOR_HARMONIC,
    MINOR_HUNGARIAN,
    LYDIAN,
    MIXOLYDIAN,
    PHRYGIAN,
    WHOLE_TONE,
    //test,
    GLOBAL
};
#define NUMBER_SCALES ((sizeof(scales)/sizeof(scale_t)))    // uses size of real scales[] array, ie existant scales, rather than the SCALE enum (which has an extra value to represent GLOBAL)

#define PITCHES_PER_CHORD 4
#define MAXIMUM_INVERSIONS  PITCHES_PER_CHORD
struct chord_t {
    const char *label;
    int8_t degree_number[PITCHES_PER_CHORD];
};

const chord_t chords[] = {
    { "Triad",       { 0, 2, 4, NOTE_OFF  } },
    { "Sus 2",       { 0, 1, 4, NOTE_OFF  } },
    { "Sus 4",       { 0, 3, 4, NOTE_OFF  } },
    { "Seven",       { 0, 2, 4,  6        } },
    { "Ninth",       { 0, 2, 6,  8        } },
    { "Oct+1",       { 0, 7,    NOTE_OFF, NOTE_OFF } },
    { "Oct+2",       { 0, 7, 14, NOTE_OFF } },
    { "Oct+3",       { 0, 7, 14, 21       } },
};

namespace CHORD {
    typedef int Type;
    const int 
        GLOBAL = -1,
        TRIAD = 0,
        SUS2 = 1,
        SUS4 = 2,
        SEVENTH = 3,
        NINTH = 4,
        OCTAVE_1 = 5,
        OCTAVE_2 = 6,
        OCTAVE_3 = 7,
        NONE = 8;
};

#define NUMBER_CHORDS (sizeof(chords)/sizeof(chord_t))

class scale_identity_t {
    public:
    SCALE scale_number = SCALE::MAJOR;
    int8_t root_note = SCALE_ROOT_C;
};

class chord_identity_t {
    public:
    CHORD::Type type = CHORD::TRIAD;
    int8_t degree = -1;
    int8_t inversion = 0;
    //int8_t velocity = MIDI_MAX_VELOCITY;

    bool diff(chord_identity_t other) {
        if (
            this->type!=other.type ||
            this->degree!=other.degree ||
            this->inversion!=other.inversion
        ) return true;

        return false;
    }
    bool is_valid_chord() {
        return this->degree>0;
    }
};

class chord_instance_t {
    public:
    //SCALE scale = SCALE::MAJOR;
    scale_identity_t scale;
    chord_identity_t chord;
    //CHORD::Type type = CHORD::NONE;
    int8_t chord_root = NOTE_OFF;
    //int8_t inversion = 0;
    int8_t velocity = MIDI_MAX_VELOCITY;
    bool changed = true;
    int8_t pitches[PITCHES_PER_CHORD] = { NOTE_OFF, NOTE_OFF, NOTE_OFF, NOTE_OFF };

    const char *get_label() {
        return chords[chord.type].label;
    }
    char pitch_string[40];
    const char *get_pitch_string() {
        if (changed) {
            snprintf(pitch_string, 40, 
                "%3s %6s: %3s,%3s,%3s,%3s inv%1i,ve=%3i", 
                get_note_name_c(chord_root), 
                chord.type!=CHORD::NONE ? chords[chord.type].label : "N/A", 
                get_note_name_c(pitches[0]), 
                get_note_name_c(pitches[1]), 
                get_note_name_c(pitches[2]), 
                get_note_name_c(pitches[3]), 
                chord.inversion,
                velocity
            );
            changed = false;
        }
        return pitch_string;
    }
    void set(CHORD::Type type, int8_t root, int8_t inversion = 0, int8_t velocity = 0) {
        //this->clear();
        this->chord.type = type;
        this->chord_root = root;
        this->chord.inversion = inversion;
        this->velocity = velocity;
        this->changed = true;
    }
    void set_from_chord_identity(chord_identity_t chord_identity, int8_t root, SCALE scale) {
        this->chord.type = chord_identity.type;
        //this->chord_root = root;
        this->chord_root = root + scales[scale].pattern->valid_chromatic_pitches[chord_identity.degree-1];
        this->chord.inversion = chord_identity.inversion;
        this->scale.scale_number = scale;
        this->changed = true;
    }
    void set_chord_type(CHORD::Type type) {
        this->chord.type = type;
        this->changed = true;
    }
    void set_chord_root(int8_t root) {
        this->chord_root = root;
        this->changed = true;
    }
    void set_pitch(int note_number, int8_t pitch) {
        pitches[note_number] = pitch;
        this->changed = true;
    }
    void set_inversion(int8_t inversion) {
        this->chord.inversion = inversion;
        this->changed = true;
    }
    void set_velocity(int8_t velocity) {
        this->velocity = velocity;
        this->changed = true;
    }
    void clear() {
        this->chord.type = CHORD::NONE;
        this->chord_root = -1;
        this->chord.inversion = 0;
        this->velocity = 0;
        memset(pitches, -1, sizeof(pitches));
        this->changed = true;
    }
    /*void copy_from(chord_instance_t source) {
        memcpy(this, source, sizeof(chord_instance_t));
    }*/
};

SCALE& operator++(SCALE& orig);
SCALE& operator++(SCALE& orig, int);
SCALE& operator--(SCALE& orig);
SCALE& operator--(SCALE& orig, int);

// todo: make use of this to replace the scale and chord variables
struct quantise_settings_t {
    int8_t scale_root = SCALE_ROOT_C;
    SCALE scale_type = SCALE::MAJOR;
    chord_identity_t chord_identity = {CHORD::TRIAD, -1, 0};
};

void set_global_scale_identity_target(scale_identity_t *scale);
void set_global_chord_identity_target(chord_identity_t *chord_identity);

int8_t get_effective_scale_root(int8_t scale_root);
SCALE get_effective_scale_type(SCALE scale_number);

int8_t get_global_scale_root();
SCALE get_global_scale_type();
scale_identity_t *get_global_scale_identity();
int8_t get_global_chord_degree();


chord_identity_t get_global_chord_identity();
int8_t get_effective_chord_degree(int8_t chord_degree);
CHORD::Type get_effective_chord_type(CHORD::Type chord_type);
int8_t get_effective_chord_inversion(int8_t inversion);

int8_t quantise_pitch_to_scale(int8_t pitch, int8_t root_note = SCALE_GLOBAL_ROOT, SCALE scale_number = SCALE::GLOBAL, bool debug = false); //, chord_identity_t chord_identity = {CHORD::TRIAD, -1, 0});
int8_t quantise_pitch_to_chord(int8_t pitch, int8_t quantise_to_nearest_range = 0, int8_t scale_root = SCALE_GLOBAL_ROOT, SCALE scale_number = SCALE::GLOBAL, chord_identity_t chord_identity = {CHORD::GLOBAL, -1, 0}, bool debug = false);
int8_t get_quantise_pitch_chord_note(int8_t pitch, CHORD::Type chord_number, int8_t note_of_chord, int8_t root_note = SCALE_GLOBAL_ROOT, SCALE scale_number = SCALE::GLOBAL, int inversion = 0, bool debug = false);

// gets the pitch note number for a scale degree 
int8_t quantise_get_root_pitch_for_degree(int8_t degree, int8_t root_note = SCALE_GLOBAL_ROOT, SCALE scale_number = SCALE::GLOBAL);


void print_scale(int8_t root_note, SCALE scale_number); //, bool debug = false);
void print_scale(int8_t root_note, scale_t scale);

#endif