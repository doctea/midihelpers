#ifndef MENU_ENVELOPEGRAPH_VIEW_MENUITEMS__INCLUDED
#define MENU_ENVELOPEGRAPH_VIEW_MENUITEMS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

//#include "../outputs/envelopes.h"

#include <LinkedList.h>

#ifndef PARAMETER_INPUT_GRAPH_HEIGHT
    #define PARAMETER_INPUT_GRAPH_HEIGHT 50
#endif

#include "bpm.h"    // because we need to know the current ticks

//template<unsigned long memory_size>
class EnvelopeDisplay : public MenuItem {
    //static constexpr
    int16_t stage_colours[7] = {
        (int16_t)0x8080,
        (int16_t)GREEN,
        (int16_t)YELLOW,
        (int16_t)ORANGE,
        (int16_t)RED,
        (int16_t)PURPLE,
        (int16_t)BLUE
    };

    public:
        EnvelopeBase *envelope = nullptr;

        EnvelopeDisplay(const char *label, EnvelopeBase *envelope) : MenuItem(label, false) {
            this->envelope = envelope;
        }

        virtual void configure(EnvelopeBase *envelope) {
            this->envelope = envelope;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setTextSize(0);

            pos.y = tft->getCursorY();

            envelope->recalculate_graph_if_necessary();

            const uint16_t base_row = pos.y;

            // todo: see if we can omptimise this by drawing in the "direction" of the framebuffer axis (ie, horizontal lines instead of vertical, from the point of view of the framebuffer)
            int last_y = 0;
            for (int screen_x = 0 ; screen_x < tft->width() ; screen_x++) {
                const uint16_t tick_for_screen_X = screen_x;
                const float value = (float)envelope->graph[tick_for_screen_X].value;
                const stage_t stage = envelope->graph[tick_for_screen_X].stage;
                const int y = PARAMETER_INPUT_GRAPH_HEIGHT - (value * PARAMETER_INPUT_GRAPH_HEIGHT);
                if (screen_x != 0) {
                    //int last_y = GRAPH_HEIGHT - (this->logged[tick_for_screen_X] * GRAPH_HEIGHT);
                    //actual->drawLine(screen_x-1, base_row + last_y, screen_x, base_row + y, YELLOW);                    
                    tft->drawLine(screen_x-1, base_row + last_y, screen_x, base_row + y, stage_colours[stage]); //parameter_input->colour);                    
                    if (envelope->is_invert())
                        tft->drawLine(screen_x, base_row,     screen_x, base_row + y, stage_colours[stage]);
                    else
                        tft->drawLine(screen_x, base_row + y, screen_x, base_row + PARAMETER_INPUT_GRAPH_HEIGHT, stage_colours[stage]);
                }
                //actual->drawFastHLine(screen_x, base_row + y, 1, GREEN);
                last_y = y;
            }

            // draw a horizontal line representing the current envelope level
            if (envelope->last_state.stage!=0) {
                const int y = envelope->is_invert() ? 
                    (envelope->last_state.lvl_now * PARAMETER_INPUT_GRAPH_HEIGHT)
                    :
                    PARAMETER_INPUT_GRAPH_HEIGHT - (envelope->last_state.lvl_now * PARAMETER_INPUT_GRAPH_HEIGHT);
                tft->drawLine(0, base_row + y, tft->width(), base_row + y, stage_colours[envelope->last_state.stage]);
            }

            tft->setCursor(pos.x, pos.y + PARAMETER_INPUT_GRAPH_HEIGHT + 5);    // set cursor to below the graph's output

            //if (this->parameter_input!=nullptr && this->parameter_input->hasExtra())
            //    tft->printf((char*)"Extra: %s\n", (char*)this->parameter_input->getExtra());

            return tft->getCursorY();
        }
};

class EnvelopeIndicator : public MenuItem {
    public:
    EnvelopeBase *envelope = nullptr;

    //static constexpr 
    const char *stage_labels[6] = {
        "Off",
        "Attack",
        "Hold",
        "Decay",
        "Sustain",
        "Release"
    };

    EnvelopeIndicator(const char *label, EnvelopeBase *envelope) : MenuItem(label, false) {
        this->envelope = envelope;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        tft->printf("%s | ", (char*)envelope->label);
        //tft->printf("   CC: %i\n", envelope->midi_cc);
        tft->printf("Stg: %7s | ", (char*)stage_labels[envelope->last_state.stage]);
        //tft->printf("Trig'd at: %-5i | ", envelope->stage_triggered_at);
        //tft->printf("Elapsed: %-5i\n", envelope->last_state.elapsed);
        char buf[5];
        snprintf(buf, 10, "%-1.2f", envelope->last_sent_actual_lvl);
        tft->printf("Lvl: %s\n", buf);
        
        return tft->getCursorY();
    }
};

#endif
