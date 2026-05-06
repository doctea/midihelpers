// Stub definitions for native test builds.
// Provides the global variables declared as extern in bpm.h.
#define ENABLE_TIME_SIGNATURE
#include "bpm.h"  // forwarder → include/bpm.h
#include "Arduino.h"

volatile uint32_t ticks            = 0;
uint32_t          ts_phase_offset  = 0;
SerialStub        Serial;

#ifdef ENABLE_TIME_SIGNATURE
time_sig_t current_time_signature = {
    DEFAULT_TIME_SIGNATURE_NUMERATOR,
    DEFAULT_TIME_SIGNATURE_DENOMINATOR
};
#endif
