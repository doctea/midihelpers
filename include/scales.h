#ifndef SCALES__INCLUDED
#define SCALES__INCLUDED

#include "midi_helpers.h"

struct scale_t {
    const char *label;
    byte valid_chromatic_pitches[PITCHES_PER_SCALE];
};

const scale_t scales[] = {
    { "Major",            { 0, 2, 4, 5, 7, 9, 11 }},
    { "Minor (Natural)",  { 0, 2, 3, 5, 7, 8, 10 }},
    { "Minor (Melodic)",  { 0, 2, 3, 5, 7, 9, 11 }},
    { "Minor (Harmonic)", { 0, 2, 3, 5, 7, 8, 11 }},
    { "Lydian",           { 0, 2, 4, 6, 7, 9, 11 }},
    { "Whole-tone",       { 0, 2, 4, 6, 8, 10, (12) }},
    { "Minor (Hungarian)",{ 0, 2, 3, 6, 7, 8, 11 }}
};

#define NUMBER_SCALES (sizeof(scales)/sizeof(scale_t))

#endif