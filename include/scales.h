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

#define MAXIMUM_OCTAVE     9

struct scale_t {
    const char *label;
    int8_t valid_chromatic_pitches[PITCHES_PER_SCALE];
};

const scale_t scales[] = {
    { "Major",            { 0, 2, 4, 5, 7, 9, 11 }},
    { "Minor (Natural)",  { 0, 2, 3, 5, 7, 8, 10 }},
    { "Minor (Melodic)",  { 0, 2, 3, 5, 7, 9, 11 }},
    { "Minor (Harmonic)", { 0, 2, 3, 5, 7, 8, 11 }},
    { "Minor (Hungarian)",{ 0, 2, 3, 6, 7, 8, 11 }},
    { "Lydian",           { 0, 2, 4, 6, 7, 9, 11 }},
    { "Mixolydian",       { 0, 2, 4, 5, 7, 9, 10 }},
    { "Phrygian",         { 0, 1, 3, 5, 7, 8, 10 }},
    { "Whole-tone",       { 0, 2, 4, 6, 8, 10, (12) }},
    //{ "TEST",             { 0, 1, 2, 3, 4, 5, 6 }},
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
    { "Seven",       { 0, 2, 4,  6  } },
    { "Ninth",       { 0, 2, 6,  8  } },
    { "Oct+1",       { 0, 7, NOTE_OFF, NOTE_OFF } },
    { "Oct+2",       { 0, 7, 14, NOTE_OFF } },
    { "Oct+3",       { 0, 7, 14, 21 } },
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

class chord_identity_t {
    public:
    CHORD::Type type = CHORD::TRIAD;
    int8_t degree = -1;
    int8_t inversion = 0;
    //int8_t velocity = MIDI_MAX_VELOCITY;
};

class chord_instance_t {
    public:
    SCALE scale = SCALE::MAJOR;
    CHORD::Type type = CHORD::NONE;
    int8_t chord_root = NOTE_OFF;
    int8_t inversion = 0;
    int8_t velocity = MIDI_MAX_VELOCITY;
    bool changed = true;
    int8_t pitches[PITCHES_PER_CHORD] = { NOTE_OFF, NOTE_OFF, NOTE_OFF, NOTE_OFF };

    const char *get_label() {
        return chords[type].label;
    }
    char pitch_string[40];
    const char *get_pitch_string() {
        if (changed) {
            snprintf(pitch_string, 40, 
                "%3s %6s: %3s,%3s,%3s,%3s inv%1i,ve=%3i", 
                get_note_name_c(chord_root), 
                type!=CHORD::NONE ? chords[type].label : "N/A", 
                get_note_name_c(pitches[0]), 
                get_note_name_c(pitches[1]), 
                get_note_name_c(pitches[2]), 
                get_note_name_c(pitches[3]), 
                inversion,
                velocity
            );
            changed = false;
        }
        return pitch_string;
    }
    void set(CHORD::Type type, int8_t root, int8_t inversion = 0, int8_t velocity = 0) {
        //this->clear();
        this->type = type;
        this->chord_root = root;
        this->inversion = inversion;
        this->velocity = velocity;
        this->changed = true;
    }
    void set_chord_type(CHORD::Type type) {
        this->type = type;
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
        this->inversion = inversion;
        this->changed = true;
    }
    void set_velocity(int8_t velocity) {
        this->velocity = velocity;
        this->changed = true;
    }
    void clear() {
        this->type = CHORD::NONE;
        this->chord_root = -1;
        this->inversion = 0;
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


extern int8_t   *global_scale_root;
extern SCALE    *global_scale_type;

void set_global_scale_root_target(int8_t *root_note);
void set_global_scale_type_target(SCALE *scale_type);
void set_global_chord_identity_target(chord_identity_t *chord_identity);

int8_t get_effective_scale_root(int8_t scale_root);
SCALE get_effective_scale_type(SCALE scale_number);

int8_t get_global_scale_root();
SCALE get_global_scale_type();
int8_t get_global_chord_degree();


chord_identity_t get_global_chord_identity();
int8_t get_effective_chord_degree(int8_t chord_degree);
CHORD::Type get_effective_chord_type(CHORD::Type chord_type);
int8_t get_effective_chord_inversion(int8_t inversion);

int8_t quantise_pitch(int8_t pitch, int8_t root_note = SCALE_GLOBAL_ROOT, SCALE scale_number = SCALE::GLOBAL, bool debug = false); //, chord_identity_t chord_identity = {CHORD::TRIAD, -1, 0});
int8_t quantise_chord(int8_t pitch, int8_t quantise_to_nearest_range = 0, int8_t scale_root = SCALE_GLOBAL_ROOT, SCALE scale_number = SCALE::GLOBAL, chord_identity_t chord_identity = {CHORD::GLOBAL, -1, 0}, bool debug = false);
int8_t get_quantise_pitch_chord_note(int8_t pitch, CHORD::Type chord_number, int8_t note_of_chord, int8_t root_note = SCALE_GLOBAL_ROOT, SCALE scale_number = SCALE::GLOBAL, int inversion = 0, bool debug = false);

// gets the pitch note number for a scale degree 
int8_t quantise_get_root_pitch_for_degree(int8_t degree, int8_t root_note = SCALE_GLOBAL_ROOT, SCALE scale_number = SCALE::GLOBAL);


void print_scale(int8_t root_note, SCALE scale_number); //, bool debug = false);

#endif