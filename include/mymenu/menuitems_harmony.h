#pragma once

#include "midi_helpers.h"

// from midi_helpers library
//String get_note_name(int pitch);
//const char *get_note_name_c(int pitch);

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

        HarmonyStatus(const char *label) : MenuItem(label) {
            this->selectable = false;
        };
        HarmonyStatus(const char *label, int *last_note, int *current_note) : HarmonyStatus(label) {
            //MenuItem(label);
            this->last_note = last_note;
            this->current_note = current_note;
        }
        HarmonyStatus(const char *label, int *last_note, int *current_note, int *other_value) : HarmonyStatus(label, last_note, current_note) {
            this->other_value = other_value;
        }
        HarmonyStatus(const char *label, int *last_note, int *current_note, int *other_value, const char *third_label) 
            : HarmonyStatus(label, last_note, current_note, other_value) {
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
