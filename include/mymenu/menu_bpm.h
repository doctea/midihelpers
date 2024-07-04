//#include "Config.h"
#ifdef ENABLE_SCREEN

#include "menu.h"
#include "menuitems.h"
#include "../bpm.h"

#include "../clock.h"

class LoopMarkerPanel : public PinnedPanelMenuItem {
    unsigned long loop_length;
    int beats_per_bar = BEATS_PER_BAR;
    int bars_per_phrase = BEATS_PER_BAR * BARS_PER_PHRASE;
    int ppqn;

    public:
        LoopMarkerPanel(int loop_length, int ppqn, int beats_per_bar = BEATS_PER_BAR, int bars_per_phrase = BARS_PER_PHRASE) : PinnedPanelMenuItem("Loop Position Header") {
            this->loop_length = loop_length;
            this->beats_per_bar = beats_per_bar;
            this->bars_per_phrase = bars_per_phrase;
            this->ppqn = ppqn;
        };

        void set_loop_length(unsigned long loop_length) {
            this->loop_length = loop_length;
        }
        void set_beats_per_bar(unsigned long beats_per_bar) {
            this->beats_per_bar = beats_per_bar;
        }        

        virtual int display(Coord pos, bool selected = false, bool opened = false) override {
            //Serial.printf("PinnedPanel display colour RED is %4x, WHITE is %4x\n", RED, C_WHITE);

            static const int bar_height = tft->getRowHeight();

            tft->setTextColor(this->default_fg, this->default_bg);
            //tft.setCursor(pos.x,pos.y);
            //int LOOP_LENGTH = PPQN * BEATS_PER_BAR * BARS_PER_PHRASE;
            unsigned int y = pos.y; //0;
            static uint_fast32_t last_serviced_tick;
            static uint_fast16_t last_position_width;

            static uint16_t tick_of_loop = 0;
            static uint_fast16_t new_position_width = 0;

            static uint_fast8_t bar_height_third = bar_height/3;

            static uint_fast16_t tft_width = tft->width();

            // save some float maths by only recalculating if tick is different from last time
            if (last_serviced_tick != ticks) {
                tick_of_loop = ticks % loop_length;
                float percent = float(tick_of_loop) / (float)loop_length;
                new_position_width = (percent*(float)tft_width);
                //Serial.printf("ticks %i: ticks%loop_length = %i: ", ticks, ticks%loop_length);
                //if (ticks%loop_length==0)   // if we're at the start of loop then blank out the display 
                if (new_position_width < last_position_width){
                    //Serial.println("so drawing black?");
                    tft->fillRect(0, y, tft_width, bar_height, this->default_bg);
                } 
                last_position_width = new_position_width;
            }
            tft->fillRect(0, y, last_position_width, bar_height, playing ? DARK_BLUE : RED);

            static uint_fast16_t step_size_beats = tft_width / (beats_per_bar*bars_per_phrase);  // safe to make static so long as beats_per_bar/bars_per_phrase is not configurable!
            for (uint_fast16_t i = 0 ; i < tft_width ; i += step_size_beats) {
                //tft->drawLine(i, y, i, y+(bar_height_third), i > new_position_width ? C_WHITE : BLACK);
                tft->drawLine(i, y, i, y+(bar_height_third), C_WHITE);
            }

            static uint_fast16_t step_size_bars = tft_width / bars_per_phrase;
            for (uint_fast16_t i = 0 ; i < tft_width ; i += step_size_bars) {
                //tft->fillRect(i, y+1, bar_height_third, bar_height-1, i > new_position_width ? C_WHITE : BLACK);
                tft->fillRect(i, y+1, bar_height_third, bar_height-1, C_WHITE);
            }

            //Serial.printf("percent %f, width %i\n", percent, tft->width());
            y += bar_height;
            return y;
        }
        //#endif
};

// BPM indicator
class BPMPositionIndicator : public MenuItem {
    public:
        BPMPositionIndicator() : MenuItem("position") {};

        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.printf("positionindicator display for %s\n", label);
            tft->setCursor(pos.x,pos.y);
            header("position", pos, selected, opened);
            tft->setTextSize(2);
            if (playing) {
                colours(opened, GREEN,   BLACK);
            } else {
                colours(opened, RED,     BLACK);
            }
            if (clock_mode==CLOCK_INTERNAL) {
                tft->printf((char*)"%04i:%02i:%02i @ %03.2f\n", 
                    BPM_CURRENT_PHRASE + 1, 
                    BPM_CURRENT_BAR_OF_PHRASE + 1,
                    BPM_CURRENT_BEAT_OF_BAR + 1,
                    bpm_current
                );
            } else if (clock_mode==CLOCK_EXTERNAL_USB_HOST) {
                tft->printf((char*)"%04i:%02i:%02i\n",
                    BPM_CURRENT_PHRASE + 1, 
                    BPM_CURRENT_BAR_OF_PHRASE + 1,
                    BPM_CURRENT_BEAT_OF_BAR + 1
                );
                tft->setTextSize(1);
                tft->println((char*)"from External USB Host");
            } else {
                tft->printf((char*)"%04i:%02i:%02i\n",
                    BPM_CURRENT_PHRASE + 1, 
                    BPM_CURRENT_BAR_OF_PHRASE + 1,
                    BPM_CURRENT_BEAT_OF_BAR + 1
                );
            }

            return tft->getCursorY();
        }

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
};


#endif