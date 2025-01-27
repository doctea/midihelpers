#pragma once

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
    static LinkedList<ObjectSelectorControl<TargetClass,int>> *scale_root_options = nullptr;
    static LinkedList<ObjectSelectorControl<TargetClass,int>> *scale_selector_options = nullptr;

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

        this->target_object = target_object;

        // assign these to self, so that we can proxy them via this->set_scale and this->get_scale
        this->scale_setter_func = scale_setter_func;
        this->scale_getter_func = scale_getter_func;

        ObjectSelectorControl<TargetClass,int> *scale_root = new ObjectSelectorControl<TargetClass,int>(
            "Root key", 
            this->target_object, 
            scale_root_setter_func, 
            scale_root_getter_func,
            nullptr,
            true
        );
        if (scale_root_options==nullptr) {
            for (size_t i = 0 ; i < 12 ; i++) {
                scale_root->add_available_value(i, note_names[i]);
            }
            scale_root_options = scale_root->get_available_values();
        } else {
            scale_root->set_available_values(scale_root_options);
        }
        scale_root->go_back_on_select = true;
        this->add(scale_root);


        // use self as intermediary to real target object in order to wrap int/SCALE type
        ObjectSelectorControl<ObjectScaleMenuItemBar,int> *scale_selector = new ObjectSelectorControl<ObjectScaleMenuItemBar,int>(
            "Scale type", 
            this, 
            &ObjectScaleMenuItemBar::set_scale, 
            &ObjectScaleMenuItemBar::get_scale,
            nullptr,
            true
        );
        if(scale_selector_options==nullptr) {
            for (size_t i = 0 ; i < NUMBER_SCALES ; i++) {
                scale_selector->add_available_value(i, scales[i].label);
            }   
            scale_selector_options = scale_selector->get_available_values();
        } else {
            scale_selector->set_available_values(scale_selector_options);
        }
        scale_selector->go_back_on_select = true;
        this->add(scale_selector);
    }
};


#include "functional-vlpp.h"
#include "menuitems_lambda_selector.h"

class LambdaScaleMenuItemBar : public SubMenuItemBar {
    public:

    bool allow_global = false;

    vl::Func<void(SCALE)> scale_setter_func;
    vl::Func<SCALE(void)> scale_getter_func;
    vl::Func<void(int8_t)> scale_root_setter_func;
    vl::Func<int8_t(void)> scale_root_getter_func;
    vl::Func<void(int8_t)> chord_degree_setter_func;
    vl::Func<int8_t(void)> chord_degree_getter_func;

    LambdaScaleMenuItemBar(
        const char *label,
        vl::Func<void(SCALE)> scale_setter_func,
        vl::Func<SCALE(void)> scale_getter_func,
        vl::Func<void(int8_t)> scale_root_setter_func,
        vl::Func<int8_t(void)> scale_root_getter_func,
        bool allow_global = false,
        bool show_sub_headers = true,
        bool show_header = true
    ) : SubMenuItemBar (label, show_sub_headers, show_header) {
        this->scale_setter_func = scale_setter_func;
        this->scale_getter_func = scale_getter_func;
        this->scale_root_setter_func = scale_root_setter_func;
        this->scale_root_getter_func = scale_root_getter_func;

        LambdaSelectorControl<int8_t> *scale_root = new LambdaSelectorControl<int8_t>(
            "Root key", 
            scale_root_setter_func, 
            scale_root_getter_func,
            nullptr,
            true,
            true
        );
        if (allow_global)
            scale_root->add_available_value(SCALE_GLOBAL_ROOT, "[use global]");
        for (size_t i = 0 ; i < 12 ; i++) {
            scale_root->add_available_value(i, note_names[i]);
        }
        scale_root->go_back_on_select = true;
        this->add(scale_root);

        LambdaSelectorControl<SCALE> *scale_selector = new LambdaSelectorControl<SCALE>(
            "Scale type", 
            scale_setter_func,
            scale_getter_func,
            nullptr,
            true,
            true
        );
        if (allow_global)
            scale_selector->add_available_value(SCALE::GLOBAL, "[use global]");
        for (size_t i = 0 ; i < NUMBER_SCALES ; i++) {
            scale_selector->add_available_value((SCALE)i, scales[i].label);
        }
        scale_selector->go_back_on_select = true;
        this->add(scale_selector);
    }

    LambdaScaleMenuItemBar(
        const char *label,
        vl::Func<void(SCALE)> scale_setter_func,
        vl::Func<SCALE(void)> scale_getter_func,
        vl::Func<void(int8_t)> scale_root_setter_func,
        vl::Func<int8_t(void)> scale_root_getter_func,
        vl::Func<void(int8_t)> chord_degree_setter_func,
        vl::Func<int8_t(void)> chord_degree_getter_func,
        bool allow_global = false,
        bool show_sub_headers = true,
        bool show_header = true
    ) : LambdaScaleMenuItemBar (label, scale_setter_func, scale_getter_func, scale_root_setter_func, scale_root_getter_func, allow_global, show_sub_headers, show_header) {

        LambdaSelectorControl<int8_t> *chord_degree_selector = new LambdaSelectorControl<int8_t>(
            "Chord degree",
            chord_degree_setter_func,
            chord_degree_getter_func,
            nullptr,
            true,
            true
        );
        if (allow_global)
            chord_degree_selector->add_available_value(-1, "[use global]");
        chord_degree_selector->add_available_value(0, "[none]");
        chord_degree_selector->add_available_value(1, "1st");
        chord_degree_selector->add_available_value(2, "2nd");
        chord_degree_selector->add_available_value(3, "3rd");
        chord_degree_selector->add_available_value(4, "4th");
        chord_degree_selector->add_available_value(5, "5th");
        chord_degree_selector->add_available_value(6, "6th");
        chord_degree_selector->add_available_value(7, "7th");
        
        this->add(chord_degree_selector);
    }

    void set_scale(SCALE scale_number) {
        scale_setter_func(scale_number);
    }
    SCALE get_scale() {
        return scale_getter_func();
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

template<class TargetClass, class DataType>
class ObjectScaleNoteMenuItem : public ObjectNumberControl<TargetClass, DataType> {
    public:

    ObjectScaleNoteMenuItem(const char* label, 
            TargetClass *target_object, 
            void(TargetClass::*setter_func)(DataType), 
            DataType(TargetClass::*getter_func)(), 
            void (*on_change_handler)(DataType last_value, DataType new_value),
            DataType minimumDataValue,
            DataType maximumDataValue,
            bool go_back_on_select = false,
            bool direct = false
    ) : ObjectNumberControl<TargetClass,DataType>(label, target_object, setter_func, getter_func, on_change_handler, minimumDataValue, maximumDataValue, go_back_on_select, direct) 
        {}

    virtual const char *getFormattedValue(int value) override {
        return get_note_name_c(value);
    }
};

template<class DataType>
class LambdaScaleNoteMenuItem : public LambdaNumberControl<DataType> {
    public:

    using setter_func_def = vl::Func<void(DataType)>;
    using getter_func_def = vl::Func<DataType(void)>;

    LambdaScaleNoteMenuItem(const char* label, 
        setter_func_def setter_func,
        getter_func_def getter_func,
        void (*on_change_handler)(DataType last_value, DataType new_value),
        DataType minimumDataValue,
        DataType maximumDataValue,
        bool go_back_on_select = false,
        bool direct = false
    ) : LambdaNumberControl<DataType>(label, setter_func, getter_func, on_change_handler, minimumDataValue, maximumDataValue, go_back_on_select, direct) 
        {}

    virtual const char *getFormattedValue(int value) override {
        return get_note_name_c(value);
    }
};