#include "menuitems.h"

#include "midi_helpers.h"
#include "scales.h"

class ScaleMenuItem : public MenuItem {
    public:

        //SCALE *scale_number = nullptr; //SCALE::MAJOR;
        SCALE scale_number = SCALE::MAJOR;
        int8_t root_note = SCALE_ROOT_A;

        ScaleMenuItem(const char *label) : MenuItem(label) {}

        virtual SCALE getScaleNumber() {
            return this->scale_number;
        }
        virtual int getRootNote() {
            return this->root_note;
        }


        virtual int display(Coord pos, bool selected, bool opened) override {
            SCALE scale_number = this->getScaleNumber();
            int8_t root_note = this->getRootNote();

            char label[MENU_C_MAX];
            snprintf(label, MENU_C_MAX, 
                "%s: %-3i => %3s %s", this->label,  //  [%i]
                root_note, 
                (char*)get_note_name_c(root_note), 
                scales[scale_number].label
                //, scale_number
            );
            pos.y = header(label, pos, selected, opened);
            //tft->printf("Root note %-3i => %3s\n", (int)root_note, (char*)get_note_name_c(root_note));
            for (int i = 0 ; i < 24 ; i++) {
                int note = root_note + i;
                if (i==12)
                    tft->setCursor(tft->width()/2, pos.y);
                if (i>=12) 
                    tft->setCursor(tft->width()/2, tft->getCursorY());

                byte quantised_note = quantise_pitch(note, root_note, scale_number);

                if (quantised_note != note) {
                    colours(false, BLUE);
                } else {
                    colours(false, GREEN);
                }
                //tft->printf("%s => %s\n", get_note_name_c(i), get_note_name_c(quantised_note));
                tft->printf("%-3i: ", i);
                tft->printf("%-3s", (char*)get_note_name_c(note));
                tft->print(" => ");
                tft->printf("%-3s", (char*)get_note_name_c(quantised_note));
                //if (quantised_note!=i) tft->print(" - quantised!");
                tft->println();
            }
            return tft->getCursorY();
        }

        int mode = 0;

        virtual bool button_select () {
            mode = !mode;
            if (mode) {
                menu_set_last_message("Toggled to SCALE", YELLOW);
            } else {
                menu_set_last_message("Toggled to ROOT", YELLOW);
            }
            return go_back_on_select;
        }

        virtual bool knob_left() override {
            if (mode==0) {
                root_note--;
                if (root_note < 0) root_note = 12;
            } else {
                (scale_number)--;
                //if (scale_number<0 || scale_number>=NUMBER_SCALES)
                //    scale_number = NUMBER_SCALES-1;
            }
            return true;
        }
        virtual bool knob_right() override {
            if (mode==0) {
                root_note++;
                root_note %= 12;
            } else {
                (scale_number)++;
                //if (scale_number >= NUMBER_SCALES)
                //    scale_number = 0;
            }
            return true;
        }
};

template<class TargetClass>
class ObjectScaleMenuItem : public ScaleMenuItem {
    public:

    TargetClass *target = nullptr;
    void(TargetClass::*scale_setter_func)(SCALE) = nullptr;
    SCALE(TargetClass::*scale_getter_func)(void) = nullptr;
    void(TargetClass::*scale_root_setter_func)(int) = nullptr;
    int(TargetClass::*scale_root_getter_func)(void) = nullptr;

    ObjectScaleMenuItem(const char *label, 
        TargetClass *target,
        void(TargetClass::*scale_setter_func)(SCALE), 
        SCALE(TargetClass::*scale_getter_func)(void),
        void(TargetClass::*scale_root_setter_func)(int),
        int(TargetClass::*scale_root_getter_func)(void)) : ScaleMenuItem(label),
            target(target),
            scale_setter_func(scale_setter_func),
            scale_getter_func(scale_getter_func),
            scale_root_setter_func(scale_root_setter_func),
            scale_root_getter_func(scale_root_getter_func)        
        {}

    virtual bool knob_left() override {
        if (mode==0) {
            int root_note = (target->*scale_root_getter_func)()-1;
            if (root_note < 0) root_note = 12;
            (target->*scale_root_setter_func)(root_note);
        } else {
            //(scale_number)--;
            SCALE scale_number = (target->*scale_getter_func)();
            scale_number--;
            (target->*scale_setter_func)(scale_number);
        }
        return true;
    }
    virtual bool knob_right() override {
        if (mode==0) {
            int root_note = (target->*scale_root_getter_func)()+1;
            root_note %= 12;
            (target->*scale_root_setter_func)(root_note);
        } else {
            //(scale_number)++;
            SCALE scale_number = (target->*scale_getter_func)();
            scale_number++;
            (target->*scale_setter_func)(scale_number);
        }
        return true;
    }

    virtual SCALE getScaleNumber() override {
        return (this->target->*scale_getter_func)();
    }
    virtual int getRootNote() override {
        return (this->target->*scale_root_getter_func)();
    }


};