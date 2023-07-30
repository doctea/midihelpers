#ifndef MIDI_HELPERS__INCLUDED
#define MIDI_HELPERS__INCLUDED

#include <Arduino.h>

#define NOTE_OFF -1

#define MIDI_MAX_VELOCITY 127
#define MIDI_MAX_NOTE     127

String get_note_name(int pitch);
const char *get_note_name_c(int pitch);
bool is_valid_note(int8_t byte);

extern const char *note_names[];

#endif