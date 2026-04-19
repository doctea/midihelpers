#pragma once

// Event-driven chord renderer reused across outputs (timing/triggering now lives outside this class).
#if defined(ENABLE_SCALES)

#include <Arduino.h>

#include "bpm.h"
#include "scales.h"

#include "functional-vlpp.h"

#ifdef ENABLE_SCREEN
    #include "mymenu/menuitems_scale.h"
    #include "mymenu/menuitems_harmony.h"
#endif

#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
#endif
class ChordPlayer 
#ifdef ENABLE_STORAGE
    : public SHDynamic<0, 6> // no children; core chord/scale/playback settings
#endif
    {
    public:

        using setter_func_def = vl::Func<void(int8_t channel, int8_t note, int8_t velocity)>;
        setter_func_def receive_note_on, receive_note_off;
        setter_func_def receive_note_on_bass, receive_note_off_bass;
        setter_func_def receive_note_on_topline, receive_note_off_topline;

        ChordPlayer(
            setter_func_def receive_note_on, 
            setter_func_def receive_note_off, 
            setter_func_def receive_note_on_bass = [](int8_t channel, int8_t note, int8_t velocity) -> void {}, 
            setter_func_def receive_note_off_bass = [](int8_t channel, int8_t note, int8_t velocity) -> void {},
            setter_func_def receive_note_on_topline = [](int8_t channel, int8_t note, int8_t velocity) -> void {},
            setter_func_def receive_note_off_topline = [](int8_t channel, int8_t note, int8_t velocity) -> void {}
        ) {
            this->receive_note_on = receive_note_on;
            this->receive_note_off = receive_note_off;
            this->receive_note_on_bass = receive_note_on_bass;
            this->receive_note_off_bass = receive_note_off_bass;
            this->receive_note_on_topline = receive_note_on_topline;
            this->receive_note_off_topline = receive_note_off_topline;

            #ifdef ENABLE_STORAGE
                this->set_path_segment("ChordPlayer");
            #endif
        }
        bool debug = false;

        bool is_playing_chord = false;
        bool is_playing_bass = false;
        bool is_playing_topline = false;
        int8_t current_note = NOTE_OFF;
        int8_t current_bass_note = NOTE_OFF, current_topline_note = NOTE_OFF;
        CHORD::Type last_chord = CHORD::NONE, current_chord = CHORD::NONE, selected_chord_number = CHORD::NONE;
        int8_t inversion = 0;

        chord_instance_t current_chord_data;
        chord_instance_t last_chord_data;

        #ifdef DEBUG_VELOCITY
            int8_t velocity = MIDI_MAX_VELOCITY;
        #endif

        bool quantise = false, play_chords = false;
        scale_index_t scale = SCALE_GLOBAL; //SCALE::MAJOR;
        int8_t scale_root = SCALE_GLOBAL_ROOT; //-1; //SCALE_ROOT_C;
 
        uint8_t channel = 0;
        #ifdef CVINPUT_CONFIGURABLE_CHANNEL
            uint8_t get_channel() { return channel; }
            void set_channel(uint8_t channel) { this->channel = channel; }
        #endif

        void set_scale(scale_index_t scale) {
            this->scale = scale;
        }
        scale_index_t get_scale() {
            return this->scale;
        }
        void set_scale_root(int8_t scale_root) {
            this->scale_root = scale_root;
        }
        int8_t get_scale_root() {
            return this->scale_root;
        }
        void set_quantise(bool quantise) {
            trigger_off_for_pitch_because_changed(this->current_note);
            this->quantise = quantise;
        }
        bool is_quantise() {
            return this->quantise;
        }
        void set_play_chords(bool play_chords) {
            trigger_off_for_pitch_because_changed(this->current_note);
            this->play_chords = play_chords;
        }
        bool is_play_chords() {
            return this->play_chords;
        }

        int8_t get_inversion() {
            return inversion;
        }
        void set_inversion(int8_t inversion) {
            /*if (this->inversion!=inversion)
                Serial.printf("%s#set_inversion(%i) (was previously %i)\n", this->get_label(), inversion, this->inversion);*/
            this->inversion = inversion;
        }
        void set_selected_chord(CHORD::Type chord) {
            /*if (this->selected_chord_number!=chord)
                Serial.printf("%s#set_selected_chord(%i) aka %s\n", this->get_label(), chord, chords[chord].label);*/
            this->selected_chord_number = chord;
        }
        CHORD::Type get_selected_chord() {
            return this->selected_chord_number;
        }

        void play_bass_note(int8_t pitch, uint8_t velocity = MIDI_MAX_VELOCITY) {
            if (is_valid_note(pitch)) {
                if (this->debug) Serial.printf("play_bass_note: Playing bass note %i (%s)\n", pitch, get_note_name_c(pitch));
                this->receive_note_on_bass(channel, pitch, velocity);
                this->current_bass_note = pitch;
                this->is_playing_bass = true;
            }
        }
        void play_topline_note(int8_t pitch, uint8_t velocity = MIDI_MAX_VELOCITY) {
            if (is_valid_note(pitch)) {
                if (this->debug) Serial.printf("play_topline_note: Playing topline note %i (%s)\n", pitch, get_note_name_c(pitch));
                this->receive_note_on_topline(channel, pitch, velocity);
                this->current_topline_note = pitch;
                this->is_playing_topline = true;
            }
        }
        void stop_bass_note() {
            if (is_valid_note(this->current_bass_note)) {
                if (this->debug) Serial.printf("stop_bass_note: Stopping bass note %i (%s)\n", this->current_bass_note, get_note_name_c(this->current_bass_note));
                this->receive_note_off_bass(channel, this->current_bass_note, 0);
            }
            this->current_bass_note = NOTE_OFF;
            this->is_playing_bass = false;
        }
        void stop_topline_note() {
            if (is_valid_note(this->current_topline_note)) {
                if (this->debug) Serial.printf("stop_topline_note: Stopping topline note %i (%s)\n", this->current_topline_note, get_note_name_c(this->current_topline_note));
                this->receive_note_off_topline(channel, this->current_topline_note, 0);
            }
            this->current_topline_note = NOTE_OFF;
            this->is_playing_topline = false;
        }

        void change_bass_note(int8_t new_note, uint8_t new_velocity) {
            if (is_valid_note(this->current_bass_note)) {
                if (this->debug) Serial.printf("change_bass_note: Changing bass note %i (%s) to %i (%s)\n", this->current_bass_note, get_note_name_c(this->current_bass_note), new_note, get_note_name_c(new_note));
                this->stop_bass_note();
                this->play_bass_note(new_note, new_velocity);
            }
        }
        void change_topline_note(int8_t new_note, uint8_t new_velocity) {
            if (is_valid_note(this->current_topline_note)) {
                if (this->debug) Serial.printf("change_topline_note: Changing topline note %i (%s) to %i (%s)\n", this->current_topline_note, get_note_name_c(this->current_topline_note), new_note, get_note_name_c(new_note));
                this->stop_topline_note();
                this->play_topline_note(new_note, new_velocity);
            }
        }

        void stop_chord() {
            if (is_valid_note(this->current_chord_data.chord_root)) {
                if (this->debug) Serial.printf("stop_chord(): Stopping chord degree root %i\n", this->current_chord_data.chord_root);
                this->stop_chord(this->current_chord_data);
            }
            //this->stop_chord(this->current_chord_data);
        }
        void stop_chord(chord_instance_t chord) {
            if (this->is_playing_chord) {
                if (this->debug) Serial.printf("stop_chord(): Stopping chord degree root %i\n", this->current_chord_data.chord_root);
                this->stop_chord(chord.chord_root, chord.chord.type, chord.chord.inversion, chord.velocity);
            }
        }

        void stop_chord(int8_t pitch, CHORD::Type chord_number = CHORD::TRIAD, int8_t inversion = 0, uint8_t velocity = 0) {
            if (debug) Serial.printf("\t---\nstop_chord: Stopping chord for %i (%s) - chord type %s, inversion %i\n", pitch, get_note_name_c(pitch), chords[chord_number].label, inversion);

            // loop over the playing notes in the chord and stop them
            for (size_t i = 0 ; i < PITCHES_PER_CHORD /*&& ((n = this->current_chord_data.pitches[i]) >= 0)*/ ; i++) {
                int8_t n = this->current_chord_data.pitches[i];
                if (debug) Serial.printf("\t\tStopping chord note\t[%i/%i]: %i\t(%s)\n", i+1, PITCHES_PER_CHORD, n, get_note_name_c(n));
                if (is_valid_note(n))
                    receive_note_off(channel, n, velocity);
            }

            // stop the bass note using the receive_note_off_bass callback
            if (is_valid_note(current_bass_note)) {
                stop_bass_note();
            } else {
                if (debug) Serial.printf("\t\tNo bass note to stop\n");
            }

            // stop the topline note using the receive_note_off_topline callback
            if (is_valid_note(current_topline_note)) {
                stop_topline_note();
            } else {
                if (debug) Serial.printf("\t\tNo topline note to stop\n");
            }
            
            last_chord = this->current_chord;
            this->last_chord_data = current_chord_data;
            this->is_playing_chord = false;
            this->current_chord_data.clear();
            if (debug) Serial.println("---");
        }

        void play_chord(chord_identity_t chord, uint8_t velocity = MIDI_MAX_VELOCITY) {
            int8_t pitch = quantise_get_root_pitch_for_degree(chord.degree);
            this->play_chord(pitch, chord.type, chord.inversion, velocity);
        }
        void play_chord(int8_t pitch, CHORD::Type chord_number = CHORD::TRIAD, int8_t inversion = 0, uint8_t velocity = MIDI_MAX_VELOCITY) {
            if (debug) Serial.printf("\t--- play_chord: playing chord for %i (%s) - chord type %s, inversion %i\n", pitch, get_note_name_c(pitch), chords[chord_number].label, inversion);
            if (is_playing_chord)
                this->stop_chord(this->current_chord_data);
            int8_t n = -1;
            this->current_chord_data.clear();
            this->current_chord_data.set(chord_number, pitch, inversion, velocity);
            //this->last_chord = current_chord;
            current_chord = chord_number;
            is_playing_chord = true;

            uint8_t bass_note = MIDI_MAX_NOTE+1;    // for tracking lowest note so that we can send bass note separately if desired; needs to be unsigned
            int8_t top_note = MIDI_MIN_NOTE-1;        // for tracking highest note so that we can send topline note separately if desired; needs to be signed

            int8_t previously_played_note = -1; // avoid duplicating notes, like what happens sometimes when playing inverted +octaved chords..!
            uint32_t time = millis();
            for (size_t i = 0 ; i < PITCHES_PER_CHORD && ((n = get_quantise_pitch_chord_note(pitch, chord_number, i, this->get_scale_root(), this->get_scale(), inversion, this->debug)) >= 0) ; i++) {
                this->current_chord_data.set_pitch(i, n);
                if (debug) Serial.printf("\t\tPlaying note\t[%i/%i]: %i\t(%s)\n", i+1, PITCHES_PER_CHORD, n, get_note_name_c(n));
                if (n!=previously_played_note) {
                    receive_note_on(channel, n, velocity);
                    if (is_valid_note(n) && n < bass_note)
                        bass_note = n;
                    else if (is_valid_note(n)) {
                        if (debug) Serial.printf("\t\tNote %i isn't less than already-known lowest bass note (%i)\n", n, bass_note);
                    }
                    if (is_valid_note(n) && n > top_note)
                        top_note = n;
                    else if (is_valid_note(n)) {
                        if (debug) Serial.printf("\t\tNote %i isn't greater than already-known highest topline note (%i)\n", n, top_note);
                    }
                } else {
                    if (debug) Serial.printf("\t\tSkipping note\t[%i/%i]: %i\t(%s)\n", i+1, PITCHES_PER_CHORD, n, get_note_name_c(n));
                }
                previously_played_note = n;
            }
            if (debug) Serial.printf("play_chord took %i ms to generate chord notes\n", millis()-time);

            // play the lowest note using the receive_note_on_bass callback
            if (is_valid_note(bass_note)) {
                play_bass_note(bass_note, velocity);
            } else {
                if (debug) Serial.printf("\t\tNo bass note to play - got %i\n", bass_note);
            }
            // play the highest note using the receive_note_on_topline callback
            if (is_valid_note(top_note)) {
                play_topline_note(top_note, velocity);
            } else {
                if (debug) Serial.printf("\t\tNo topline note to play - got %i\n", top_note);
            }
            if (debug) Serial.println("---");

        }

        void stop_all() {
            this->trigger_off_for_pitch_because_changed(this->current_note);
        }

        virtual void trigger_off_for_pitch_because_length(int8_t pitch, uint8_t velocity = MIDI_MIN_VELOCITY) {
            // don't reset current_note so that we don't retrigger the same note again immediately
            if (is_playing_chord)
                this->stop_chord(this->current_chord_data);
            else {
                if (is_valid_note(this->current_note))
                    this->receive_note_off(channel, this->current_note, 0);
                if (is_valid_note(this->current_bass_note))
                    stop_bass_note();
                if (is_valid_note(this->current_topline_note))
                    stop_topline_note();
            }
            this->current_note = NOTE_OFF;
        }
        virtual void trigger_off_for_pitch_because_changed(int8_t pitch, uint8_t velocity = MIDI_MIN_VELOCITY) {
            if (is_playing_chord)
                this->stop_chord(this->current_chord_data);
            else {
                if (is_valid_note(this->current_note))
                    this->receive_note_off(channel, this->current_note, 0);
                if (is_valid_note(this->current_bass_note)) 
                    stop_bass_note();
                if (is_valid_note(this->current_topline_note))
                    stop_topline_note();
            }
            this->current_note = NOTE_OFF;
        }
        virtual void trigger_on_for_pitch(int8_t pitch, uint8_t velocity = MIDI_MAX_VELOCITY, CHORD::Type chord_number = CHORD::TRIAD, int8_t inversion = 0) {
            if (this->is_playing_chord || is_valid_note(this->current_note))
                this->trigger_off_for_pitch_because_changed(this->current_note);

            if (!is_valid_note(pitch))
                return;
            if (this->is_quantise()) {
                pitch = quantise_pitch_to_scale(pitch, this->scale_root, this->scale);
            }
            this->current_note = pitch;


            #ifdef DEBUG_VELOCITY
                this->velocity = velocity;
            #endif
            if (!is_quantise() || !is_play_chords() || this->selected_chord_number==CHORD::NONE) {
                this->receive_note_on(channel, pitch, velocity);
                stop_bass_note();
                stop_topline_note();
            } else
                this->play_chord(pitch, chord_number, inversion, velocity);
        }

    #ifdef ENABLE_SCREEN
        // for caching available_values to save a bit of RAM
        #if __cplusplus >= 201703L
            inline static LinkedList<LambdaSelectorControl<CHORD::Type>::option> *chord_type_control_options;
        #else
            static LinkedList<LambdaSelectorControl<CHORD::Type>::option> *chord_type_control_options;
        #endif

        FLASHMEM
        LinkedList<MenuItem *> *make_menu_items(LinkedList<MenuItem *> *menuitems = nullptr) {
            if (menuitems==nullptr)
                menuitems = new LinkedList<MenuItem *>();

            #ifdef DEBUG_VELOCITY
                DirectNumberControl<int8_t> *velocity_control = new DirectNumberControl<int8_t>("Velocity", &this->velocity, 127, 0, 127);
                menuitems->add(velocity_control);
            #endif

            #ifdef CVINPUT_CONFIGURABLE_CHANNEL
                menuitems->add(new LambdaNumberControl<byte>("Channel", [=](byte v) -> void { this->set_channel(v); }, [=]() -> byte { return this->get_channel(); }));
            #endif

            //menuitems->add(new ToggleControl<bool>("Debug", &this->debug));

            // TODO: move scale/key and quantise items to a dedicated class that can be re-used
            // TODO: make mono, fake-poly, and true-poly versions of class:-
            //          mono forces one note at a time, and doesn't offer auto-chord function?
            //          fake-poly offers auto-chord functions
            //          true-poly doesn't offer auto-chord functions
            //          all versions offer quantisation to scale
            menuitems->add(new LambdaScaleMenuItemBar(
                "Scale / Key", 
                [=](scale_index_t scale) -> void { this->set_scale(scale); }, 
                [=]() -> scale_index_t { return this->get_scale(); },
                [=](int8_t scale_root) -> void { this->set_scale_root(scale_root); },
                [=]() -> int8_t { return this->get_scale_root(); },
                true
            ));

            SubMenuItemBar *bar = new SubMenuItemBar("Quantise / chords");
            bar->add(new LambdaToggleControl("Quantise",    
                [=](bool v) -> void { this->set_quantise(v); },
                [=]() -> bool { return this->is_quantise(); }
            ));
            bar->add(new LambdaToggleControl("Play chords", 
                [=](bool v) -> void { this->set_play_chords(v); },
                [=]() -> bool { return this->is_play_chords(); }
            ));

            LambdaSelectorControl<CHORD::Type> *selected_chord_control = new LambdaSelectorControl<CHORD::Type>(
                "Chord", 
                [=](CHORD::Type chord_type) -> void { this->set_selected_chord(chord_type); }, 
                [=]() -> CHORD::Type { return this->get_selected_chord(); },
                nullptr, true
            );
            if (chord_type_control_options==nullptr) {
                for (size_t i = 0 ; i < NUMBER_CHORDS ; i++) {
                    selected_chord_control->add_available_value(static_cast<CHORD::Type>(i), chords[i].label);
                }
                chord_type_control_options = selected_chord_control->get_available_values();
            } else {
                selected_chord_control->set_available_values(chord_type_control_options);
            }

            bar->add(new LambdaNumberControl<int8_t>(
                "Inversion", 
                [=](int8_t v) -> void { this->set_inversion(v); }, 
                [=]() -> int8_t { return this->get_inversion(); },
                nullptr, 0, MAX_INVERSIONS, true
            ));
            bar->add(selected_chord_control);

            menuitems->add(bar);

            menuitems->add(new ChordMenuItem("Current chord",   &this->current_chord_data));
            menuitems->add(new ChordMenuItem("Last chord",      &this->last_chord_data));

            menuitems->add(new ToggleControl<bool>("Debug", &this->debug));

            return menuitems;
        }

    #endif

    #ifdef ENABLE_STORAGE
        virtual void setup_saveable_settings() override {
            ISaveableSettingHost::setup_saveable_settings();

            register_setting(new LSaveableSetting<bool>(
                    "quantise", 
                    "ChordPlayer",
                    &this->quantise,
                    [=](bool v) -> void { this->set_quantise(v); },
                    [=]() -> bool { return this->is_quantise(); }
            ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);  // allow quantise to be saved at scene or project level, since it's more of a performance setting than a preference setting

            register_setting(new LSaveableSetting<bool>(
                    "play_chords", 
                    "ChordPlayer",
                    &this->play_chords,
                    [=](bool v) -> void { this->set_play_chords(v); },
                    [=]() -> bool { return this->is_play_chords(); }
            ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);  // allow play_chords to be saved at scene or project level, since it's more of a performance setting than a preference setting

            register_setting(new LSaveableSetting<scale_index_t>(
                    "scale", 
                    "ChordPlayer",
                    &this->scale,
                    [=](scale_index_t v) -> void { this->set_scale(v); },
                    [=]() -> scale_index_t { return this->get_scale(); }
            ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);  // allow scale to be saved at scene or project level, since it's more of a performance setting than a preference setting

            register_setting(new LSaveableSetting<int8_t>(
                    "scale_root", 
                    "ChordPlayer",
                    &this->scale_root,
                    [=](int8_t v) -> void { this->set_scale_root(v); },
                    [=]() -> int8_t { return this->get_scale_root(); }
            ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);  // allow scale_root to be saved at scene or project level, since it's more of a performance setting than a preference setting

            register_setting(new LSaveableSetting<int8_t>(
                    "inversion", 
                    "ChordPlayer",
                    &this->inversion,
                    [=](int8_t v) -> void { this->set_inversion(v); },
                    [=]() -> int8_t { return this->get_inversion(); } 
            ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);  // allow inversion to be saved at scene or project level, since it's more of a performance setting than a preference setting

            register_setting(new LSaveableSetting<int>(
                    "selected_chord_number", 
                    "ChordPlayer",
                    nullptr,
                    [=](int v) -> void { this->set_selected_chord((CHORD::Type)v); },
                    [=]() -> int { return (int)this->get_selected_chord(); }
            ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);  // allow selected_chord_number to be saved at scene or project level, since it's more of a performance setting than a preference setting

            #ifdef CVINPUT_CONFIGURABLE_CHANNEL
                register_setting(new LSaveableSetting<uint8_t>(
                        "channel", 
                        "ChordPlayer",
                        &this->channel,
                        [=](uint8_t v) -> void { this->set_channel(v); },
                        [=]() -> uint8_t { return this->get_channel(); }
                ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);  // allow channel to be saved at scene or project level, since it's more of a performance setting than a preference setting
            #endif
        }

    #endif

};

#endif