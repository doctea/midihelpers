/**
 * profiling.h — Lightweight per-slot timing for MCU hot-path profiling.
 *
 * Usage:
 *   1. Define #define ENABLE_PROFILING before including (in platformio.ini build_flags,
 *      or in a project config header) to activate.  Without it, all macros compile away.
 *
 *   2. Declare a slot (choose one):
 *        a) At file scope (preferred for interrupt-called functions on RP2040):
 *             PROFILE_SLOT_DECL(my_slot, "label");
 *        b) Inside a function (initialises on first call — avoid for ISR-called fns):
 *             PROFILE_SLOT(my_slot, "label");
 *
 *   3. Measure a whole function with RAII:
 *        void my_fn() { PROFILE_SCOPE(my_slot); ... }
 *
 *      Or measure a sub-region manually:
 *        PROFILE_START(my_slot);
 *        expensive_thing();
 *        PROFILE_STOP(my_slot);
 *
 *   4. Dump all registered slots to Serial (or any Print&):
 *        profile_print_all(Serial);
 *
 *      Reset all counters:
 *        profile_reset_all();
 *
 * Notes:
 *   - Uses micros() — 1 µs resolution, works in ISR context on RP2040/RP2350.
 *   - Requires C++17 (inline variables).  All supported targets (RP2040, RP2350,
 *     Teensy 4.1 with PlatformIO) compile with gnu++17.
 *   - NOT thread-safe across cores.  If a slot is written from both Core 0 and Core 1,
 *     results will be approximate.  For now all hot paths live on Core 0.
 */
#pragma once

// Always include Arduino.h so Print is available in both the enabled and
// disabled branch (avoids "incomplete type" errors in IDEs / static analysis).
#include <Arduino.h>
#include <stdint.h>
#include <limits.h>

#ifdef ENABLE_PROFILING

// ─── Slot struct ─────────────────────────────────────────────────────────────

struct ProfileSlot {
    const char* name;
    uint32_t    count   = 0;
    uint32_t    sum_us  = 0;
    uint32_t    min_us  = UINT32_MAX;
    uint32_t    max_us  = 0;
    uint32_t    _start  = 0;

    inline void start() {
        _start = micros();
    }

    inline void stop() {
        uint32_t elapsed = micros() - _start;
        sum_us += elapsed;
        count++;
        if (elapsed < min_us) min_us = elapsed;
        if (elapsed > max_us) max_us = elapsed;
    }

    void reset() {
        count = 0; sum_us = 0; max_us = 0;
        min_us = UINT32_MAX;
    }

    void print(Print& out) const {
        if (!count) {
            out.printf("[%-24s] no data\n", name);
            return;
        }
        out.printf("[%-24s] n=%-7lu  avg=%5luus  min=%5luus  max=%6luus  total=%lums\n",
            name,
            (unsigned long)count,
            (unsigned long)(sum_us / count),
            (unsigned long)(min_us == UINT32_MAX ? 0 : min_us),
            (unsigned long)max_us,
            (unsigned long)(sum_us / 1000)
        );
    }
};

// ─── Global registry (C++17 inline variables — one instance across all TUs) ──

#ifndef MAX_PROFILE_SLOTS
#define MAX_PROFILE_SLOTS 32
#endif

inline ProfileSlot* _prof_registry[MAX_PROFILE_SLOTS] = {};
inline uint8_t      _prof_registry_count = 0;

inline bool _prof_register(ProfileSlot* slot) {
    if (_prof_registry_count < MAX_PROFILE_SLOTS)
        _prof_registry[_prof_registry_count++] = slot;
    return true;
}

inline void profile_print_all(Print& out) {
    out.println(F("=== Profile Report ==="));
    for (uint8_t i = 0; i < _prof_registry_count; i++)
        _prof_registry[i]->print(out);
    out.println(F("======================"));
}

inline void profile_reset_all() {
    for (uint8_t i = 0; i < _prof_registry_count; i++)
        _prof_registry[i]->reset();
}

// ─── Macros ───────────────────────────────────────────────────────────────────

/**
 * PROFILE_SLOT_DECL — declare + auto-register a slot at FILE scope.
 * Use this for functions called from interrupt context on RP2040, where
 * function-local static initialisation guards are unreliable.
 *
 * Place OUTSIDE any function body in the .cpp file that defines the function.
 */
#define PROFILE_SLOT_DECL(var, slot_name) \
    static ProfileSlot var = { slot_name }; \
    static bool _preg_##var = _prof_register(&var)

/**
 * PROFILE_SLOT — declare + auto-register a slot INSIDE a function.
 * Uses function-local statics (initialised on first call).
 * Safe for non-ISR functions; avoid for ISR-called functions on RP2040.
 */
#define PROFILE_SLOT(var, slot_name) \
    static ProfileSlot var = { slot_name }; \
    static bool _preg_##var = (_prof_register(&var), true)

/** Manual start/stop — when you only want to time part of a function. */
#define PROFILE_START(var)  (var).start()
#define PROFILE_STOP(var)   (var).stop()

/** RAII scope timer.  Slot must be in scope (declared at file scope or above). */
struct _ProfileScope {
    ProfileSlot& _s;
    explicit _ProfileScope(ProfileSlot& s) : _s(s) { _s.start(); }
    ~_ProfileScope()                                { _s.stop();  }
};

/**
 * PROFILE_SCOPE(var) — time from this statement to end of enclosing scope.
 * Slot `var` must already be declared (e.g. via PROFILE_SLOT_DECL at file scope).
 */
#define PROFILE_SCOPE(var) _ProfileScope _pscope_##var(var)

#else // ── ENABLE_PROFILING not defined ─────────────────────────────────────

#define PROFILE_SLOT_DECL(var, name)
#define PROFILE_SLOT(var, name)
#define PROFILE_START(var)      ((void)0)
#define PROFILE_STOP(var)       ((void)0)
#define PROFILE_SCOPE(var)      ((void)0)

struct ProfileSlot { void print(...) const {} void reset() {} };  // minimal stub

inline void profile_print_all(Print&) {}
inline void profile_reset_all() {}

#endif // ENABLE_PROFILING
