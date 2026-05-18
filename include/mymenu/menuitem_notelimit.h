#pragma once

#include "midi_helpers.h"

#include "menuitems_lambda.h"
#include "menuitems_object_selector.h"
#include "menuitems.h"

template<class DataType = NOTE_LIMIT_MODE>
class NoteLimitModeControl : public LambdaSelectorControl<DataType> {
    public:

    static LinkedList<typename LambdaSelectorControl<DataType>::option> *note_limit_mode_options;

    NoteLimitModeControl(
        const char* label, 
        vl::Func<void(DataType)> setter_func,
        vl::Func<DataType(void)> getter_func,
        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr,
        bool go_back_on_select = false,
        bool direct = false
    ) : LambdaSelectorControl<DataType>(label, setter_func, getter_func, on_change_handler, go_back_on_select, direct) {
        if (note_limit_mode_options==nullptr) {
            this->add_available_value(NOTE_LIMIT_MODE::IGNORE, "Drop");
            this->add_available_value(NOTE_LIMIT_MODE::TRANSPOSE, "Wrap");
            note_limit_mode_options = this->available_values;
        } else {
            this->set_available_values(note_limit_mode_options);
        }
    }

    virtual bool action_opened() override {
        NOTE_LIMIT_MODE value = this->getter_func();
        this->setter_func(
            value == NOTE_LIMIT_MODE::TRANSPOSE ? 
            NOTE_LIMIT_MODE::IGNORE : 
            NOTE_LIMIT_MODE::TRANSPOSE
        );
        return false;   // don't 'open'
    }
};

template<class DataType>
LinkedList<typename LambdaSelectorControl<DataType>::option>* NoteLimitModeControl<DataType>::note_limit_mode_options = nullptr;