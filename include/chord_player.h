// todo: abstract handling of playing chords currently in usb_midi_clocker's behaviour_cvinput, so that we can re-use this in Microlidian

/*#include <Arduino.h>
#include "scales.h"

class ChordPlayer<int PPQN = 24> {
    public:

    bool is_playing = false;
    int last_note = -1, current_note = -1, current_raw_note = -1;
    CHORD::Type last_chord = CHORD::NONE, current_chord = CHORD::NONE, selected_chord_number = CHORD::NONE;
    unsigned long note_started_at_tick = 0;
    int32_t note_length_ticks = PPQN;
    int32_t trigger_on_ticks = 0;   // 0 = on change

    chord_instance_t current_chord_data;
    chord_instance_t last_chord_data;


};*/