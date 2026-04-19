#pragma once

#include <Arduino.h>

#include "bpm.h"
#include "midi_helpers.h"
#include "functional-vlpp.h"

#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
#endif

#ifdef ENABLE_SCREEN
    #include "mymenu/menuitems_harmony.h"
    #include "submenuitem_bar.h"
    #include "menuitems_lambda_selector.h"
    #include "menuitems_toggle.h"
#endif

class PitchTriggerSource
#ifdef ENABLE_STORAGE
    : public SHDynamic<0, 3>
#endif
{
public:
    using note_callback_def = vl::Func<void(int8_t note, int8_t velocity)>;

    note_callback_def on_note_on;
    note_callback_def on_note_off_for_length;
    note_callback_def on_note_off_for_change;

    bool debug = false;

    bool is_playing = false;
    int8_t last_note = NOTE_OFF;
    int8_t current_note = NOTE_OFF;
    int8_t current_raw_note = NOTE_OFF;
    unsigned long note_started_at_tick = 0;

    int32_t note_length_ticks = PPQN;
    int32_t trigger_on_ticks = 0; // 0 = on change
    int32_t trigger_delay_ticks = 0;

    PitchTriggerSource(
        note_callback_def on_note_on = [](int8_t note, int8_t velocity) -> void {},
        note_callback_def on_note_off_for_length = [](int8_t note, int8_t velocity) -> void {},
        note_callback_def on_note_off_for_change = [](int8_t note, int8_t velocity) -> void {}
    ) {
        this->on_note_on = on_note_on;
        this->on_note_off_for_length = on_note_off_for_length;
        this->on_note_off_for_change = on_note_off_for_change;

        #ifdef ENABLE_STORAGE
            this->set_path_segment("PitchTrigger");
        #endif
    }

    virtual void set_note_length(int32_t length_ticks) {
        this->note_length_ticks = length_ticks;
    }
    virtual int32_t get_note_length() {
        return this->note_length_ticks;
    }

    virtual void set_trigger_on_ticks(int32_t length_ticks) {
        this->trigger_on_ticks = length_ticks;
    }
    virtual int32_t get_trigger_on_ticks() {
        return this->trigger_on_ticks;
    }

    virtual void set_trigger_delay_ticks(int32_t delay_ticks) {
        this->trigger_delay_ticks = delay_ticks;
    }
    virtual int32_t get_trigger_delay_ticks() {
        return this->trigger_delay_ticks;
    }

    virtual void trigger_off_for_pitch_because_length(int8_t pitch, uint8_t velocity = MIDI_MIN_VELOCITY) {
        if (is_valid_note(this->current_note)) {
            this->on_note_off_for_length(this->current_note, velocity);
        }
        this->is_playing = false;
        this->last_note = pitch;
        this->current_note = NOTE_OFF;
    }

    virtual void trigger_off_for_pitch_because_changed(int8_t pitch, uint8_t velocity = MIDI_MIN_VELOCITY) {
        if (is_valid_note(this->current_note)) {
            this->on_note_off_for_change(this->current_note, velocity);
        }
        this->is_playing = false;
        this->last_note = this->current_note;
        this->current_note = NOTE_OFF;
    }

    virtual void trigger_on_for_pitch(unsigned long tick, int8_t pitch, uint8_t velocity = MIDI_MAX_VELOCITY) {
        if (!is_valid_note(pitch)) {
            return;
        }

        this->current_note = pitch;
        this->note_started_at_tick = tick;
        this->on_note_on(this->current_note, velocity);
        this->is_playing = true;
    }

    virtual void stop_all(uint8_t velocity = MIDI_MIN_VELOCITY) {
        if (this->is_playing && is_valid_note(this->current_note)) {
            this->on_note_off_for_change(this->current_note, velocity);
        }

        this->is_playing = false;
        this->last_note = this->current_note;
        this->current_note = NOTE_OFF;
    }

    virtual void on_pre_clock(unsigned long tick, int8_t monitored_note, int8_t velocity, int8_t raw_note) {
        if (this->debug) {
            Serial.printf("---- PitchTriggerSource::on_pre_clock(%i, %i, %i, %i)\n", tick, monitored_note, velocity, raw_note);
        }

        if (this->is_playing && this->get_note_length() > 0 && abs((long)this->note_started_at_tick - (long)tick) >= this->get_note_length()) {
            if (this->debug) {
                Serial.printf("PitchTriggerSource: Stopping note %i because elapsed is (%u-%u=%u)\n", this->current_note, this->note_started_at_tick, tick, abs((long)this->note_started_at_tick - (long)tick));
            }
            this->trigger_off_for_pitch_because_length(this->current_note);
        }

        this->current_raw_note = raw_note;

        if (this->is_playing && !is_valid_note(monitored_note) && is_valid_note(this->current_note)) {
            if (this->debug) {
                Serial.printf("PitchTriggerSource: Stopping note %i because monitored note is invalid\n", this->current_note);
            }
            this->trigger_off_for_pitch_because_changed(this->current_note);
            return;
        }

        if (!is_valid_note(monitored_note) || (monitored_note == this->current_note && this->get_trigger_on_ticks() == 0)) {
            if (this->debug) {
                Serial.println("----");
            }
            return;
        }

        if (this->is_playing && this->get_trigger_on_ticks() == 0) {
            this->trigger_off_for_pitch_because_changed(this->current_note);
        }

        if (this->get_trigger_on_ticks() == 0 && (monitored_note == this->current_note || monitored_note == this->last_note)) {
            if (this->debug) {
                Serial.printf("PitchTriggerSource: Not retriggering %i because it has not changed\n", monitored_note);
            }
            if (this->debug) {
                Serial.println("----");
            }
            return;
        }

        if (!(this->get_trigger_on_ticks() == 0 || (tick - this->trigger_delay_ticks) % this->get_trigger_on_ticks() == 0)) {
            if (this->debug) {
                Serial.println("----");
            }
            return;
        }

        this->trigger_on_for_pitch(tick, monitored_note, velocity);

        if (this->debug) {
            Serial.println("----");
        }
    }

    virtual void on_pre_clock(unsigned long tick, int8_t monitored_note, int8_t velocity) {
        this->on_pre_clock(tick, monitored_note, velocity, monitored_note);
    }

#ifdef ENABLE_SCREEN
    #if __cplusplus >= 201703L
        inline static LinkedList<LambdaSelectorControl<int32_t>::option> *length_ticks_control_options;
        inline static LinkedList<LambdaSelectorControl<int32_t>::option> *trigger_ticks_control_options;
        inline static LinkedList<LambdaSelectorControl<int32_t>::option> *trigger_delay_ticks_control_options;
    #else
        static LinkedList<LambdaSelectorControl<int32_t>::option> *length_ticks_control_options;
        static LinkedList<LambdaSelectorControl<int32_t>::option> *trigger_ticks_control_options;
        static LinkedList<LambdaSelectorControl<int32_t>::option> *trigger_delay_ticks_control_options;
    #endif

    FLASHMEM
    LinkedList<MenuItem *> *make_menu_items(LinkedList<MenuItem *> *menuitems = nullptr) {
        if (menuitems == nullptr) {
            menuitems = new LinkedList<MenuItem *>();
        }

        HarmonyStatus *harmony = new HarmonyStatus(
            "CV->MIDI pitch",
            &this->last_note,
            &this->current_note,
            &this->current_raw_note,
            "Raw"
        );
        menuitems->add(harmony);

        SubMenuItemBar *bar = new SubMenuItemBar("Trigger/durations");

        LambdaSelectorControl<int32_t> *length_ticks_control = new LambdaSelectorControl<int32_t>(
            "Note length",
            [=](int32_t v) -> void { this->set_note_length(v); },
            [=]() -> int32_t { return this->get_note_length(); },
            nullptr,
            true
        );
        if (length_ticks_control_options == nullptr) {
            length_ticks_control->add_available_value(0, "-");
            length_ticks_control->add_available_value(PPQN / 8, "32nd");
            length_ticks_control->add_available_value(PPQN / 4, "16th");
            length_ticks_control->add_available_value(PPQN / 3, "12th");
            length_ticks_control->add_available_value(PPQN / 2, "8th");
            length_ticks_control->add_available_value(PPQN, "Beat");
            length_ticks_control->add_available_value(PPQN * 2, "2xBeat");
            length_ticks_control->add_available_value(PPQN * 4, "Bar");
            length_ticks_control_options = length_ticks_control->get_available_values();
        } else {
            length_ticks_control->set_available_values(length_ticks_control_options);
        }
        bar->add(length_ticks_control);

        LambdaSelectorControl<int32_t> *trigger_ticks_control = new LambdaSelectorControl<int32_t>(
            "Trigger each",
            [=](int32_t v) -> void { this->set_trigger_on_ticks(v); },
            [=]() -> int32_t { return this->get_trigger_on_ticks(); },
            nullptr,
            true
        );
        if (trigger_ticks_control_options == nullptr) {
            trigger_ticks_control->add_available_value(0, "Change");
            trigger_ticks_control->add_available_value(PPQN / 8, "32nd");
            trigger_ticks_control->add_available_value(PPQN / 4, "16th");
            trigger_ticks_control->add_available_value(PPQN / 3, "12th");
            trigger_ticks_control->add_available_value(PPQN / 2, "8th");
            trigger_ticks_control->add_available_value(PPQN, "Beat");
            trigger_ticks_control->add_available_value(PPQN * 2, "2xBeat");
            trigger_ticks_control->add_available_value(PPQN * 4, "Bar");
            trigger_ticks_control_options = trigger_ticks_control->get_available_values();
        } else {
            trigger_ticks_control->set_available_values(trigger_ticks_control_options);
        }
        bar->add(trigger_ticks_control);

        LambdaSelectorControl<int32_t> *trigger_delay_ticks_control = new LambdaSelectorControl<int32_t>(
            "Delay",
            [=](int32_t v) -> void { this->set_trigger_delay_ticks(v); },
            [=]() -> int32_t { return this->get_trigger_delay_ticks(); },
            nullptr,
            true
        );
        if (trigger_delay_ticks_control_options == nullptr) {
            trigger_delay_ticks_control->add_available_value(0, "None");
            trigger_delay_ticks_control->add_available_value(PPQN / 8, "32nd");
            trigger_delay_ticks_control->add_available_value(PPQN / 4, "16th");
            trigger_delay_ticks_control->add_available_value(PPQN / 3, "12th");
            trigger_delay_ticks_control->add_available_value(PPQN / 2, "8th");
            trigger_delay_ticks_control->add_available_value(PPQN, "Beat");
            trigger_delay_ticks_control->add_available_value(PPQN * 2, "2xBeat");
            trigger_delay_ticks_control_options = trigger_delay_ticks_control->get_available_values();
        } else {
            trigger_delay_ticks_control->set_available_values(trigger_delay_ticks_control_options);
        }
        bar->add(trigger_delay_ticks_control);

        menuitems->add(bar);
        menuitems->add(new ToggleControl<bool>("Debug", &this->debug));

        return menuitems;
    }
#endif

#ifdef ENABLE_STORAGE
    virtual void setup_saveable_settings() override {
        ISaveableSettingHost::setup_saveable_settings();

        register_setting(new LSaveableSetting<int32_t>(
            "note_length_ticks",
            "PitchTrigger",
            &this->note_length_ticks,
            [=](int32_t v) -> void { this->set_note_length(v); },
            [=]() -> int32_t { return this->get_note_length(); }
        ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);

        register_setting(new LSaveableSetting<int32_t>(
            "trigger_on_ticks",
            "PitchTrigger",
            &this->trigger_on_ticks,
            [=](int32_t v) -> void { this->set_trigger_on_ticks(v); },
            [=]() -> int32_t { return this->get_trigger_on_ticks(); }
        ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);

        register_setting(new LSaveableSetting<int32_t>(
            "trigger_delay_ticks",
            "PitchTrigger",
            &this->trigger_delay_ticks,
            [=](int32_t v) -> void { this->set_trigger_delay_ticks(v); },
            [=]() -> int32_t { return this->get_trigger_delay_ticks(); }
        ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
    }
#endif
};
