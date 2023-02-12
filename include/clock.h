#include "bpm.h"
//#include "midi/midi_outs.h"

#define CLOCK_INTERNAL          0
#define CLOCK_EXTERNAL_USB_HOST 1
#define CLOCK_NONE              2
#define NUM_CLOCK_SOURCES       3

#ifndef DEFAULT_CLOCK_MODE
  #define DEFAULT_CLOCK_MODE CLOCK_INTERNAL
#endif

extern int clock_mode;

/// use cheapclock clock
extern uint32_t last_ticked_at_micros;
void setup_cheapclock();

void pc_usb_midi_handle_clock();
bool check_and_unset_pc_usb_midi_clock_ticked();

bool update_clock_ticks();
