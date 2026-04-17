// conductor.h
// Central coordinator for timekeeping (clock, BPM, time signature) and
// harmonic context (key, scale, quantisation, global chord).
//
// Wrap this with #define ENABLE_CONDUCTOR in your project config.
//
// Persistence: when ENABLE_STORAGE is defined, Conductor inherits from
// SHDynamic and registers its settings so they can be saved/loaded via
// saveloadlib. Settings scopes:
//   SL_SCOPE_SYSTEM  — clock_mode
//   SL_SCOPE_PROJECT — bpm, time_signature, scale root+type, chord identity
#pragma once

#include "bpm.h"
#include "clock.h"

#ifdef ENABLE_SCALES
    #include "scales.h"
#endif

#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
#endif

// Forward declaration for create_menu_items. Full types resolved in menu_conductor.cpp.
#ifdef ENABLE_SCREEN
    class Menu;

    enum conductor_menu_options_t {
        COMBINE_NONE = 0,
        COMBINE_MUSE = 1 << 0,  // combine time signature and scale settings into a single "Muse" page
    };
#endif

class Conductor
#ifdef ENABLE_STORAGE
    : public SHDynamic<0, 10>
#endif
{
public:
    // ── Construction ────────────────────────────────────────────────────────
    Conductor() {
        #ifdef ENABLE_STORAGE
            this->set_path_segment("Conductor");
        #endif
    }

    // Call once from setup() after saveloadlib is initialised.
    // Registers itself as the global scale / chord identity target so that
    // the rest of the codebase can query scale/chord via the existing helpers.
    void begin() {
        #ifdef ENABLE_SCALES
            set_global_scale_identity_target(&_scale_identity);
            set_global_chord_identity_target(&_chord_identity);
        #endif
    }

    // ── Clock source ────────────────────────────────────────────────────────
    ClockMode get_clock_mode() const { return clock_mode; }
    void set_clock_mode(ClockMode m) { change_clock_mode(m); }

    // Cast helpers for saveloadlib (stores as int)
    int get_clock_mode_int() const { return (int)clock_mode; }
    void set_clock_mode_int(int m) { change_clock_mode((ClockMode)m); }

    // ── BPM ─────────────────────────────────────────────────────────────────
    float get_bpm_value() const { return get_bpm(); }
    void  set_bpm_value(float v) { set_bpm(v); }

    // ── Playback state ───────────────────────────────────────────────────────
    bool is_playing() const { return playing; }
    void start()    { clock_start(); }
    void stop()     { clock_stop(); }
    void continue_play() { clock_continue(); }

    // ── Song position (ticks) ───────────────────────────────────────────────
    uint32_t get_ticks() const { return ticks; }

    // ── Time signature ──────────────────────────────────────────────────────
    #ifdef ENABLE_TIME_SIGNATURE
        uint8_t get_numerator()   const { return get_time_signature_numerator(); }
        uint8_t get_denominator() const { return get_time_signature_denominator(); }
        void set_numerator(uint8_t v)   { set_time_signature_numerator(v); }
        void set_denominator(uint8_t v) { set_time_signature_denominator(v); }

        // int wrappers for saveloadlib
        int get_numerator_int()   const { return (int)get_time_signature_numerator(); }
        int get_denominator_int() const { return (int)get_time_signature_denominator(); }
        void set_numerator_int(int v)   { set_time_signature_numerator((uint8_t)v); }
        void set_denominator_int(int v) { set_time_signature_denominator((uint8_t)v); }
    #endif

    // ── Global key / scale ──────────────────────────────────────────────────
    #ifdef ENABLE_SCALES
        int8_t get_scale_root() const { return _scale_identity.root_note; }
        void   set_scale_root(int8_t r) { _scale_identity.root_note = r; }

        scale_index_t get_scale_type() const { return _scale_identity.scale_number; }
        void          set_scale_type(scale_index_t t) { _scale_identity.scale_number = t; }

        const scale_identity_t& get_scale_identity() const { return _scale_identity; }

        // ── Global chord ─────────────────────────────────────────────────────
        int8_t       get_chord_degree()    const { return _chord_identity.degree; }
        CHORD::Type  get_chord_type()      const { return _chord_identity.type; }
        int8_t       get_chord_inversion() const { return _chord_identity.inversion; }

        void set_chord_degree(int8_t d)        { _chord_identity.degree    = d; }
        void set_chord_type(CHORD::Type t)     { _chord_identity.type      = t; }
        void set_chord_inversion(int8_t inv)   { _chord_identity.inversion = inv; }

        const chord_identity_t& get_chord_identity() const { return _chord_identity; }
        void set_chord_identity(const chord_identity_t& c) { _chord_identity = c; }

        // Convenience: quantise a pitch to the current global scale
        int8_t quantise_to_scale(int8_t pitch) const {
            return quantise_pitch_to_scale(pitch, _scale_identity.root_note, _scale_identity.scale_number);
        }

        // Convenience: quantise a pitch to the current global chord
        int8_t quantise_to_chord(int8_t pitch, int8_t range = 0) const {
            return quantise_pitch_to_chord(pitch, range,
                _scale_identity.root_note, _scale_identity.scale_number, _chord_identity);
        }
    #endif // ENABLE_SCALES

    // ── Menu integration ────────────────────────────────────────────────────
    // Call from setup_menu() after including mymenu/menu_conductor.h.
    // Defined in src/menu_conductor.cpp.
    #ifdef ENABLE_SCREEN
        void make_menu_items(Menu *menu, conductor_menu_options_t options = conductor_menu_options_t::COMBINE_NONE);
    #endif // ENABLE_SCREEN

    // ── saveloadlib integration ─────────────────────────────────────────────
    #ifdef ENABLE_STORAGE
        void setup_saveable_settings() override {
            if (saveable_settings_setup) return;
            saveable_settings_setup = true;

            // BPM — project scope
            register_setting(
                new LSaveableSetting<float>(
                    "bpm", "Conductor", nullptr,
                    [this](float v) { set_bpm_value(v); },
                    [this]() -> float { return get_bpm_value(); }
                ),
                SL_SCOPE_PROJECT
            );

            // Clock mode — system scope (survives project changes)
            register_setting(
                new LSaveableSetting<int>(
                    "clock_mode", "Conductor", nullptr,
                    [this](int v) { set_clock_mode_int(v); },
                    [this]() -> int { return get_clock_mode_int(); }
                ),
                SL_SCOPE_SYSTEM
            );

            #ifdef ENABLE_TIME_SIGNATURE
                register_setting(
                    new LSaveableSetting<int>(
                        "time_num", "Conductor", nullptr,
                        [this](int v) { set_numerator_int(v); },
                        [this]() -> int { return get_numerator_int(); }
                    ),
                    SL_SCOPE_PROJECT | SL_SCOPE_SCENE
                );
                register_setting(
                    new LSaveableSetting<int>(
                        "time_den", "Conductor", nullptr,
                        [this](int v) { set_denominator_int(v); },
                        [this]() -> int { return get_denominator_int(); }
                    ),
                    SL_SCOPE_PROJECT | SL_SCOPE_SCENE
                );
            #endif

            #ifdef ENABLE_SCALES
                register_setting(
                    new LSaveableSetting<int>(
                        "scale_root", "Conductor", nullptr,
                        [this](int v) { set_scale_root((int8_t)v); },
                        [this]() -> int { return (int)get_scale_root(); }
                    ),
                    SL_SCOPE_PROJECT
                );
                register_setting(
                    new LSaveableSetting<int>(
                        "scale_type", "Conductor", nullptr,
                        [this](int v) { set_scale_type((scale_index_t)v); },
                        [this]() -> int { return (int)get_scale_type(); }
                    ),
                    SL_SCOPE_PROJECT
                );
                register_setting(
                    new LSaveableSetting<int>(
                        "chord_degree", "Conductor", nullptr,
                        [this](int v) { set_chord_degree((int8_t)v); },
                        [this]() -> int { return (int)get_chord_degree(); }
                    ),
                    SL_SCOPE_PROJECT
                );
                register_setting(
                    new LSaveableSetting<int>(
                        "chord_type", "Conductor", nullptr,
                        [this](int v) { set_chord_type((CHORD::Type)v); },
                        [this]() -> int { return (int)get_chord_type(); }
                    ),
                    SL_SCOPE_PROJECT
                );
                register_setting(
                    new LSaveableSetting<int>(
                        "chord_inversion", "Conductor", nullptr,
                        [this](int v) { set_chord_inversion((int8_t)v); },
                        [this]() -> int { return (int)get_chord_inversion(); }
                    ),
                    SL_SCOPE_PROJECT
                );
            #endif // ENABLE_SCALES
        }
    #endif // ENABLE_STORAGE

private:
    #ifdef ENABLE_SCALES
        scale_identity_t _scale_identity;
        chord_identity_t _chord_identity;
    #endif
};

// Global singleton instance — define in exactly one .cpp:
//   #include "conductor.h"
//   Conductor *conductor = nullptr;
//
// Then declare extern in other files:
//   extern Conductor *conductor;
extern Conductor *conductor;
