#ifndef MIDI_HELPERS__INCLUDED
#define MIDI_HELPERS__INCLUDED

#include <Arduino.h>

#define NOTE_OFF -1

#define MIDI_MAX_VELOCITY 127
#define MIDI_MAX_NOTE     127

String get_note_name(int pitch);
const char *get_note_name_c(int pitch);
bool is_valid_note(int8_t byte);

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

int8_t quantise_pitch(int8_t pitch, int8_t root_note = SCALE_ROOT_A, int8_t scale_number = 0);

#endif

