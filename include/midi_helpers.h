#pragma GCC diagnostic ignored "-Wformat-truncation"
#pragma GCC diagnostic ignored "-Wstringop-truncation"

#ifndef MIDI_HELPERS__INCLUDED
#define MIDI_HELPERS__INCLUDED

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <cinttypes>
  #include <string.h>
#endif

#define NOTE_OFF -1

#define MIDI_MAX_VELOCITY 127
#define MIDI_MIN_VELOCITY 0
#define MIDI_MAX_NOTE     127
#define MIDI_MIN_NOTE     0

String get_note_name(int pitch);
const char *get_note_name_c(int pitch);
bool is_valid_note(int8_t byte);

extern const char *note_names[];

#endif