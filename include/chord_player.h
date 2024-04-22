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

#include <Arduino.h>

#include "bpm.h"
#include "scales.h"

#include "functional-vlpp.h"

#ifdef ENABLE_SCREEN
    #include "mymenu/menuitems_scale.h"
    #include "mymenu/menuitems_harmony.h"
#endif
class ChordPlayer {
    public:

        using setter_func_def = vl::Func<void(int8_t channel, int8_t note, int8_t velocity)>;
        setter_func_def receive_note_on, receive_note_off;

        ChordPlayer(setter_func_def receive_note_on, setter_func_def receive_note_off) {
            this->receive_note_on = receive_note_on;
            this->receive_note_off = receive_note_off;
        }
        bool debug = false;

        bool is_playing = false;
        bool is_playing_chord = false;
        int last_note = -1, current_note = -1, current_raw_note = -1;
        CHORD::Type last_chord = CHORD::NONE, current_chord = CHORD::NONE, selected_chord_number = CHORD::NONE;
        unsigned long note_started_at_tick = 0;
        int32_t note_length_ticks = PPQN;
        int32_t trigger_on_ticks = 0;   // 0 = on change
        int32_t trigger_delay_ticks = 0;
        int8_t inversion = 0;

        chord_instance_t current_chord_data;
        chord_instance_t last_chord_data;

        #ifdef DEBUG_VELOCITY
            int8_t velocity = MIDI_MAX_VELOCITY;
        #endif

        bool quantise = false, play_chords = false;
        SCALE scale = SCALE::MAJOR;
        int8_t scale_root = SCALE_ROOT_C;
 
        uint8_t channel = 0;
        #ifdef CVINPUT_CONFIGURABLE_CHANNEL
            uint8_t get_channel() { return channel; }
            void set_channel(uint8_t channel) { this->channel = channel; }
        #endif

        void set_scale(SCALE scale) {
            //trigger_off_for_pitch_because_changed(this->current_note);
            this->scale = scale;
        }
        SCALE get_scale() {
            return this->scale;
        }
        void set_scale_root(int8_t scale_root) {
            //trigger_off_for_pitch_because_changed(this->current_note);
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
            //trigger_off_for_pitch_because_changed(this->current_note);
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

        virtual void set_note_length(int32_t length_ticks) {
            this->note_length_ticks = length_ticks;
        }
        virtual int32_t get_note_length () {
            return this->note_length_ticks;
        }
        virtual void set_trigger_on_ticks(int32_t length_ticks) {
            this->trigger_on_ticks = length_ticks;
        }
        virtual int32_t get_trigger_on_ticks () {
            return this->trigger_on_ticks;
        }
        virtual void set_trigger_delay_ticks(int32_t delay_ticks) {
            this->trigger_delay_ticks = delay_ticks;
        }
        virtual int32_t get_trigger_delay_ticks () {
            return this->trigger_delay_ticks;
        }


        void stop_chord(chord_instance_t chord) {
            this->stop_chord(chord.chord_root, chord.chord_type, chord.inversion, chord.velocity);
        }

        void stop_chord(int8_t pitch, CHORD::Type chord_number = CHORD::TRIAD, int8_t inversion = 0, uint8_t velocity = 0) {
            if (debug) Serial.printf("\t---\nstop_chord: Stopping chord for %i (%s) - chord type %s, inversion %i\n", pitch, get_note_name_c(pitch), chords[chord_number].label, inversion);

            //int8_t n = -1;
            //for (size_t i = 0 ; (n = quantise_pitch_chord_note(pitch, chord_number, i, this->scale_root, this->scale, this->current_chord_data->inversion, this->debug)) >= 0 ; i++) {
            for (size_t i = 0 ; i < PITCHES_PER_CHORD /*&& ((n = this->current_chord_data.pitches[i]) >= 0)*/ ; i++) {
                int8_t n = this->current_chord_data.pitches[i];
                if (debug) Serial.printf("\t\tStopping note\t[%i/%i]: %i\t(%s)\n", i, PITCHES_PER_CHORD, n, get_note_name_c(n));
                if (is_valid_note(n))
                    receive_note_off(channel, n, velocity);
            }
            
            last_chord = this->current_chord;
            this->last_chord_data = current_chord_data;
            this->is_playing = false;
            this->is_playing_chord = false;
            this->current_chord_data.clear();
            if (debug) Serial.println("---");
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

            int8_t previously_played_note = -1; // avoid duplicating notes, like what happens sometimes when playing inverted +octaved chords..!
            for (size_t i = 0 ; i < PITCHES_PER_CHORD && ((n = quantise_pitch_chord_note(pitch, chord_number, i, this->get_scale_root(), this->get_scale(), inversion, this->debug)) >= 0) ; i++) {
                this->current_chord_data.set_pitch(i, n);
                if (debug) Serial.printf("\t\tPlaying note\t[%i/%i]: %i\t(%s)\n", i, PITCHES_PER_CHORD, n, get_note_name_c(n));
                if (n!=previously_played_note) {
                    receive_note_on(channel, n, velocity);
                } else {
                    if (debug) Serial.printf("\t\tSkipping note\t[%i/%i]: %i\t(%s)\n", i, PITCHES_PER_CHORD, n, get_note_name_c(n));
                }
                previously_played_note = n;
            }
            if (debug) Serial.println("---");
        }

        virtual void trigger_off_for_pitch_because_length(int8_t pitch, uint8_t velocity = MIDI_MIN_VELOCITY) {
            // don't reset current_note so that we don't retrigger the same note again immediately
            if (is_playing_chord) //is_quantise()) 
                this->stop_chord(this->current_chord_data);
            else
                this->receive_note_off(channel, this->current_note, 0);

            is_playing = false;
            this->last_note = pitch;
            this->current_note = 255;
        }
        virtual void trigger_off_for_pitch_because_changed(int8_t pitch, uint8_t velocity = MIDI_MIN_VELOCITY) {
            if (is_playing_chord) //is_quantise())
                this->stop_chord(this->current_chord_data);
            else
                this->receive_note_off(channel, this->current_note, 0);

            this->is_playing = false;
            this->last_note = this->current_note;
            this->current_note = 255;
        }
        virtual void trigger_on_for_pitch(int8_t pitch, uint8_t velocity = MIDI_MAX_VELOCITY, CHORD::Type chord_number = CHORD::TRIAD, int8_t inversion = 0) {
            if (this->is_playing)
                this->stop_chord(this->current_chord_data);

            this->current_note = pitch;
            this->note_started_at_tick = ticks;
            #ifdef DEBUG_VELOCITY
                this->velocity = velocity;
            #endif
            if (!is_quantise() || !is_play_chords() || this->selected_chord_number==CHORD::NONE)
                this->receive_note_on(channel, this->current_note, velocity);
            else
                this->play_chord(pitch, chord_number, inversion, velocity);
            this->is_playing = true;
        }

        //void on_tick(unsigned long ticks) override {
        // if we send this during tick then the notes never get received, for some reason.  sending during on_pre_clock seems to work ok for now.
        // TODO: see if this is solved now and we can revert back to using on_tick, now that we have updated to newer version of USBHost_t36 library?
        void on_pre_clock(unsigned long ticks, int8_t new_note, int8_t velocity) {

            //if (!(get_trigger_on_ticks()==0 || (ticks-trigger_delay_ticks) % get_trigger_on_ticks()==0))
            //    return;
            
            if (this->debug) Serial.printf("---- on_pre_clock(%i, %i, %i)\n", ticks, new_note, velocity);
            // check if playing note duration has passed regardless of whether pitch_input is set, so that notes will still finish even if disconncted
            if (is_playing && this->get_note_length()>0 && abs((long)this->note_started_at_tick-(long)ticks) >= this->get_note_length()) {
                if (this->debug) Serial.printf("CVInput: Stopping note\t%i because playing and elapsed is (%u-%u=%u)\n", current_note, note_started_at_tick, ticks, abs((long)this->note_started_at_tick-(long)ticks));
                trigger_off_for_pitch_because_length(current_note);
                //this->current_note = -1; // dont clear current_note, so that we don't retrigger it again
            }

            this->current_raw_note = new_note;
            if (this->is_quantise())
                new_note = quantise_pitch(new_note, this->scale_root, this->scale);

            // has pitch become invalid?  is so and if note playing, stop note
            if (is_playing && !is_valid_note(new_note) && is_valid_note(this->current_note)) {
                if (this->debug) Serial.printf("CVInput: Stopping note\t%i because playing and new_note isn't valid\n", new_note);
                trigger_off_for_pitch_because_changed(this->current_note);
            } else if (is_valid_note(new_note) && (new_note!=this->current_note || this->get_trigger_on_ticks()>0)) {
                // note has changed from valid to a different valid
                if (is_playing && this->get_trigger_on_ticks()==0) {
                    if (this->debug) Serial.printf("CVInput: Stopping note\t%i because of new_note\t%i\n", this->current_note, new_note);
                    trigger_off_for_pitch_because_changed(this->current_note);
                }
                if (this->get_note_length()>0) { // && (get_trigger_on_ticks()==0 || (ticks-trigger_delay_ticks) % get_trigger_on_ticks()==0)) {
                    if (this->debug) Serial.printf("CVInput: Starting note %i\tat\t%u\n", new_note, ticks);
                    if (!(get_trigger_on_ticks()==0 || (ticks-trigger_delay_ticks) % get_trigger_on_ticks()==0))
                        return;

                    trigger_on_for_pitch(new_note, velocity, selected_chord_number, this->inversion);
                }
            }
            if (this->debug) Serial.println("----");
        }

    #ifdef ENABLE_SCREEN
        LinkedList<MenuItem *> *make_menu_items(LinkedList<MenuItem *> *menuitems = nullptr) {
            if (menuitems==nullptr)
                menuitems = new LinkedList<MenuItem *>();

            // todo: move the below to ChordPlayer, and call it in here
            #ifdef DEBUG_VELOCITY
                DirectNumberControl<int8_t> *velocity_control = new DirectNumberControl<int8_t>("Velocity", &this->velocity, 127, 0, 127);
                menuitems->add(velocity_control);
            #endif

            //Serial.println(F("DeviceBehaviour_CVInput::make_menu_items() setting up HarmonyStatus")); Serial_flush();
            HarmonyStatus *harmony = new HarmonyStatus(
                "CV->MIDI pitch", 
                &this->last_note, 
                &this->current_note,
                &this->current_raw_note,
                "Raw"
            );
            menuitems->add(harmony);

            SubMenuItemBar *bar = new SubMenuItemBar("Trigger/durations");
            //Serial.println(F("about to create length_ticks_control ObjectSelectorControl..")); Serial_flush();
            LambdaSelectorControl<int32_t> *length_ticks_control = new LambdaSelectorControl<int32_t>(
                    "Note length",
                    [=](int32_t v) -> void { this->set_note_length(v); },
                    [=]() -> int32_t { return this->get_note_length(); },
                    nullptr,
                    true
            );
            //Serial.println(F("about to add values..")); Serial_flush();
            length_ticks_control->add_available_value(0,                 "None");
            length_ticks_control->add_available_value(PPQN/PPQN,         "-");
            length_ticks_control->add_available_value(PPQN/8,            "32nd");
            length_ticks_control->add_available_value(PPQN/4,            "16th");
            length_ticks_control->add_available_value(PPQN/3,            "12th");
            length_ticks_control->add_available_value(PPQN/2,            "8th");
            length_ticks_control->add_available_value(PPQN,              "Beat");
            length_ticks_control->add_available_value(PPQN*2,            "2xBeat");
            length_ticks_control->add_available_value(PPQN*4,            "Bar");
            //Serial.println(F("about to add to menuitems list..")); Serial_flush();
            bar->add(length_ticks_control);

            //Serial.println(F("about to create length_ticks_control ObjectSelectorControl..")); Serial_flush();
            LambdaSelectorControl<int32_t> *trigger_ticks_control = new LambdaSelectorControl<int32_t>(
                    "Trigger each",
                    [=](int32_t v) -> void { this->set_trigger_on_ticks(v); },
                    [=]() -> int32_t { return this->get_trigger_on_ticks(); },
                    nullptr,
                    true
            );
            trigger_ticks_control->add_available_value(0,                 "Change");
            //trigger_ticks_control->add_available_value(PPQN/PPQN,         "-");
            trigger_ticks_control->add_available_value(PPQN/8,            "32nd");
            trigger_ticks_control->add_available_value(PPQN/4,            "16th");
            trigger_ticks_control->add_available_value(PPQN/3,            "12th");
            trigger_ticks_control->add_available_value(PPQN/2,            "8th");
            trigger_ticks_control->add_available_value(PPQN,              "Beat");
            trigger_ticks_control->add_available_value(PPQN*2,            "2xBeat");
            trigger_ticks_control->add_available_value(PPQN*4,            "Bar");
            //Serial.println(F("about to add to menuitems list..")); Serial_flush();
            bar->add(trigger_ticks_control);

            LambdaSelectorControl<int32_t> *trigger_delay_ticks_control 
                = new LambdaSelectorControl<int32_t>(
                    "Delay",
                    [=](int32_t v) -> void { this->set_trigger_delay_ticks(v); },
                    [=]() -> int32_t { return this->get_trigger_delay_ticks(); },
                    nullptr,
                    true
            );
            //trigger_ticks_control->add_available_value(0,                 "Change");
            //trigger_ticks_control->add_available_value(PPQN/PPQN,         "-");
            trigger_delay_ticks_control->add_available_value(0,                 "None");
            trigger_delay_ticks_control->add_available_value(PPQN/8,            "32nd");
            trigger_delay_ticks_control->add_available_value(PPQN/4,            "16th");
            trigger_delay_ticks_control->add_available_value(PPQN/3,            "12th");
            trigger_delay_ticks_control->add_available_value(PPQN/2,            "8th");
            trigger_delay_ticks_control->add_available_value(PPQN,              "Beat");
            trigger_delay_ticks_control->add_available_value(PPQN*2,            "2xBeat");
            //trigger_ticks_control->add_available_value(PPQN*4,            "Bar");
            bar->add(trigger_delay_ticks_control);

            menuitems->add(bar);

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
            // TODO: allow all pitched behaviours to use a 'global scale' setting (-1?)
            menuitems->add(new LambdaScaleMenuItemBar(
                "Scale / Key", 
                [=](SCALE scale) -> void { this->set_scale(scale); }, 
                [=]() -> SCALE { return this->get_scale(); },
                [=](int8_t scale_root) -> void { this->set_scale_root(scale_root); },
                [=]() -> int8_t { return this->get_scale_root(); },
                true
            ));

            bar = new SubMenuItemBar("Quantise / chords");
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
            for (size_t i = 0 ; i < NUMBER_CHORDS ; i++) {
                selected_chord_control->add_available_value(i, chords[i].label);
            }

            bar->add(new LambdaNumberControl<int8_t>(
                "Inversion", 
                [=](int8_t v) -> void { this->set_inversion(v); }, 
                [=]() -> int8_t { return this->get_inversion(); },
                nullptr, 0, 4, true
            ));
            bar->add(selected_chord_control);

            menuitems->add(bar);

            menuitems->add(new ChordMenuItem("Current chord",   &this->current_chord_data));
            menuitems->add(new ChordMenuItem("Last chord",      &this->last_chord_data));

            menuitems->add(new ToggleControl<bool>("Debug", &this->debug));

            return menuitems;
        }

    #endif
};