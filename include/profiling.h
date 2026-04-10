/**
 * profiling.h — Lightweight per-slot timing for MCU hot-path profiling.
 *
 * Usage:
 *   1. Define ENABLE_PROFILING in build_flags to activate.
 *      Without it, all macros compile to nothing.
 *
 *   2. Declare a slot:
 *        a) File scope (ISR-safe — no guard-variable race on RP2040):
 *             PROFILE_SLOT_DECL(my_slot, "label");
 *        b) Inside a function (first-call init — avoid for ISR-called fns):
 *             PROFILE_SLOT(my_slot, "label");
 *
 *   3. Time something:
 *        void my_fn() { PROFILE_SCOPE(my_slot); ... }      // RAII
 *        PROFILE_START(my_slot);  work();  PROFILE_STOP(my_slot);  // manual
 *
 *   4. Spike detection — set a threshold (µs); samples above it are counted
 *      and the most recent is captured with its tick value:
 *        PROFILE_SET_SPIKE_THRESHOLD(my_slot, 3000);  // e.g. in setup()
 *      The current tick is read from the global _profile_tick; update it by
 *      calling PROFILE_SET_TICK(ticks) at the start of your tick handler.
 *
 *   5. Dump / reset:
 *        profile_print_all(Serial);
 *        profile_reset_all();
 *
 * Output format per slot:
 *   [label                 ] n=NNN  avg=XXXus  min=XXXus  max=XXXus  total=XXXms
 *     hist: <=100:N  <=500:N  <=2ms:N  <=5ms:N  <=10ms:N  >10ms:N
 *     spikes >Xus: N  last: Yus @ tick Z
 *
 * Notes:
 *   - Uses micros() — 1 µs resolution, works in ISR context on RP2040/RP2350.
 *   - Requires C++17 (inline variables).
 *   - NOT thread-safe across cores; results are approximate if a slot is
 *     written from both cores.
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <limits.h>

#ifdef ENABLE_PROFILING

// ─── Histogram bucket boundaries (µs) — 5 thresholds → 6 buckets ────────────
// Bucket 0 : elapsed <= 100 µs
// Bucket 1 :          <= 500 µs
// Bucket 2 :          <= 2000 µs
// Bucket 3 :          <= 5000 µs
// Bucket 4 :          <= 10000 µs
// Bucket 5 :          >  10000 µs
#define PROFILE_NUM_BUCKETS   6
#define PROFILE_HIST_BOUND_0  100u
#define PROFILE_HIST_BOUND_1  500u
#define PROFILE_HIST_BOUND_2  2000u
#define PROFILE_HIST_BOUND_3  5000u
#define PROFILE_HIST_BOUND_4  10000u

// ─── Global tick context ─────────────────────────────────────────────────────
// Set this at the start of your tick handler so spikes can be correlated with
// clock position:  PROFILE_SET_TICK(ticks);
inline uint32_t _profile_tick = 0;
#define PROFILE_SET_TICK(t)   (_profile_tick = (uint32_t)(t))

// ─── Spike history ring buffer size ──────────────────────────────────────────
// Stores the N most recent spike events per slot.  Increase for more context;
// decrease to save RAM (each entry is 8 bytes, default = 4 entries = 32 bytes/slot).
#ifndef PROFILE_SPIKE_HISTORY
#define PROFILE_SPIKE_HISTORY 4
#endif

// ─── Slot struct ─────────────────────────────────────────────────────────────

struct ProfileSlot {
    const char* name;

    // Basic stats
    uint32_t count   = 0;
    uint32_t sum_us  = 0;
    uint32_t min_us  = UINT32_MAX;
    uint32_t max_us  = 0;
    uint32_t _start  = 0;

    // Histogram
    uint32_t hist[PROFILE_NUM_BUCKETS] = {};

    // Spike history ring buffer — only active when spike_threshold_us > 0.
    // Stores the PROFILE_SPIKE_HISTORY most recent spikes; oldest entry is
    // overwritten when the buffer is full.
    // spike_modulo: if non-zero, each entry also stores tick % spike_modulo so
    // you can see which phase of a clock period the spike lands on.
    // Examples for PPQN=24:
    //   spike_modulo =  6  →  16th-note offset (0..5)
    //   spike_modulo = 24  →  beat offset     (0..23)
    //   spike_modulo = 96  →  bar offset      (0..95)
    //   spike_modulo = 384 →  phrase offset   (0..383)
    struct SpikeEntry {
        uint32_t elapsed_us;
        uint32_t tick;
    };
    uint32_t  spike_threshold_us = 0;
    uint32_t  spike_modulo       = 0;   // set via PROFILE_SET_SPIKE_MODULO()
    uint32_t  spike_count        = 0;
    SpikeEntry spike_history[PROFILE_SPIKE_HISTORY] = {};
    uint8_t   spike_history_head = 0;   // next write position (ring)

    inline void start() {
        _start = micros();
    }

    inline void stop() {
        const uint32_t elapsed = micros() - _start;
        sum_us += elapsed;
        count++;
        if (elapsed < min_us) min_us = elapsed;
        if (elapsed > max_us) max_us = elapsed;

        // Histogram bucket
        uint8_t b;
        if      (elapsed <= PROFILE_HIST_BOUND_0) b = 0;
        else if (elapsed <= PROFILE_HIST_BOUND_1) b = 1;
        else if (elapsed <= PROFILE_HIST_BOUND_2) b = 2;
        else if (elapsed <= PROFILE_HIST_BOUND_3) b = 3;
        else if (elapsed <= PROFILE_HIST_BOUND_4) b = 4;
        else                                       b = 5;
        hist[b]++;

        // Spike history
        if (spike_threshold_us > 0 && elapsed >= spike_threshold_us) {
            spike_history[spike_history_head] = { elapsed, _profile_tick };
            spike_history_head = (spike_history_head + 1) % PROFILE_SPIKE_HISTORY;
            spike_count++;
        }
    }

    void reset() {
        count = 0; sum_us = 0; max_us = 0;
        min_us = UINT32_MAX;
        for (uint8_t i = 0; i < PROFILE_NUM_BUCKETS; i++) hist[i] = 0;
        spike_count = 0;
        spike_history_head = 0;
        for (uint8_t i = 0; i < PROFILE_SPIKE_HISTORY; i++)
            spike_history[i] = {0, 0};
        // spike_threshold_us and spike_modulo are config — NOT reset
    }

    void print(Print& out) const {
        if (!count) {
            out.printf("[%-24s] no data\n", name);
            return;
        }
        out.printf(
            "[%-24s] n=%-7lu  avg=%5luus  min=%5luus  max=%6luus  total=%lums\n",
            name,
            (unsigned long)count,
            (unsigned long)(sum_us / count),
            (unsigned long)(min_us == UINT32_MAX ? 0u : min_us),
            (unsigned long)max_us,
            (unsigned long)(sum_us / 1000)
        );
        out.printf(
            "  hist: <=%uus:%-5lu  <=%uus:%-5lu  <=%uus:%-5lu  <=%uus:%-5lu  <=%uus:%-5lu  >%uus:%-5lu\n",
            PROFILE_HIST_BOUND_0, (unsigned long)hist[0],
            PROFILE_HIST_BOUND_1, (unsigned long)hist[1],
            PROFILE_HIST_BOUND_2, (unsigned long)hist[2],
            PROFILE_HIST_BOUND_3, (unsigned long)hist[3],
            PROFILE_HIST_BOUND_4, (unsigned long)hist[4],
            PROFILE_HIST_BOUND_4, (unsigned long)hist[5]
        );
        if (spike_threshold_us == 0) return;

        out.printf("  spikes >%luus: %lu\n",
            (unsigned long)spike_threshold_us,
            (unsigned long)spike_count);

        if (spike_count == 0) return;

        // Print ring buffer oldest-first.
        // When fewer than PROFILE_SPIKE_HISTORY spikes have been recorded,
        // entries before spike_history_head are still zero — skip them.
        const bool buffer_full = (spike_count >= PROFILE_SPIKE_HISTORY);
        const uint8_t start_idx = buffer_full ? spike_history_head : 0;
        const uint8_t entries   = buffer_full ? PROFILE_SPIKE_HISTORY
                                              : (uint8_t)spike_count;
        out.print("  recent spikes (oldest->newest):");
        uint32_t prev_tick = 0;
        for (uint8_t i = 0; i < entries; i++) {
            const uint8_t idx = (start_idx + i) % PROFILE_SPIKE_HISTORY;
            const SpikeEntry& e = spike_history[idx];
            if (spike_modulo > 0)
                out.printf("  %luus@t%lu(%%%lu=%lu)",
                    (unsigned long)e.elapsed_us,
                    (unsigned long)e.tick,
                    (unsigned long)spike_modulo,
                    (unsigned long)(e.tick % spike_modulo));
            else
                out.printf("  %luus@t%lu",
                    (unsigned long)e.elapsed_us,
                    (unsigned long)e.tick);
            if (i > 0 && prev_tick > 0)
                out.printf("(+%lu)", (unsigned long)(e.tick - prev_tick));
            prev_tick = e.tick;
        }
        out.println();
    }
};

// ─── Global registry ─────────────────────────────────────────────────────────

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

/** File-scope slot — ISR-safe (no function-local static guard race). */
#define PROFILE_SLOT_DECL(var, slot_name) \
    static ProfileSlot var = { slot_name }; \
    static bool _preg_##var = _prof_register(&var)

/** Function-local static slot — fine for non-ISR paths. */
#define PROFILE_SLOT(var, slot_name) \
    static ProfileSlot var = { slot_name }; \
    static bool _preg_##var = (_prof_register(&var), true)

/** Manual start/stop. */
#define PROFILE_START(var)  (var).start()
#define PROFILE_STOP(var)   (var).stop()

/** Set spike detection threshold on a slot (µs). */
#define PROFILE_SET_SPIKE_THRESHOLD(var, us)   ((var).spike_threshold_us = (us))

/** Set the clock modulo printed alongside each spike tick.
 *  E.g. PROFILE_SET_SPIKE_MODULO(p_seq, 96) prints tick%96 (bar phase at PPQN=24).
 *  Set to 0 to disable. */
#define PROFILE_SET_SPIKE_MODULO(var, n)       ((var).spike_modulo = (uint32_t)(n))

/** RAII scope timer.  Slot must already be in scope. */
struct _ProfileScope {
    ProfileSlot& _s;
    explicit _ProfileScope(ProfileSlot& s) : _s(s) { _s.start(); }
    ~_ProfileScope()                                { _s.stop();  }
};
#define PROFILE_SCOPE(var) _ProfileScope _pscope_##var(var)

#else // ── ENABLE_PROFILING not defined — everything is a no-op ──────────────

#define PROFILE_SET_TICK(t)                     ((void)0)
#define PROFILE_SLOT_DECL(var, name)
#define PROFILE_SLOT(var, name)
#define PROFILE_START(var)                      ((void)0)
#define PROFILE_STOP(var)                       ((void)0)
#define PROFILE_SET_SPIKE_THRESHOLD(var, us)    ((void)0)
#define PROFILE_SET_SPIKE_MODULO(var, n)        ((void)0)
#define PROFILE_SCOPE(var)                      ((void)0)

struct ProfileSlot { void print(Print&) const {} void reset() {} };

inline void profile_print_all(Print&) {}
inline void profile_reset_all() {}

#endif // ENABLE_PROFILING
