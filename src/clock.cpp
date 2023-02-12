#include "clock.h"
//#include "midi/midi_outs.h"

#ifndef CORE_TEENSY
    #define FLASHMEM
#endif

int clock_mode = DEFAULT_CLOCK_MODE;

/// use cheapclock clock
uint32_t last_ticked_at_micros = micros();
FLASHMEM void setup_cheapclock() {
    ticks = 0;
    set_bpm(bpm_current);
}

bool usb_midi_clock_ticked = false;
unsigned long last_usb_midi_clock_ticked_at;
void pc_usb_midi_handle_clock() {
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST && usb_midi_clock_ticked) {
        Serial.printf("WARNING: received a usb midi clock tick at %u, but last one from %u was not yet processed (didn't process within gap of %u)!\n", millis(), last_usb_midi_clock_ticked_at, millis()-last_usb_midi_clock_ticked_at);
    }
    /*if (CLOCK_EXTERNAL_USB_HOST) {  // TODO: figure out why tempo estimation isn't working and fix
        tap_tempo_tracker.push_beat();
    }*/
    last_usb_midi_clock_ticked_at = millis();
    usb_midi_clock_ticked = true;
}
bool check_and_unset_pc_usb_midi_clock_ticked() {
    bool v = usb_midi_clock_ticked;
    usb_midi_clock_ticked = false;
    /*if(clock_mode==CLOCK_EXTERNAL_USB_HOST && ticks%PPQN==0) {  // TODO: figure out why this isn't working and fix
        set_bpm(tap_tempo_tracker.bpm_calculate_current());
    }*/
    return v;
}

bool update_clock_ticks() {
    static unsigned long last_ticked = 0;
    if (clock_mode==CLOCK_EXTERNAL_USB_HOST && /*playing && */check_and_unset_pc_usb_midi_clock_ticked()) {
        ticks++;
        return true;
    } else if (clock_mode==CLOCK_INTERNAL && playing && micros() - last_ticked >= micros_per_tick) {
        ticks++;
        last_ticked = micros();
        if (is_bpm_on_beat(ticks)) {
            Serial.printf("beat %i!\n", ticks / PPQN);
            Serial.flush();
        }
        return true;
    }
    return false;
}