#ifdef ENABLE_SCREEN

#ifndef MENU_CLOCK_SELECTOR__INCLUDED
#define MENU_CLOCK_SELECTOR__INCLUDED

//#include "Config.h"
#include "../clock.h"

#include "menuitems.h"

class ClockSourceSelectorControl : public SelectorControl<int_least8_t> {
    int_least16_t actual_value_index;
    //void (*setter_func)(MIDIOutputWrapper *midi_output);
    //MIDIOutputWrapper *initial_selected_output_wrapper;

    public:

    ClockSourceSelectorControl(const char *label, int_least8_t initial_value) 
        : SelectorControl(label, initial_value), actual_value_index(initial_value) {};

    virtual const char* get_label_for_index(int_least8_t index) {
        if (index==CLOCK_INTERNAL)
            return "Internal";
        else if (index==CLOCK_EXTERNAL_USB_HOST)
            return "External USB-MIDI";
        #ifdef ENABLE_CLOCK_INPUT_MIDI_DIN
        else if (index==CLOCK_EXTERNAL_MIDI_DIN)
            return "External DIN-MIDI";
        #endif
        #ifdef ENABLE_CLOCK_INPUT_CV
        else if (index==CLOCK_EXTERNAL_CV)
            return "External CV";
        #endif
        else if (index==CLOCK_NONE)
            return "None";
        return "??";
    }

    virtual void setter (int_least8_t new_value) {
        #ifdef USE_UCLOCK
            if (clock_mode==CLOCK_INTERNAL && new_value!=clock_mode) {
                uClock.stop();
            } else if (new_value==CLOCK_INTERNAL && playing) {
                uClock.start();
            }
        #endif
        change_clock_mode((ClockMode) new_value);
        actual_value_index = clock_mode;
        //selected_value_index = clock_mode;
    }
    virtual int_least8_t getter () {
        return clock_mode; //selected_value_index;
    }

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println("MidiOutputSelectorControl display()!");

        pos.y = header(label, pos, selected, opened);
        tft->setTextSize(2);

        num_values = NUM_CLOCK_SOURCES;

        tft->setTextSize(2);

        if (!opened) {
            // not opened, so just show the current value
            //colours(opened && selected_value_index==i, col, BLACK);

            tft->printf((char*)"%s", (char*)get_label_for_index(getter())); //selected_value_index));
            tft->println((char*)"");
        } else {
            //int current_value = actual_value_index;
            
            for (int i = 0 ; i < num_values ; i++) {
                bool is_current_value_selected = ((int)i)==getter();
                int col = is_current_value_selected ? GREEN : C_WHITE;
                colours(opened && selected_value_index==(int)i, col, BLACK);
                tft->printf((char*)"%s\n", (char*)get_label_for_index(i));
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println((char*)"");
        }
        return tft->getCursorY() + 2;
    }

    virtual bool button_select() {
        this->setter(selected_value_index);

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, "Set clock to %i: %s", selected_value_index, get_label_for_index(selected_value_index));
        //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg, GREEN);

        return go_back_on_select;
    }

};


#ifdef USE_UCLOCK

#include <uClock.h>

const int available_ppqn_values[] = {
    umodular::clock::uClockClass::PPQN_1,
    umodular::clock::uClockClass::PPQN_2,
    umodular::clock::uClockClass::PPQN_4,
    umodular::clock::uClockClass::PPQN_8,
    umodular::clock::uClockClass::PPQN_12,
    umodular::clock::uClockClass::PPQN_24,
    umodular::clock::uClockClass::PPQN_48,
    umodular::clock::uClockClass::PPQN_96,
    umodular::clock::uClockClass::PPQN_384,
    umodular::clock::uClockClass::PPQN_480,
    umodular::clock::uClockClass::PPQN_960
};
class ExternalPPQNSelectorControl : public SelectorControl<int> {
    public:
    ExternalPPQNSelectorControl(const char *label, int initial_value) 
        : SelectorControl(label, initial_value) {
            this->available_values = available_ppqn_values;
            this->num_values = 11;
        };
    virtual const char* get_label_for_index(int index) {
        switch(index) {
            case umodular::clock::uClockClass::PPQN_1:   return "1 PPQN";
            case umodular::clock::uClockClass::PPQN_2:   return "2 PPQN";
            case umodular::clock::uClockClass::PPQN_4:   return "4 PPQN";
            case umodular::clock::uClockClass::PPQN_8:   return "8 PPQN";
            case umodular::clock::uClockClass::PPQN_12:  return "12 PPQN";
            case umodular::clock::uClockClass::PPQN_24:  return "24 PPQN";
            case umodular::clock::uClockClass::PPQN_48:  return "48 PPQN";
            case umodular::clock::uClockClass::PPQN_96:  return "96 PPQN";
            case umodular::clock::uClockClass::PPQN_384: return "384 PPQN";
            case umodular::clock::uClockClass::PPQN_480: return "480 PPQN";
            case umodular::clock::uClockClass::PPQN_960: return "960 PPQN";
            default: return "??";
        }
    }
    virtual void setter (int new_value) {
        Serial.printf("ExternalPPQNSelectorControl::setter(%i)\n", new_value);
        #ifdef ENABLE_CLOCK_INPUT_CV
        if (clock_mode==CLOCK_EXTERNAL_CV) {
            external_cv_ppqn = (umodular::clock::uClockClass::PPQNResolution) new_value;
            messages_log_add("Set external CV PPQN");
            uClock.setInputPPQN(external_cv_ppqn);
        } else 
        #endif
        if (clock_mode==CLOCK_INTERNAL) {
            internal_ppqn = (umodular::clock::uClockClass::PPQNResolution) new_value;
            messages_log_add("Set internal PPQN");
            uClock.setInputPPQN(internal_ppqn);
        } else {
            messages_log_add("Cannot set PPQN unless clock mode is internal or external CV");
        }
    }
    virtual int getter () {
        #ifdef ENABLE_CLOCK_INPUT_CV
        if (clock_mode==CLOCK_EXTERNAL_CV) {
            return external_cv_ppqn;
        } else 
        #endif
        if (clock_mode==CLOCK_INTERNAL) {
            return internal_ppqn;
        }
        return DEFAULT_CV_PPQN;
    }
};
#endif // USE_UCLOCK

#endif
#endif