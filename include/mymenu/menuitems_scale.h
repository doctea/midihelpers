#include "menuitems.h"

#include "midi_helpers.h"
#include "scales.h"

class ScaleMenuItem : public MenuItem {
    public:

        //SCALE *scale_number = nullptr; //SCALE::MAJOR;
        SCALE scale_number = SCALE::MAJOR;
        int8_t root_note = SCALE_ROOT_A;
        bool full_display = true;

        ScaleMenuItem(const char *label, bool full_display = true) : MenuItem(label), full_display(full_display) {}

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
            if (full_display) {
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
        int(TargetClass::*scale_root_getter_func)(void),
        bool full_display = false
    ) : ScaleMenuItem(label, full_display),
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

#include "submenuitem_bar.h"

template<class TargetClass>
class ObjectScaleMenuItemBar : public SubMenuItemBar {
    public:
    TargetClass *target_object = nullptr;

    // have difficulty using SCALE type with ObjectSelectorControls, so wrap it as int
    void(TargetClass::*scale_setter_func)(SCALE) = nullptr;
    SCALE(TargetClass::*scale_getter_func)(void) = nullptr;

    void set_scale(int scale_number) {
        if (this->target_object!=nullptr && this->scale_setter_func!=nullptr)
            (this->target_object->*scale_setter_func)((SCALE)scale_number);
    }
    int get_scale() {
        if (this->target_object!=nullptr && this->scale_getter_func!=nullptr)
            return(this->target_object->*scale_getter_func)();
        return 0;
    }

    ObjectScaleMenuItemBar(
        const char *label, 
        TargetClass *target_object,
        void(TargetClass::*scale_setter_func)(SCALE),
        SCALE(TargetClass::*scale_getter_func)(void),
        void(TargetClass::*scale_root_setter_func)(int),
        int(TargetClass::*scale_root_getter_func)(void)
    ) : SubMenuItemBar(label) {
        //this->debug = true;

        this->scale_setter_func = scale_setter_func;
        this->scale_getter_func = scale_getter_func;

        this->target_object = target_object;

        // use self as intermediary to real target object in order to wrap int/SCALE type
        ObjectSelectorControl<ObjectScaleMenuItemBar,int> *scale_selector = new ObjectSelectorControl<ObjectScaleMenuItemBar,int>(
            "Scale type", 
            this, 
            &ObjectScaleMenuItemBar::set_scale, 
            &ObjectScaleMenuItemBar::get_scale,
            nullptr,
            true
        );
        for (size_t i = 0 ; i < NUMBER_SCALES ; i++) {
            scale_selector->add_available_value(i, scales[i].label);
        }   
        scale_selector->go_back_on_select = true;
        this->add(scale_selector);

        ObjectSelectorControl<TargetClass,int> *scale_root = new ObjectSelectorControl<TargetClass,int>(
            "Root key", 
            this->target_object, 
            scale_root_setter_func, 
            scale_root_getter_func,
            nullptr,
            true
        );
        for (size_t i = 0 ; i < 12 ; i++) {
            scale_root->add_available_value(i, note_names[i]);
        }
        scale_root->go_back_on_select = true;
        this->add(scale_root);
    }

};


class ChordMenuItem : public MenuItem {
    public:
    chord_instance_t *chord_data = nullptr;
    ChordMenuItem(const char *label, chord_instance_t *chord_data) : MenuItem(label) {
        this->chord_data = chord_data;
        this->selectable = false;
    }

    int display(Coord pos, bool selected, bool opened) override {
        pos.y = this->header(this->chord_data->get_pitch_string(), pos, selected, opened);
        return pos.y;
    }
};