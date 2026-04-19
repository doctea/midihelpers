#pragma once

#include <Arduino.h>

#include "parameter_inputs/ParameterInput.h"
#include "pitch_trigger_source.h"

class CVPitchTriggerCore {
public:
    using note_callback_def = PitchTriggerSource::note_callback_def;

private:
    BaseParameterInput *pitch_input = nullptr;
    BaseParameterInput *velocity_input = nullptr;

    bool debug = false;

    PitchTriggerSource pitch_trigger;

public:
    CVPitchTriggerCore(
        note_callback_def on_note_on = [](int8_t note, int8_t velocity) -> void {},
        note_callback_def on_note_off_for_length = [](int8_t note, int8_t velocity) -> void {},
        note_callback_def on_note_off_for_change = [](int8_t note, int8_t velocity) -> void {}
    )
        : pitch_trigger(on_note_on, on_note_off_for_length, on_note_off_for_change) {}

    void set_debug(bool value) {
        this->debug = value;
        this->pitch_trigger.debug = value;
    }

    bool is_debug() const {
        return this->debug;
    }

    void set_parameter_input_pitch(BaseParameterInput *parameter_input) {
        this->pitch_input = parameter_input;
    }

    BaseParameterInput *get_parameter_input_pitch() const {
        return this->pitch_input;
    }

    void set_parameter_input_velocity(BaseParameterInput *parameter_input) {
        this->velocity_input = parameter_input;
    }

    BaseParameterInput *get_parameter_input_velocity() const {
        return this->velocity_input;
    }

    int8_t read_note() const {
        if (this->pitch_input != nullptr && this->pitch_input->supports_pitch()) {
            return this->pitch_input->get_voltage_pitch();
        }
        return NOTE_OFF;
    }

    int read_velocity() const {
        if (this->velocity_input != nullptr) {
            return constrain(
                ((float)MIDI_MAX_VELOCITY) * (float)this->velocity_input->get_normal_value_unipolar(),
                0,
                MIDI_MAX_VELOCITY
            );
        }
        return MIDI_MAX_VELOCITY;
    }

    void on_pre_clock(unsigned long ticks) {
        int8_t new_note = this->read_note();
        int velocity = this->read_velocity();

        if (this->debug) {
            Serial.printf("CVPitchTriggerCore::on_pre_clock(tick=%lu, note=%i, velocity=%i)\n", ticks, new_note, velocity);
        }

        this->pitch_trigger.on_pre_clock(ticks, new_note, velocity);
    }

    void stop_all(uint8_t velocity = MIDI_MIN_VELOCITY) {
        this->pitch_trigger.stop_all(velocity);
    }

    PitchTriggerSource *get_pitch_trigger() {
        return &this->pitch_trigger;
    }

    const PitchTriggerSource *get_pitch_trigger() const {
        return &this->pitch_trigger;
    }
};