#pragma once

#include "midi_helpers.h"

// from midi_helpers library
//String get_note_name(int pitch);
//const char *get_note_name_c(int pitch);

#include "menu.h"

// todo: make this accept int8_t, or different datatypes, or even lambda
class HarmonyStatus : public MenuItem {
    public:

        int *last_note = nullptr;
        int *current_note = nullptr;
        int *other_value = nullptr;

        // column headers
        const char *header_label[3] = {
            "Last",
            "Current",
            "Other"
        };

        HarmonyStatus(const char *label, bool show_header = true) : MenuItem(label) {
            this->selectable = false;
            this->show_header = show_header;
        };
        HarmonyStatus(const char *label, int *last_note, int *current_note, bool show_header = true) : HarmonyStatus(label, show_header) {
            //MenuItem(label);
            this->last_note = last_note;
            this->current_note = current_note;
        }
        HarmonyStatus(const char *label, int *last_note, int *current_note, int *other_value, bool show_header = true) : HarmonyStatus(label, last_note, current_note, show_header) {
            this->other_value = other_value;
        }
        HarmonyStatus(const char *label, int *last_note, int *current_note, int *other_value, const char *third_label, bool show_header = true) 
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

#include <scales.h>
class HarmonyDisplay : public MenuItem {
    public:

    SCALE *scale_number;
    int_fast8_t *scale_root;
    int_fast8_t *current_note;
    bool *quantise_enabled;

    HarmonyDisplay(const char *label, SCALE *scale_number, int_fast8_t *scale_root, int_fast8_t *current_note, bool *quantise_enabled) : MenuItem(label, false) {
        this->scale_number = scale_number;
        this->scale_root = scale_root;
        this->current_note = current_note;
        this->quantise_enabled = quantise_enabled;
    }

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

        // draw all white notes first
        for (int_fast8_t r = 0 ; r < NUM_CHROMATIC_NOTES ; r++) {
            x_pos = c * key_width;

            const bool playing = r == (*current_note % NUM_CHROMATIC_NOTES);
            const bool white_key = (r==0 || r==2 || r==4 || r==5 || r==7 || r==9 || r==11);

            if (white_key) { // white key
                uint16_t colour = playing ? YELLOW : (white_key?C_WHITE:tft->halfbright_565(C_WHITE));
                const bool valid = !*quantise_enabled || (*quantise_enabled && quantise_pitch(r, *scale_root, *scale_number)==r);
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

            const bool playing = r == (*current_note % NUM_CHROMATIC_NOTES);
            const bool white_key = (r==0 || r==2 || r==4 || r==5 || r==7 || r==9 || r==11);

            if (white_key) {
                c++;
            } else {    // black key
                uint16_t colour = playing ? YELLOW : (white_key?C_WHITE:tft->halfbright_565(C_WHITE));
                const bool valid = !*quantise_enabled || (*quantise_enabled && quantise_pitch(r, *scale_root, *scale_number)==r);
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
        //tft->printf("drew %i white notes?\n", c);

        return tft->getCursorY();
    }

};