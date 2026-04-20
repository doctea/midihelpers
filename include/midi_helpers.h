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
#define MIDI_NUM_NOTES	  128

#define MIDI_MAX_CHANNEL 16
#define MIDI_MIN_CHANNEL 1
#define MIDI_CHANNEL_OMNI 0


String get_note_name(int pitch);
const char *get_note_name_c(int pitch, int channel = 1);
extern const char *note_names[];
bool is_valid_note(int8_t byte);

// for doing lowest_note and highest_note
enum NOTE_LIMIT_MODE : int8_t {
    IGNORE, TRANSPOSE
};
inline NOTE_LIMIT_MODE& operator++(NOTE_LIMIT_MODE& m)    { m = static_cast<NOTE_LIMIT_MODE>(static_cast<int8_t>(m) + 1); return m; }
inline NOTE_LIMIT_MODE  operator++(NOTE_LIMIT_MODE& m, int) { NOTE_LIMIT_MODE t = m; ++m; return t; }
inline NOTE_LIMIT_MODE& operator--(NOTE_LIMIT_MODE& m)    { m = static_cast<NOTE_LIMIT_MODE>(static_cast<int8_t>(m) - 1); return m; }
inline NOTE_LIMIT_MODE  operator--(NOTE_LIMIT_MODE& m, int) { NOTE_LIMIT_MODE t = m; --m; return t; }
int8_t apply_note_limits(
    int8_t note, 
    NOTE_LIMIT_MODE lowest_note_mode, 
    NOTE_LIMIT_MODE highest_note_mode, 
    int8_t lowest_note, 
    int8_t highest_note
);


// interface for classes that can receive MIDI note data
class IMIDINoteTarget {
  public: 
    virtual void sendNoteOn(uint8_t pitch, uint8_t velocity, uint8_t channel) = 0;
    virtual void sendNoteOff(uint8_t pitch, uint8_t velocity, uint8_t channel) = 0;
};

// interface for classes that can receive MIDI CC data
class IMIDICCTarget {
  protected:
    bool enable_cc_output = true;

  public:
      virtual bool is_cc_output_enabled() {
          return enable_cc_output;
      }
      virtual void set_cc_output_enabled(bool enabled) {
          enable_cc_output = enabled;
      }
      virtual void sendControlChange(uint8_t cc_number, uint8_t value, uint8_t channel) {};
};

class IMIDIProxiedCCTarget : virtual public IMIDICCTarget {
    public:
        virtual void sendProxiedControlChange(uint8_t cc_number, uint8_t value, uint8_t channel) {};
};

class IMIDINoteAndCCTarget : virtual public IMIDICCTarget, virtual public IMIDINoteTarget {

};

// interface for classes that can receive gate messages (ie GateManager); todo: should also make envelope generators implement this
class IGateTarget {
    public:
      virtual void send_gate_on(int8_t bank, int8_t gate) {
          this->send_gate(bank, gate, true);
      }
      virtual void send_gate_off(int8_t bank, int8_t gate) {
          this->send_gate(bank, gate, false);
      }
      virtual void send_gate(int8_t bank, int8_t gate, bool state) = 0;
};


// todo: utilise this!
class IMIDIPitchBendTarget {
    public:
        virtual void sendPitchBend(int16_t bend, uint8_t channel) {};
};


struct midi_note_event_t {
    int8_t note = NOTE_OFF;
    int8_t velocity = MIDI_MAX_VELOCITY;
    int8_t channel = 0;
};
// todo: make this structure global and re-usable everywhere that we need it
// todo: make a saveloadlib-compatible SaveableMIDINoteArraySetting class that can save/load an array of these
// todo: maybe we need an SL_SCOPE_SNAPSHOT scope for things like this that we want to save/load but not necessarily have them be part of the regular scene/project settings (since they might be more 'ephemeral' or 'performance' settings than 'preference' settings)?

