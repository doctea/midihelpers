// arranger_test_stubs.cpp
// Defines all globals needed by saveloadlib and bpm.h for native test builds.
// Placed in test/native/ so PlatformIO compiles it alongside test_arranger.cpp.

#include "Arduino.h"
#include "saveloadlib.h"
// bpm.h globals come from the real include/bpm.h (via stub forwarder)
#include "bpm.h"

// ── bpm globals ────────────────────────────────────────────────────────────
volatile uint32_t ticks           = 0;
uint32_t          ts_phase_offset = 0;

#ifdef ENABLE_TIME_SIGNATURE
time_sig_t current_time_signature = {
    DEFAULT_TIME_SIGNATURE_NUMERATOR,
    DEFAULT_TIME_SIGNATURE_DENOMINATOR
};
#endif

// ── saveloadlib globals ─────────────────────────────────────────────────────
SerialStub Serial;

const char* warning_label = " - WARNING: no target nor getter func!";
char linebuf[SL_MAX_LINE] = {0};

ISaveableSettingHost*  SL_ROOT              = nullptr;
SL_ArenaBase*          sl_setting_arena     = nullptr;
SL_TreeCounts          sl_cached_tree_counts = {0, 0, 0};
bool                   sl_tree_counts_valid  = false;
char                   sl_seg_pool[SL_SEG_POOL_SIZE];
uint16_t               sl_seg_pool_used      = 0;
