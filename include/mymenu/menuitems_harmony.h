#pragma once

#include "midi_helpers.h"

// from midi_helpers library
//String get_note_name(int pitch);
//const char *get_note_name_c(int pitch);

#include "conductor.h"

#include "menu.h"

// todo: make this accept int8_t, or different datatypes, or even lambda
class HarmonyStatus : public MenuItem {
    public:

        int8_t *last_note = nullptr;
        int8_t *current_note = nullptr;
        int8_t *other_value = nullptr;

        // column headers
        const char *header_label[3] = {
            "Last",
            "Current",
            "Other"
        };

        HarmonyStatus(const char *label, bool show_header = true) : MenuItem(label) {
            this->selectable = false;
            this->show_header = show_header;
            IF_MENU_PERF_PARTIAL_UPDATES(this->add_redraw_policy(REDRAW_ON_CUSTOM);)   // to update when notes change without needing to redraw whole menu
        };
        HarmonyStatus(const char *label, int8_t *last_note, int8_t *current_note, bool show_header = true) : HarmonyStatus(label, show_header) {
            //MenuItem(label);
            this->last_note = last_note;
            this->current_note = current_note;
        }
        HarmonyStatus(const char *label, int8_t *last_note, int8_t *current_note, int8_t *other_value, bool show_header = true) : HarmonyStatus(label, last_note, current_note, show_header) {
            this->other_value = other_value;
        }
        HarmonyStatus(const char *label, int8_t *last_note, int8_t *current_note, int8_t *other_value, const char *third_label, bool show_header = true) 
            : HarmonyStatus(label, last_note, current_note, other_value, show_header) {
                this->set_header(2, third_label);
            }
        /*virtual void configure(int *last_note, int *current_note) {   // for if we need to late-bind the harmony note sources
            this->last_note = last_note;
            this->current_note = current_note;   
        }*/
        void set_header(int id, const char *label) {
            header_label[id] = label;
        }
        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x, pos.y);
            pos.y = header(label, pos, selected, opened);
            //tft->setTextColor(rgb(0xFFFFFF),0);
            tft->setCursor(0, pos.y);
            colours(opened);

            this->renderValue(selected, opened, MENU_C_MAX);

            return tft->getCursorY();
        }

        #if MENU_PERF_PARTIAL_UPDATES
            int8_t last_note_value, current_note_value, other_note_value;
            virtual bool check_needs_redraw_custom(bool selected, bool opened) override {
                bool needs_redraw = false;
                if (last_note && *last_note != last_note_value) {
                    last_note_value = *last_note;
                    needs_redraw = true;
                }
                if (current_note && *current_note != current_note_value) {
                    current_note_value = *current_note;
                    needs_redraw = true;
                }
                if (other_value && *other_value != other_note_value) {
                    other_note_value = *other_value;
                    needs_redraw = true;
                }
                return needs_redraw;
            }
        #endif

        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
            tft->setTextSize(1);
            if (this->other_value!=nullptr) {
                tft->printf("%8s  :  %8s  :  %8s\n", (char*)header_label[0], (char*)header_label[1], (char*)header_label[2]);
            } else {
                tft->printf("%8s  :  %8s\n", (char*)header_label[0], (char*)header_label[1]);
            }

            tft->setTextSize(2);
            if (!last_note || !current_note) {
                tft->println((char *)"[not set]");
            } else if (this->other_value!=nullptr) {
                tft->printf("%4s : %4s : %4s\n",     // \n not needed on smaller screen because already fills row.. is needed on big tft?
                    (char*)(get_note_name_c(*last_note)), 
                    (char*)(get_note_name_c(*current_note)),
                    (char*)(get_note_name_c(*other_value))
                );
            } else {
                tft->printf("%4s : %4s\n",     // \n not needed on smaller screen because already fills row.. is needed on big tft?
                    (char*)(get_note_name_c(*last_note)), 
                    (char*)(get_note_name_c(*current_note))
                );
            }
            return tft->getCursorY();
        }
};

#define NUM_CHROMATIC_NOTES 12
#define NUM_WHITE_NOTES 7

#ifdef ENABLE_SCALES

#include <scales.h>
// graphical keyboard display of the current notes and scale
class HarmonyDisplay : public MenuItem {
    public:

    scale_index_t *scale_number;
    int_fast8_t *scale_root;
    int8_t *current_note;
    quantise_mode_t *quantise_mode;
    chord_identity_t *current_chord_identity = nullptr;

    HarmonyDisplay(const char *label, scale_index_t *scale_number, int_fast8_t *scale_root, int8_t *current_note, quantise_mode_t *quantise_mode, bool show_header = true) 
        : MenuItem(label, false, show_header) 
    {
        this->scale_number = scale_number;
        this->scale_root = scale_root;
        this->current_note = current_note;
        this->quantise_mode = quantise_mode;
        IF_MENU_PERF_PARTIAL_UPDATES(this->add_redraw_policy(REDRAW_ON_CUSTOM);)
    }

    HarmonyDisplay(const char *label, scale_index_t *scale_number, int_fast8_t *scale_root, int8_t *current_note, quantise_mode_t *quantise_mode, chord_identity_t *current_chord_identity = nullptr, bool show_header = true) 
        : HarmonyDisplay(label, scale_number, scale_root, current_note, quantise_mode, show_header) {
        this->current_chord_identity = current_chord_identity;
    }

    HarmonyDisplay(
        const char *label, 
        scale_identity_t *scale_identity, 
        int8_t *current_note, 
        quantise_mode_t *quantise_mode, 
        chord_identity_t *current_chord_identity = nullptr, 
        bool show_header = true
    ) : HarmonyDisplay(label, &scale_identity->scale_number, &scale_identity->root_note, current_note, quantise_mode, show_header) {
        this->current_chord_identity = current_chord_identity;
    }

    int8_t last_note_value = NOTE_OFF;
    scale_index_t last_scale_number;
    int_fast8_t last_scale_root;
    quantise_mode_t last_quantise_mode;
    chord_identity_t last_chord_identity;
    
    #if MENU_PERF_PARTIAL_UPDATES
        virtual bool check_needs_redraw_custom(bool selected, bool opened) override {
            bool diff = false;
            if (*this->current_note != this->last_note_value) {
                this->last_note_value = *this->current_note;
                diff = true;
            }
            if (this->scale_number != nullptr && *this->scale_number != this->last_scale_number) {
                this->last_scale_number = *this->scale_number;
                diff = true;
            }
            if (this->scale_root != nullptr && *this->scale_root != this->last_scale_root) {
                this->last_scale_root = *this->scale_root;
                diff = true;
            }
            if (this->quantise_mode != nullptr && *this->quantise_mode != this->last_quantise_mode) {
                this->last_quantise_mode = *this->quantise_mode;
                diff = true;
            }
            if (this->current_chord_identity != nullptr && this->current_chord_identity->diff(this->last_chord_identity)) {
                this->last_chord_identity = *this->current_chord_identity;
                diff = true;
            }
            return diff;
        }
    #endif

    virtual int display(Coord pos, bool selected, bool opened) override {
        tft->setCursor(pos.x, pos.y);
        pos.y = header(label, pos, selected, opened);
        //tft->setTextColor(rgb(0xFFFFFF),0);
        tft->setCursor(0, pos.y);
        //colours(opened);
        
        // draw a keyboard !
        int8_t key_width = tft->width() / NUM_WHITE_NOTES;
        int8_t key_height = 32;
        int8_t key_height_black = 20;
        int8_t gap = 4;

        int16_t x_pos = 0;

        int c = 0;

        int8_t scale_root_to_use = get_effective_scale_root(*scale_root);
        scale_index_t scale_number_to_use = get_effective_scale_type(*scale_number);
        quantise_mode_t mode = quantise_mode ? *quantise_mode : QUANTISE_MODE_NONE;

        // draw all white notes first
        for (int_fast8_t r = 0 ; r < NUM_CHROMATIC_NOTES ; r++) {
            x_pos = c * key_width;
            const bool playing = current_note != nullptr && r == (*current_note % NUM_CHROMATIC_NOTES);
            const bool white_key = (r==0 || r==2 || r==4 || r==5 || r==7 || r==9 || r==11);
            if (white_key) {
                uint16_t colour = playing ? YELLOW : (white_key?C_WHITE:tft->halfbright_565(C_WHITE));

                const bool valid = 
                            !mode 
                            ||
                            (mode == quantise_mode_t::QUANTISE_MODE_CHORD && current_chord_identity != nullptr && conductor->quantise_to_chord(r) == r) 
                            ||
                            (mode == quantise_mode_t::QUANTISE_MODE_SCALE && quantise_pitch_to_scale(r, scale_root_to_use, scale_number_to_use)==r);

                if (!valid) colour = tft->halfbright_565(colour);

                if (valid)
                    tft->fillRect(x_pos, pos.y, key_width-gap, key_height, colour);
                else
                    tft->drawRect(x_pos, pos.y, key_width-gap, key_height, colour);
                c++;
            }
        }

        // draw black notes after white notes, because they need to go 'on top'
        c = 0;
        for (int_fast8_t r = 0 ; r < NUM_CHROMATIC_NOTES ; r++) {
            x_pos = c * key_width;

            const bool playing = current_note != nullptr && r == (*current_note % NUM_CHROMATIC_NOTES);
            const bool white_key = (r==0 || r==2 || r==4 || r==5 || r==7 || r==9 || r==11);

            if (white_key) {
                c++;
            } else {    // black key
                uint16_t colour = playing ? YELLOW : (white_key?C_WHITE:tft->halfbright_565(C_WHITE));
                const bool valid = 
                            !mode 
                            ||
                            (mode == quantise_mode_t::QUANTISE_MODE_CHORD && conductor->quantise_to_chord(r, 2) == r) 
                            ||
                            (mode == quantise_mode_t::QUANTISE_MODE_SCALE && quantise_pitch_to_scale(r, scale_root_to_use, scale_number_to_use)==r);
                if (!valid) colour = tft->halfbright_565(colour);

                x_pos -= (key_width/2);
                if (valid)
                    tft->fillRect(x_pos, pos.y, key_width-gap, key_height_black, colour);
                else
                    tft->drawRect(x_pos, pos.y, key_width-gap, key_height_black, colour);
                //c++;
            }
        }

        tft->setCursor(0, pos.y + key_height + gap);

        return tft->getCursorY();
    }

};

#endif