// menuitem to show some information about what's going on in uClock internals, to try and aid debugging

#include "menuitems.h"
#include "midi_helpers.h"

class UclockDebugPanel : public MenuItem {
    public:
        UclockDebugPanel() : MenuItem("uClock debug") {
            this->selectable = false;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x,pos.y);
            tft->print("uClock debug info:");
            tft->setTextSize(1);
            #ifdef UCLOCK_DEBUG_LOGGING
                tft->print(" (debug logging enabled)");
            #endif
            tft->println();

            tft->printf("state: %i\n", uClock.clock_state);
            tft->printf("tick: %lu\n", uClock.tick);
            tft->printf("int_clock_tick: %lu\n", uClock.int_clock_tick);
            tft->printf("ext_clock_tick: %lu\n", uClock.ext_clock_tick);
            tft->printf("ext_clock_us: %lu\n", uClock.ext_clock_us);
            tft->printf("ext_interval: %lu\n", uClock.ext_interval);
            tft->printf("external_tempo: %lu\n", uClock.external_tempo);

            for (uint8_t i=0; i < uClock.ext_interval_buffer_size; i++) {
                tft->printf("  ext interval buffer[%i]: %lu\n", i, uClock.ext_interval_buffer[i]);
            }

            return tft->getCursorY();
        }
};