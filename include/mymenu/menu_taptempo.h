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
            tft->printf("Sample size: [%i/%i]\n", taptempotracker->get_num_samples(), taptempotracker->get_max_sample_size());
            //tft->println(taptempotracker->is_tracking()?"Sensing.." : "Tap to start!");
            //for (int i = 0 ; i < taptempotracker->get_num_samples() ; i++) {
            //    tft->printf("%i: %i\n", i, taptempotracker->get_history_gap(i));
            //}

            tft->printf("Beat phase %%: %i\n", taptempotracker->beat_phase*100.0);
            tft->printf("Tap phase  %%: %i\n", taptempotracker->tap_phase*100.0);
            tft->printf("Phase diffab: %i\n", (taptempotracker->beat_phase - taptempotracker->tap_phase)*100.0);
            //if (taptempotracker->is_tracking())
            char asf[20];
            snprintf(asf, 20, "%3.2f", taptempotracker->phase_diff_pc);
                tft->printf("Phase diff %%: %s\n", asf);
                tft->printf("Last temp bpm: %i\n", (int)taptempotracker->last_temp_bpm);
            //else 
              //  tft->println("..waiting..");*/
            //tft->printf("Tempo bpm: %i\n", (int)taptempotracker->temp_bpm);

            //tft->printf("B-tick=%i  T-tick=%i\n", ticks % PPQN, taptempotracker->tap_ticks);

            //tft->printf("Tap tick duration: %i\n", taptempotracker->tap_tick_duration);


            float height = 20.0;
            float last_value = 0.0;
            float conv = 2000.0 / tft->width();
            //tft->printf("ms per pixel = %i\n", conv);

            pos.y = tft->getCursorY();
            for (int i = 0 ; i < tft->width() ; i++) {
                // first to be rendered should always be the oldest entry
                // last to be rendered should always be the newest entry
                float v = taptempotracker->get_internal_phase_for_x_ago(i*conv);
                tft->drawLine(
                    (i), 
                    pos.y + last_value, 
                    (i+1), 
                    pos.y + (v * height), 
                    C_WHITE
                );

                last_value = v * height;
            }
            pos.y += height + 5;

            //pos.y = tft->getCursorY();
            for (int i = 0 ; i < tft->width() ; i++) {
                // first to be rendered should always be the oldest entry
                // last to be rendered should always be the newest entry
                float v = taptempotracker->get_tap_phase_for_x_ago(i*conv);
                tft->drawLine(
                    (i), 
                    pos.y + last_value, 
                    (i+1), 
                    pos.y + (v * height), 
                    C_WHITE
                );

                last_value = v * height;
            }
            pos.y += height + 5;

            tft->setCursor(0, pos.y);

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