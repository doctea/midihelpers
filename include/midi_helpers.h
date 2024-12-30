#if (defined __GNUC__) && (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 4) && (__GNUC_PATCHLEVEL__ > 1)
  #pragma GCC diagnostic ignored "-Wformat-truncation"
  #pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

#pragma once

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <cinttypes>
  #include <string.h>
#endif

#define NOTE_OFF -1

#define MIDI_MAX_VELOCITY 127
#define MIDI_MIN_VELOCITY 0
#define MIDI_MAX_CC       127
#define MIDI_MIN_CC       0
#define MIDI_MAX_NOTE     127
#define MIDI_MIN_NOTE     0
#define MIDI_NUM_NOTES    128

#define MIDI_MAX_CHANNEL 16
#define MIDI_MIN_CHANNEL 1
#define MIDI_CHANNEL_OMNI 0

String get_note_name(int pitch);
const char *get_note_name_c(int pitch, int channel = 1);
bool is_valid_note(int8_t byte);

extern const char *note_names[];

// interface for classes that can receive MIDI note data
class IMIDINoteTarget {
  public: 
    virtual void sendNoteOn(uint8_t pitch, uint8_t velocity, uint8_t channel) = 0;
    virtual void sendNoteOff(uint8_t pitch, uint8_t velocity, uint8_t channel) = 0;
};

// interface for classes that can receive MIDI CC data
class IMIDICCTarget {
    public:
        virtual void sendControlChange(uint8_t cc_number, uint8_t value, uint8_t channel) {};
};

class IMIDIProxiedCCTarget : virtual public IMIDICCTarget {
    public:
        virtual void sendProxiedControlChange(uint8_t cc_number, uint8_t value, uint8_t channel) {};
};

class IMIDINoteAndCCTarget : virtual public IMIDICCTarget, virtual public IMIDINoteTarget {

};
