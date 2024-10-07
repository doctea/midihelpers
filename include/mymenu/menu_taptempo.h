#pragma once

//#include "Config.h"
#ifdef ENABLE_SCREEN

#include "menu.h"
#include "menuitems.h"
#include "../bpm.h"

#include "../clock.h"

#include "../taptempo.h"

class TapTempoControl : public MenuItem {
    public:
        TapTempoTracker *taptempotracker = nullptr;
        TapTempoControl(char *label, TapTempoTracker *taptempotracker) : MenuItem(label) {
            this->taptempotracker = taptempotracker;
        };

        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.printf("positionindicator display for %s\n", label);
            tft->setCursor(pos.x,pos.y);
            header(this->label, pos, selected, opened);
            tft->setTextSize(2);

            tft->printf("Tap Estimate: %i\n", (int)taptempotracker->clock_tempo_estimate());
            tft->printf("Sample size: %i\n", taptempotracker->get_num_samples());
            tft->println(taptempotracker->is_tracking()?"Sensing.." : "Tap to start!");
            for (int i = 0 ; i < taptempotracker->get_num_samples() ; i++) {
                tft->printf("%i: %i\n", i, taptempotracker->get_history_gap(i));
            }

            return tft->getCursorY();
        }

        /*
        virtual bool knob_left() override {
            if (clock_mode==CLOCK_INTERNAL)
                set_bpm(bpm_current-1);
            return true;
        }
        virtual bool knob_right() override {
            if (clock_mode==CLOCK_INTERNAL)
                set_bpm(bpm_current+1);
            return true;
        }
        */

        virtual bool button_right() override {
            taptempotracker->clock_tempo_tap();
            //messages_log_add(String("tapped! ") + String(taptempotracker->get_num_samples()));
            //messages_log_add(String("estimate=") + String(taptempotracker->clock_tempo_estimate()));
            return SELECT_DONTEXIT;
        }
};

#endif