#pragma once

#include "midi_helpers.h"

#include "menu.h"

class NoteDisplay : public MenuItem {
public:
    NoteTracker *note_tracker = nullptr;

    NoteDisplay(const char *label, NoteTracker *note_tracker) : MenuItem(label) {
        this->note_tracker = note_tracker;
    };

    virtual int display(Coord pos, bool selected, bool opened) override {
        tft->setCursor(pos.x, pos.y);
        pos.y = header(label, pos, selected, opened);
        //tft->setTextColor(rgb(0xFFFFFF),0);
        tft->setCursor(0, pos.y);
        colours(opened);

        tft->setTextSize(2);

        //this->renderValue(selected, opened, MENU_C_MAX);
        for (int i = 0 ; i < MIDI_NUM_NOTES ; i++) {
            if (note_tracker->is_note_held(i)) {
                tft->print(get_note_name_c(i));
                tft->print(" ");
            }
        }
        tft->println();

        return tft->getCursorY();
    }

};

#define NUM_CHROMATIC_NOTES 12
#define NUM_WHITE_NOTES 7

#include <scales.h>
class NoteHarmonyDisplay : public MenuItem {
    public:

    scale_index_t *scale_number;
    int_fast8_t *scale_root;
    //int_fast8_t *current_note;
    quantise_mode_t *quantise_mode;
    NoteTracker *note_tracker = nullptr;

    NoteHarmonyDisplay(
        const char *label, 
        scale_index_t *scale_number, 
        int_fast8_t *scale_root, 
        NoteTracker *note_tracker, 
        quantise_mode_t *quantise_mode
    ) : MenuItem(label, false) {
        this->scale_number = scale_number;
        this->scale_root = scale_root;
        this->quantise_mode = quantise_mode;
        this->note_tracker = note_tracker;
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

        int8_t scale_root_to_use = get_effective_scale_root(*scale_root);
        scale_index_t scale_number_to_use = get_effective_scale_type(*scale_number);
        quantise_mode_t mode = quantise_mode ? *quantise_mode : QUANTISE_MODE_NONE;

        // draw all white notes first
        for (int_fast8_t r = 0 ; r < NUM_CHROMATIC_NOTES ; r++) {
            x_pos = c * key_width;
            const bool playing = note_tracker->is_note_held_any_octave_transposed(r);
            const bool white_key = (r==0 || r==2 || r==4 || r==5 || r==7 || r==9 || r==11);
            if (white_key) {
                uint16_t colour = playing ? YELLOW : (white_key?C_WHITE:tft->halfbright_565(C_WHITE));

                const bool valid = 
                            !mode 
                            ||
                            (mode == quantise_mode_t::QUANTISE_MODE_CHORD && conductor->quantise_to_chord(r) == r) 
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

            const bool playing = note_tracker->is_note_held_any_octave_transposed(r); //r == (*current_note % NUM_CHROMATIC_NOTES);
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