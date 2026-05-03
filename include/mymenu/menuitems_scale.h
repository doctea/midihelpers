#pragma once

#include "menuitems.h"
#include "menuitems_popout.h"

#include "midi_helpers.h"
#include "scales.h"

class ScaleMenuItem : public MenuItem {
    public:

        //SCALE *scale_number = nullptr; //SCALE::MAJOR;
        scale_index_t scale_number = SCALE_FIRST;
        int8_t root_note = SCALE_ROOT_A;
        bool full_display = true;

        ScaleMenuItem(const char *label, bool full_display = true) : MenuItem(label), full_display(full_display) {}

        virtual scale_index_t getScaleNumber() {
            return this->scale_number;
        }
        virtual int getRootNote() {
            return this->root_note;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            scale_index_t scale_number = this->getScaleNumber();
            int8_t root_note = this->getRootNote();

            char label[MENU_C_MAX];
            snprintf(label, MENU_C_MAX, 
                "%s: %-3i => %3s %s", this->label,  //  [%i]
                root_note, 
                (char*)get_note_name_c(root_note), 
                scales[scale_number]->label
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

                    byte quantised_note = quantise_pitch_to_scale(note, root_note, scale_number);

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
    void(TargetClass::*scale_setter_func)(scale_index_t) = nullptr;
    scale_index_t(TargetClass::*scale_getter_func)(void) = nullptr;
    void(TargetClass::*scale_root_setter_func)(int) = nullptr;
    int(TargetClass::*scale_root_getter_func)(void) = nullptr;

    ObjectScaleMenuItem(const char *label, 
        TargetClass *target,
        void(TargetClass::*scale_setter_func)(scale_index_t), 
        scale_index_t(TargetClass::*scale_getter_func)(void),
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
            scale_index_t scale_number = (target->*scale_getter_func)();
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
            scale_index_t scale_number = (target->*scale_getter_func)();
            scale_number++;
            (target->*scale_setter_func)(scale_number);
        }
        return true;
    }

    virtual scale_index_t getScaleNumber() override {
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
    void(TargetClass::*scale_setter_func)(scale_index_t) = nullptr;
    scale_index_t(TargetClass::*scale_getter_func)(void) = nullptr;

    void set_scale(int scale_number) {
        if (this->target_object!=nullptr && this->scale_setter_func!=nullptr)
            (this->target_object->*scale_setter_func)((scale_index_t)scale_number);
    }
    int get_scale() {
        if (this->target_object!=nullptr && this->scale_getter_func!=nullptr)
            return(this->target_object->*scale_getter_func)();
        return 0;
    }

    ObjectScaleMenuItemBar(
        const char *label, 
        TargetClass *target_object,
        void(TargetClass::*scale_setter_func)(scale_index_t),
        scale_index_t(TargetClass::*scale_getter_func)(void),
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
            // add all 12 notes, arranged in circle of fifths
            // todo: make this endlessly scrollable
            int note = 0;
            for (size_t i = 0 ; i < 12 ; i++) {
                scale_root->add_available_value(note, note_names[note]);
                note += 7;
                note %= 12;
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
                scale_selector->add_available_value(i, scales[i]->label);
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

template<class DataType>
class LambdaScaleTakeoverSelector : public LambdaSelectorControl<DataType> {
    public:

    static const int PREVIEW_NUM_CHROMATIC_NOTES = 12;
    static const int PREVIEW_NUM_WHITE_NOTES = 7;

    protected:

    vl::Func<scale_index_t(void)> preview_scale_getter_func;
    vl::Func<int8_t(void)> preview_root_getter_func;
    bool has_preview_scale_getter = false;
    bool has_preview_root_getter = false;
    int overlay_preview_index = -1;

    public:

    LambdaScaleTakeoverSelector(
        const char* label,
        vl::Func<void(DataType)> setter_func,
        vl::Func<DataType(void)> getter_func,
        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr,
        bool go_back_on_select = false,
        bool direct = false
    ) : LambdaSelectorControl<DataType>(label, setter_func, getter_func, on_change_handler, go_back_on_select, direct) {
    }

    void set_preview_scale_getter(vl::Func<scale_index_t(void)> getter_func) {
        this->preview_scale_getter_func = getter_func;
        this->has_preview_scale_getter = true;
    }

    void set_preview_root_getter(vl::Func<int8_t(void)> getter_func) {
        this->preview_root_getter_func = getter_func;
        this->has_preview_root_getter = true;
    }

    int wrap_index(int index) {
        const int count = this->available_values!=nullptr ? (int)this->available_values->size() : 0;
        if (count<=0)
            return -1;
        while (index < 0)
            index += count;
        while (index >= count)
            index -= count;
        return index;
    }

    virtual const char *get_overlay_subtitle(int display_index) {
        (void)display_index;
        return nullptr;
    }

    virtual bool get_overlay_scale_preview(int display_index, scale_index_t &out_scale, int8_t &out_root) {
        (void)display_index;
        if (!this->has_preview_scale_getter || !this->has_preview_root_getter)
            return false;

        out_scale = get_effective_scale_type(this->preview_scale_getter_func());
        out_root = get_effective_scale_root(this->preview_root_getter_func());
        return out_scale>=0 && out_scale<(int)NUMBER_SCALES;
    }

    static bool is_white_preview_key(int8_t note) {
        return note==0 || note==2 || note==4 || note==5 || note==7 || note==9 || note==11;
    }

    static void draw_scale_preview_keyboard(
        DisplayTranslator *tft,
        int x,
        int y,
        int max_width,
        int max_height,
        scale_index_t scale_number,
        int8_t scale_root
    ) {
        if (tft==nullptr || max_width<=0 || max_height<=0)
            return;

        const int key_gap = 2;
        const int key_width = max_width / PREVIEW_NUM_WHITE_NOTES;
        if (key_width<=key_gap)
            return;

        const int key_height = max_height;
        const int key_height_black = (key_height * 5) / 8;

        const uint16_t white_key_colour = C_WHITE;
        const uint16_t black_key_colour = tft->halfbright_565(C_WHITE);

        int16_t x_pos = x;
        int white_index = 0;
        for (int_fast8_t note = 0 ; note < PREVIEW_NUM_CHROMATIC_NOTES ; note++) {
            if (!is_white_preview_key(note))
                continue;

            x_pos = x + (white_index * key_width);
            const bool in_scale = quantise_pitch_to_scale(note, scale_root, scale_number) == note;

            if (in_scale)
                tft->fillRect(x_pos+2, y, key_width-key_gap-2, key_height-2, white_key_colour);
            else
                tft->drawRect(x_pos+2, y, key_width-key_gap-2, key_height-2, white_key_colour);

            white_index++;
        }

        white_index = 0;
        for (int_fast8_t note = 0 ; note < PREVIEW_NUM_CHROMATIC_NOTES ; note++) {
            if (is_white_preview_key(note)) {
                white_index++;
                continue;
            }

            x_pos = x + (white_index * key_width) - (key_width / 2);
            const bool in_scale = quantise_pitch_to_scale(note, scale_root, scale_number) == note;

            if (in_scale)
                tft->fillRect(x_pos+2, y, key_width-key_gap-2, key_height_black-2, black_key_colour);
            else
                tft->drawRect(x_pos+2, y, key_width-key_gap-2, key_height_black-2, black_key_colour);
        }
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        if (!opened)
            return LambdaSelectorControl<DataType>::display(pos, selected, opened);

        const int internal_idx = (int)this->get_internal_value();
        const int current_idx = this->get_index_for_value(this->getter_func());
        const int display_index = (this->available_values!=nullptr && internal_idx>=0 && internal_idx<(int)this->available_values->size()) ? internal_idx : current_idx;

        const int left_index = this->wrap_index(display_index - 1);
        const int right_index = this->wrap_index(display_index + 1);
        char left_hint[MENU_C_MAX];
        char right_hint[MENU_C_MAX];
        snprintf(left_hint, MENU_C_MAX, "< %s", this->get_label_for_index(left_index));
        snprintf(right_hint, MENU_C_MAX, "%s >", this->get_label_for_index(right_index));

        SelectorTakeoverOverlaySpec overlay;
        overlay.title = this->label;
        overlay.subtitle = this->get_overlay_subtitle(display_index);
        overlay.value = this->get_label_for_index(display_index);
        overlay.left_hint = left_hint;
        overlay.right_hint = right_hint;
        overlay.frame_colour = selected ? GREEN : C_WHITE;
        overlay.left_hint_fg = C_WHITE; // this->tft->halfbright_565(C_WHITE);
        overlay.right_hint_fg = C_WHITE; // this->tft->halfbright_565(C_WHITE);
        overlay.box_padding = 4;
        overlay.min_box_h = 28;
        overlay.subtitle_top_gap = 4;

        scale_index_t preview_scale = SCALE_FIRST;
        int8_t preview_root = SCALE_ROOT_A;
        if (this->get_overlay_scale_preview(display_index, preview_scale, preview_root)) {
            this->overlay_preview_index = display_index;
            overlay.has_extra = true;
            overlay.extra_height = 32;
            overlay.draw_extra_fn = [](void *userdata, DisplayTranslator *tft, int x, int y, int max_width, int max_height) {
                auto *self = static_cast<LambdaScaleTakeoverSelector<DataType>*>(userdata);
                scale_index_t scale_to_draw = SCALE_FIRST;
                int8_t root_to_draw = SCALE_ROOT_A;
                const int draw_index = self->overlay_preview_index;
                if (!self->get_overlay_scale_preview(draw_index, scale_to_draw, root_to_draw))
                    return;
                LambdaScaleTakeoverSelector<DataType>::draw_scale_preview_keyboard(tft, x, y, max_width, max_height, scale_to_draw, root_to_draw);
            };
            overlay.draw_extra_userdata = this;
        }

        return menu_draw_selector_takeover_overlay(this->tft, pos, overlay);
    }

    virtual bool wants_fullscreen_overlay_when_opened_in_bar() override {
        return true;
    }
};

template<class DataType>
class LambdaScaleSelector : public LambdaScaleTakeoverSelector<DataType> {
    public:

    LambdaScaleSelector(
        const char* label, 
        vl::Func<void(DataType)> setter_func,
        vl::Func<DataType(void)> getter_func,
        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr,
        bool go_back_on_select = false,
        bool direct = false
    ) : LambdaScaleTakeoverSelector<DataType>(label, setter_func, getter_func, on_change_handler, go_back_on_select, direct) {
    }

    virtual const char *get_label() override {
        scale_index_t scale_number = (scale_index_t)this->get_current_value();
        scale_number = get_effective_scale_type(scale_number);
        if (scale_number!=SCALE_GLOBAL)
            return (char*)scales[scale_number]->pattern->label;
        else
            return "[global]";        
    }

    virtual const char *get_overlay_subtitle(int display_index) override {
        if (display_index<0 || this->available_values==nullptr || display_index>=(int)this->available_values->size())
            return "None";

        scale_index_t selected = this->available_values->get(display_index).value;
        if (selected == SCALE_GLOBAL)
            return "Global";

        selected = get_effective_scale_type(selected);
        if (selected>=0 && selected<(int)NUMBER_SCALES && scales[selected]!=nullptr && scales[selected]->pattern!=nullptr && scales[selected]->pattern->label!=nullptr)
            return scales[selected]->pattern->label;

        return "None";
    }

    virtual bool get_overlay_scale_preview(int display_index, scale_index_t &out_scale, int8_t &out_root) override {
        if (!this->has_preview_root_getter)
            return false;

        out_root = get_effective_scale_root(this->preview_root_getter_func());

        scale_index_t selected_scale = SCALE_GLOBAL;
        if (display_index>=0 && this->available_values!=nullptr && display_index<(int)this->available_values->size())
            selected_scale = this->available_values->get(display_index).value;
        else if (this->has_preview_scale_getter)
            selected_scale = this->preview_scale_getter_func();

        out_scale = get_effective_scale_type(selected_scale);
        return out_scale>=0 && out_scale<(int)NUMBER_SCALES;
    }
};

class LambdaScaleMenuItemBar : public SubMenuItemBar {
    public:

    bool allow_global = false;

    vl::Func<void(scale_index_t)> scale_setter_func;
    vl::Func<scale_index_t(void)> scale_getter_func;
    vl::Func<void(int8_t)> scale_root_setter_func;
    vl::Func<int8_t(void)> scale_root_getter_func;

    LambdaScaleMenuItemBar(
        const char *label,
        vl::Func<void(scale_index_t)> scale_setter_func,
        vl::Func<scale_index_t(void)> scale_getter_func,
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

        // choose which set of options to use (creating if necessary)

        static LinkedList<LambdaSelectorControl<scale_index_t>::option> *scale_selector_options_with_global;
        static LinkedList<LambdaSelectorControl<scale_index_t>::option> *scale_selector_options_no_global;
        static LinkedList<LambdaSelectorControl<int8_t>::option> *scale_root_options_with_global;
        static LinkedList<LambdaSelectorControl<int8_t>::option> *scale_root_options_no_global;
    
        LinkedList<LambdaSelectorControl<int8_t>::option> *scale_root_options = nullptr;
        LinkedList<LambdaSelectorControl<scale_index_t>::option> *scale_selector_options = nullptr;

        if (allow_global) {
            // first, versions with 'global' as an option
            if (scale_root_options_with_global==nullptr) {
                scale_root_options_with_global = new LinkedList<LambdaSelectorControl<int8_t>::option>();
                scale_root_options_with_global->add(LambdaSelectorControl<int8_t>::option {SCALE_GLOBAL_ROOT, "[global]"});
            }
            scale_root_options = scale_root_options_with_global;

            if (scale_selector_options_with_global==nullptr) {
                scale_selector_options_with_global = new LinkedList<LambdaSelectorControl<scale_index_t>::option>();
                scale_selector_options_with_global->add(LambdaSelectorControl<scale_index_t>::option {SCALE_GLOBAL, "[global]"});
            }
            scale_selector_options = scale_selector_options_with_global;
        } else {
            // then versions without 'global' as an option
            if (scale_root_options_no_global==nullptr) {
                scale_root_options_no_global = new LinkedList<LambdaSelectorControl<int8_t>::option>();
            }
            scale_root_options = scale_root_options_no_global;

            if (scale_selector_options_no_global==nullptr) {
                scale_selector_options_no_global = new LinkedList<LambdaSelectorControl<scale_index_t>::option>();
            }
            scale_selector_options = scale_selector_options_no_global;            
        }
       
        // todo: make whether its circle of fifths or chromatic configurable
        // todo: make this endlessly scrollable

        // add all 12 notes to scale_root_options, arranged in circle of fifths
        if (scale_root_options->size()<=1) {
            Serial.printf("populating scale_root_options with 12 notes\n");
            int8_t note = 0;
            for (size_t i = 0 ; i < 12 ; i++) {
                scale_root_options->add(LambdaSelectorControl<int8_t>::option { note, note_names[note]});
                note += 7;
                note %= 12;
            }
        } 

        // add all the scales to the scale_selector_options list
        if (scale_selector_options->size()<=1) {
            Serial.printf("populating scale_selector_options with %i scales\n", NUMBER_SCALES);
            for (size_t i = 0 ; i < NUMBER_SCALES ; i++) {
                if (debug) Serial.printf("LambdaScaleMenuItemBar: adding scale %i: %s @ %p\n", i, scales[i]->label, scales[i]);
                //print_scale(0, *scales[i]);
                scale_selector_options->add(LambdaScaleSelector<scale_index_t>::option { (scale_index_t)i, scales[i]->label });
            }
        }

        // create the scale_root control + add to this menu
        LambdaScaleTakeoverSelector<int8_t> *scale_root = new LambdaScaleTakeoverSelector<int8_t>(
            "Root key", 
            scale_root_setter_func, 
            scale_root_getter_func,
            nullptr,
            true,
            true
        );
        scale_root->set_preview_scale_getter(scale_getter_func);
        scale_root->set_preview_root_getter(scale_root_getter_func);
        scale_root->set_available_values(scale_root_options);
        scale_root->go_back_on_select = true;
        this->add(scale_root);

        // create the scale_selector control + add to this menu
        LambdaScaleSelector<scale_index_t> *scale_selector = new LambdaScaleSelector<scale_index_t>(
            "Scale type", 
            scale_setter_func,
            scale_getter_func,
            nullptr,
            true,
            true //false // direct -- setting to 'true' causes unacceptable lag when scrolling through the list    // TODO: fix this!!
        );
        scale_selector->set_preview_scale_getter(scale_getter_func);
        scale_selector->set_preview_root_getter(scale_root_getter_func);
        scale_selector->set_available_values(scale_selector_options);
        scale_selector->go_back_on_select = true;
        this->add(scale_selector);
    }

    void set_scale(scale_index_t scale_number) {
        scale_setter_func(scale_number);
    }
    scale_index_t get_scale() {
        return scale_getter_func();
    }
};

class LambdaChordSubMenuItemBar : public SubMenuItemBar {
    public:

    vl::Func<void(int8_t)> chord_degree_setter_func;
    vl::Func<int8_t(void)> chord_degree_getter_func;
    vl::Func<void(CHORD::Type)> chord_setter_func;
    vl::Func<CHORD::Type(void)> chord_getter_func;
    vl::Func<void(int8_t)> inversion_setter_func;
    vl::Func<int8_t(void)> inversion_getter_func;

    LambdaChordSubMenuItemBar(
        const char *label,
        vl::Func<void(int8_t)> chord_degree_setter_func,
        vl::Func<int8_t(void)> chord_degree_getter_func,
        vl::Func<void(CHORD::Type)> chord_setter_func,
        vl::Func<CHORD::Type(void)> chord_getter_func,
        vl::Func<void(int8_t)> inversion_setter_func,
        vl::Func<int8_t(void)> inversion_getter_func,
        bool allow_global = true,
        bool show_sub_headers = true,
        bool show_header = true
    ) : SubMenuItemBar (label, show_sub_headers, show_header) {
        this->chord_setter_func = chord_setter_func;
        this->chord_getter_func = chord_getter_func;
        this->inversion_setter_func = inversion_setter_func;
        this->inversion_getter_func = inversion_getter_func;

        LambdaSelectorControl<int8_t> *chord_degree_selector = new LambdaSelectorControl<int8_t>(
            "Chord degree",
            chord_degree_setter_func,
            chord_degree_getter_func,
            nullptr,
            true,
            true
        );
        if (allow_global)
            chord_degree_selector->add_available_value(-1, "[global]"); 
        chord_degree_selector->add_available_value(0, "[none]");
        chord_degree_selector->add_available_value(1, "1st");
        chord_degree_selector->add_available_value(2, "2nd");
        chord_degree_selector->add_available_value(3, "3rd");
        chord_degree_selector->add_available_value(4, "4th");
        chord_degree_selector->add_available_value(5, "5th");
        chord_degree_selector->add_available_value(6, "6th");
        chord_degree_selector->add_available_value(7, "7th");
        
        this->add(chord_degree_selector);

        LambdaSelectorControl<CHORD::Type> *chord_selector = new LambdaSelectorControl<CHORD::Type>(
            "Chord type", 
            chord_setter_func,
            chord_getter_func,
            nullptr,
            true,
            true
        );
        for (size_t i = 0 ; i < NUMBER_CHORDS ; i++) {
            chord_selector->add_available_value((CHORD::Type)i, chords[i].label);
        }
        chord_selector->go_back_on_select = true;
        this->add(chord_selector);

        LambdaNumberControl<int8_t> *inversion_selector = new LambdaNumberControl<int8_t>(
            "Inversion", 
            inversion_setter_func,
            inversion_getter_func,
            nullptr,
            0, MAX_INVERSIONS, true
        );
        this->add(inversion_selector);
    }
};

class LambdaChordSubMenuItemBarWithIndicator : public LambdaChordSubMenuItemBar {
    public:

    int8_t my_section, my_bar;
    int8_t *current_section, *current_bar;

    bool is_on_current_section_bar() {
        return this->my_section == *this->current_section && this->my_bar == *this->current_bar;
    }

    LambdaChordSubMenuItemBarWithIndicator(
        const char *label,
        vl::Func<void(int8_t)> chord_degree_setter_func,
        vl::Func<int8_t(void)> chord_degree_getter_func,
        vl::Func<void(CHORD::Type)> chord_setter_func,
        vl::Func<CHORD::Type(void)> chord_getter_func,
        vl::Func<void(int8_t)> inversion_setter_func,
        vl::Func<int8_t(void)> inversion_getter_func,
        int8_t my_section,
        int8_t my_bar,
        int8_t *current_section,
        int8_t *current_bar,
        bool allow_global = true,
        bool show_sub_headers = true,
        bool show_header = true
    ) : LambdaChordSubMenuItemBar (
            label,
            chord_degree_setter_func,
            chord_degree_getter_func,
            chord_setter_func,
            chord_getter_func,
            inversion_setter_func,
            inversion_getter_func,
            allow_global,
            show_sub_headers,
            show_header
        ),
        current_section(current_section),
        current_bar(current_bar)
    {
        this->my_section = my_section;
        this->my_bar = my_bar;

        // todo: show the chord root note too
        // todo: attempt to show if chord is major, minor, dim, etc?

        // indicate if this is the currently-playing bar
        this->add(new CallbackMenuItem(
            "current?",
            [=]() -> const char* {
                if (is_on_current_section_bar()) {
                    return "*";
                } else {
                    return " ";
                }
            },
            [=]() -> uint16_t { return is_on_current_section_bar() ? GREEN : C_WHITE; },
            false
        ));
    }

    virtual int get_max_pixel_width(int item_number) override {
        // leave 1 character at the end for the indicator
        return (unsigned int)item_number < this->items->size() -1 ? 
            (tft->width() - tft->characterWidth()) / 3 :
            tft->characterWidth()
            ;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        return LambdaChordSubMenuItemBar::display(pos, selected, opened);
    }
};

class LambdaPlaylistSubMenuItemBarWithIndicator : public SubMenuItemBar {
    public:

    int8_t my_playlist_index;
    int8_t *current_playlist_index;

    vl::Func<void(int8_t)> section_setter_func;
    vl::Func<int8_t(void)> section_getter_func;
    vl::Func<void(int8_t)> repeats_setter_func;
    vl::Func<int8_t(void)> repeats_getter_func;

    bool is_current_playlist() {
        return this->my_playlist_index == *this->current_playlist_index;
    }

    LambdaPlaylistSubMenuItemBarWithIndicator(
        const char *label,
        vl::Func<void(int8_t)> section_setter_func,
        vl::Func<int8_t(void)> section_getter_func,
        vl::Func<void(int8_t)> repeats_setter_func,
        vl::Func<int8_t(void)> repeats_getter_func,
        int8_t my_playlist_index,
        int8_t *current_playlist_index,
        int8_t num_song_sections,
        int8_t max_repeats,
        bool show_sub_headers = true,
        bool show_header = true
    ) : SubMenuItemBar (label, show_sub_headers, show_header),
        current_playlist_index(current_playlist_index)
    {
        this->my_playlist_index = my_playlist_index;

        this->section_setter_func = section_setter_func;
        this->section_getter_func = section_getter_func;
        this->repeats_setter_func = repeats_setter_func;
        this->repeats_getter_func = repeats_getter_func;

        this->add(new LambdaNumberControl<int8_t>(
            "Section", 
            section_setter_func, 
            section_getter_func,
            nullptr, 
            (int8_t)0, num_song_sections - 1,
            true, true
        ));
        this->add(new LambdaNumberControl<int8_t>(
            "Repeats", 
            repeats_setter_func, 
            repeats_getter_func,
            nullptr,            
            (int8_t)0, max_repeats,
            true, true
        ));

        // indicate if this is the currently-playing playlist
        this->add(new CallbackMenuItem(
            "current?",
            [=]() -> const char* {
                if (this->is_current_playlist()) {
                    return "*";
                } else {
                    return " ";
                }
            },
            [=]() -> uint16_t { return this->is_current_playlist() ? GREEN : C_WHITE; },
            false
        ));
    }

    virtual int get_max_pixel_width(int item_number) override {
        // leave 1 character at the end for the indicator
        return ((unsigned int)item_number) < this->items->size() -1 ? 
            (tft->width() - tft->characterWidth()) / 3 :
            tft->characterWidth()
            ;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        return SubMenuItemBar::display(pos, selected, opened);
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

// Dedicated selector control for quantise_mode_t - pre-populated with Off/Scale/Chord options.
// Use instead of manually building a LambdaSelectorControl<int8_t> for quantise mode.
class LambdaQuantiseModeControl : public LambdaSelectorControl<int8_t> {
    public:

    LambdaQuantiseModeControl(
        const char *label,
        vl::Func<void(int8_t)> setter_func,
        vl::Func<int8_t(void)> getter_func,
        bool go_back_on_select = true,
        bool direct = true
    ) : LambdaSelectorControl<int8_t>(label, setter_func, getter_func, nullptr, go_back_on_select, direct) {
        static LinkedList<LambdaSelectorControl<int8_t>::option> *quantise_mode_options = nullptr;
        if (quantise_mode_options == nullptr) {
            quantise_mode_options = new LinkedList<LambdaSelectorControl<int8_t>::option>();
            quantise_mode_options->add(LambdaSelectorControl<int8_t>::option { (int8_t)QUANTISE_MODE_NONE,  "Off"   });
            quantise_mode_options->add(LambdaSelectorControl<int8_t>::option { (int8_t)QUANTISE_MODE_SCALE, "Scale" });
            quantise_mode_options->add(LambdaSelectorControl<int8_t>::option { (int8_t)QUANTISE_MODE_CHORD, "Chord" });
        }
        this->set_available_values(quantise_mode_options);
    }
};