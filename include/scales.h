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
    { "Triad",           { 0, 2, 4, -1 } },
    { "Sus2",            { 0, 1, 4, -1 } },
    { "Sus4",            { 0, 3, 4, -1 } },
    { "7",               { 0, 2, 4,  6 } },
};
enum CHORD {
    TRIAD,
    SUS2,
    SUS4,
    SEVENTH,
    NONE
};
#define NUMBER_CHORDS (sizeof(chords)/sizeof(chord_t))

SCALE& operator++(SCALE& orig);
SCALE& operator++(SCALE& orig, int);
SCALE& operator--(SCALE& orig);
SCALE& operator--(SCALE& orig, int);

int8_t quantise_pitch(int8_t pitch, int8_t root_note = SCALE_ROOT_A, SCALE scale_number = SCALE::MAJOR);
int8_t quantise_pitch_chord_note(int8_t pitch, CHORD chord_number, int8_t note_of_chord, int8_t root_note, SCALE scale_number);

#endif