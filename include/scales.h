#ifndef SCALES_H__INCLUDED
#define SCALES_H__INCLUDED

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

struct scale_t {
    const char *label;
    byte valid_chromatic_pitches[PITCHES_PER_SCALE];
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
    { "Whole-tone",       { 0, 2, 4, 6, 8, 10, (12) }}
};
enum SCALE {
    MAJOR,
    MINOR_NATURAL,
    MINOR_MELODIC,
    MINOR_HARMONIC,
    MINOR_HUNGARIAN,
    LYDIAN,
    WHOLE_TONE
};
#define NUMBER_SCALES (sizeof(scales)/sizeof(scale_t))


#define PITCHES_PER_CHORD 4
struct chord_t {
    const char *label;
    int8_t degree_number[PITCHES_PER_CHORD];
};

const chord_t chords[] = {
    { "Triad",       { 0, 2, 4, -1 } },
    { "Sus 2",       { 0, 1, 4, -1 } },
    { "Sus 4",       { 0, 3, 4, -1 } },
    { "Seven",       { 0, 2, 4,  6 } },
    { "Oct+1",       { 0, 7, -1, -1 } },
    { "Oct+2",       { 0, 7, 14, -1 } },
    { "Oct+3",       { 0, 7, 14, 21 } },
};

namespace CHORD {
    typedef int Type;
    const int 
        TRIAD = 0,
        SUS2 = 1,
        SUS4 = 2,
        SEVENTH = 3,
        OCTAVE_1 = 4,
        OCTAVE_2 = 5,
        OCTAVE_3 = 6,
        NONE = 7;
};

#define NUMBER_CHORDS (sizeof(chords)/sizeof(chord_t))


class chord_instance_t {
    public:
    SCALE scale = SCALE::MAJOR;
    CHORD::Type chord_type = CHORD::NONE;
    int8_t chord_root = -1;
    int8_t pitches[PITCHES_PER_CHORD] = { -1, -1, -1, -1 };

    const char *get_label() {
        return chords[chord_type].label;
    }
    char pitch_string[40];
    const char *get_pitch_string() {
        snprintf(pitch_string, 40, "%s %s: %s, %s, %s, %s", get_note_name_c(chord_root), chord_type!=CHORD::NONE?chords[chord_type].label:"N/A", get_note_name_c(pitches[0]), get_note_name_c(pitches[1]), get_note_name_c(pitches[2]), get_note_name_c(pitches[3]));
        return pitch_string;
    }
    void set(CHORD::Type type, int8_t root) {
        //this->clear();
        this->chord_type = type;
        this->chord_root = root;
    }
    void set_chord_type(CHORD::Type type) {
        this->chord_type = type;
    }
    void set_chord_root(int8_t root) {
        this->chord_root = root;
    }
    void set_pitch(int note_number, int8_t pitch) {
        pitches[note_number] = pitch;
    }
    void clear() {
        this->chord_type = CHORD::NONE;
        this->chord_root = -1;
        memset(pitches, 0, sizeof(pitches));
    }
    /*void copy_from(chord_instance_t source) {
        memcpy(this, source, sizeof(chord_instance_t));
    }*/
};

SCALE& operator++(SCALE& orig);
SCALE& operator++(SCALE& orig, int);
SCALE& operator--(SCALE& orig);
SCALE& operator--(SCALE& orig, int);

int8_t quantise_pitch(int8_t pitch, int8_t root_note = SCALE_ROOT_A, SCALE scale_number = SCALE::MAJOR);
int8_t quantise_pitch_chord_note(int8_t pitch, CHORD::Type chord_number, int8_t note_of_chord, int8_t root_note, SCALE scale_number, bool debug = false);

#endif