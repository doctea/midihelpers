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
//   SL_SCOPE_SCENE   — none currently, but could be used for song-section-specific settings like quantise on/off
#pragma once

#include "bpm.h"
#include "clock.h"

#ifdef ENABLE_SCALES
    #include "scales.h"
#endif

#include <LinkedList.h>
#include <functional-vlpp.h>

#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
#endif

// Forward declaration for create_menu_items. Full types resolved in menu_conductor.cpp.
#ifdef ENABLE_SCREEN
    class Menu;

    enum conductor_menu_options_t {
        COMBINE_NONE = 0,
        COMBINE_TIME_SIG_WITH_TRANSPORT = 1 << 0,  // combine time signature and scale settings into a single "Muse" page
        COMBINE_HARMONY_WITH_TIME_SIG = 1 << 1, // combine harmony and time signature settings into a single "Muse" page
        COMBINE_ALL = 0xFF
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
        #ifdef ENABLE_SCALES
            set_global_scale_identity_target(&this->global_scale_identity);
            set_global_chord_identity_target(&this->global_chord_identity);
        #endif
    }

    // ── Harmony-change notification ────────────────────────────────────────
    #ifdef ENABLE_SCALES
        // Callback signature: void(const scale_identity_t&, const chord_identity_t&)
        using harmony_change_cb_t = vl::Func<void(const scale_identity_t&, const chord_identity_t&)>;

        // Register a subscriber. Callback is fired whenever scale root/type,
        // chord degree/type/inversion, or quantise flags change.
        void register_harmony_change_callback(harmony_change_cb_t cb) {
            _harmony_callbacks.add(cb);
        }
    #endif // ENABLE_SCALES

    // ── Time-signature-change notification ────────────────────────────────
    #ifdef ENABLE_TIME_SIGNATURE
        // Callback signature: void(time_sig_t time_sig)
        using time_sig_change_cb_t = vl::Func<void(time_sig_t)>;

        // Register a subscriber. Callback is fired whenever numerator or
        // denominator changes to a new value.
        void register_time_sig_change_callback(time_sig_change_cb_t cb) {
            _time_sig_callbacks.add(cb);
        }
    #endif // ENABLE_TIME_SIGNATURE

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
        uint8_t get_numerator()   const { return ::get_time_signature_numerator(); }
        uint8_t get_denominator() const { return ::get_time_signature_denominator(); }
        void set_numerator(uint8_t v)   { set_time_signature({v, ::get_time_signature_denominator()}); }
        void set_denominator(uint8_t v) { set_time_signature({::get_time_signature_numerator(), v}); }
        void set_time_signature(time_sig_t time_sig) {
            // denominator validation (prevent divide-by-zero or odd denominators)
            if (time_sig.denominator == 0)       time_sig.denominator = 2;
            if (time_sig.denominator % 2 != 0)   time_sig.denominator = 4;
            if (current_time_signature.numerator == time_sig.numerator && current_time_signature.denominator == time_sig.denominator) return;
            ::set_time_signature(time_sig);  // current_time_signature is the single source of truth
            notify_time_sig_changed(time_sig);
        }
        time_sig_t get_time_signature() const { return ::get_time_signature(); }

        // // int wrappers for saveloadlib
        // int get_numerator_int()   const { return (int)get_time_signature_numerator(); }
        // int get_denominator_int() const { return (int)get_time_signature_denominator(); }
        // void set_numerator_int(int v)   { set_numerator((uint8_t)v); }
        // void set_denominator_int(int v) { set_denominator((uint8_t)v); }
    #endif

    // ── Global key / scale ──────────────────────────────────────────────────
    #ifdef ENABLE_SCALES
        int8_t get_scale_root() const { return global_scale_identity.root_note; }
        void   set_scale_root(int8_t r) { if (global_scale_identity.root_note == r) return; global_scale_identity.root_note = r; notify_harmony_changed(); }

        scale_index_t get_scale_type() const { return global_scale_identity.scale_number; }
        void          set_scale_type(scale_index_t t) { if (global_scale_identity.scale_number == t) return; global_scale_identity.scale_number = t; notify_harmony_changed(); }

        const scale_identity_t& get_scale_identity() const { return global_scale_identity; }

        quantise_mode_t get_global_quantise_mode() const { return global_quantise_mode; }
        void set_global_quantise_mode(quantise_mode_t mode) { if (global_quantise_mode == mode) return; global_quantise_mode = mode; notify_harmony_changed(); }

        // ── Global chord ─────────────────────────────────────────────────────
        int8_t      get_chord_degree()    const { return global_chord_identity.degree; }
        CHORD::Type get_chord_type()      const { return global_chord_identity.type; }
        int8_t      get_chord_inversion() const { return global_chord_identity.inversion; }

        int8_t      get_chord_root()      const { 
            return quantise_get_root_pitch_for_degree(
                global_chord_identity.degree, 
                get_scale_root(), 
                get_scale_type()
            );
        }

        void set_chord_degree(int8_t d)        { if (global_chord_identity.degree    == d)   return; global_chord_identity.degree    = d;   notify_harmony_changed(); }
        void set_chord_type(CHORD::Type t)     { if (global_chord_identity.type      == t)   return; global_chord_identity.type      = t;   notify_harmony_changed(); }
        void set_chord_inversion(int8_t inv)   { if (global_chord_identity.inversion == inv) return; global_chord_identity.inversion = inv; notify_harmony_changed(); }

        const chord_identity_t& get_chord_identity() const { return global_chord_identity; }
        void set_chord_identity(const chord_identity_t& c, bool requantise_immediately = true) { 
            if (!global_chord_identity.diff(c)) 
                return; 
            global_chord_identity = c; 
            notify_harmony_changed(); 
        }

        // Convenience: quantise a pitch to the current global scale
        int8_t quantise_to_scale(int8_t pitch) const {
            return quantise_pitch_to_scale(
                pitch, 
                global_scale_identity.root_note, 
                global_scale_identity.scale_number
            );
        }

        // Convenience: quantise a pitch to the current global chord
        int8_t quantise_to_chord(int8_t pitch, int8_t range = 0) const {
            return quantise_pitch_to_chord(
                pitch, 
                range,
                global_scale_identity.root_note, 
                global_scale_identity.scale_number, 
                global_chord_identity
            );
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
                    "bpm", "BPM", nullptr,
                    [this](float v) { set_bpm_value(v); },
                    [this]() -> float { return get_bpm_value(); }
                ),
                SL_SCOPE_PROJECT
            );

            // Clock mode — system scope (survives project changes)
            register_setting(
                new LSaveableSetting<int>(
                    "clock_mode", "Clock", nullptr,
                    [this](int v) { set_clock_mode_int(v); },
                    [this]() -> int { return get_clock_mode_int(); }
                ),
                SL_SCOPE_SYSTEM
            );

            #ifdef ENABLE_TIME_SIGNATURE
                register_setting(
                    new LSaveablePairSetting<uint8_t, uint8_t>(
                        "time_sig", "Time Signature",
                        [this](uint8_t num, uint8_t den) { set_time_signature({num, den}); },
                        [this]() -> uint8_t { return get_numerator(); },
                        [this]() -> uint8_t { return get_denominator(); }
                    ),
                    SL_SCOPE_PROJECT | SL_SCOPE_SCENE
                );
            #endif

            #ifdef ENABLE_SCALES
                register_setting(
                    new LSaveableSetting<quantise_mode_t>(
                        "Global quantise mode",
                        "Quantise",
                        &this->global_quantise_mode,
                        [=](quantise_mode_t value) -> void {
                            this->set_global_quantise_mode((quantise_mode_t)constrain((int)value, (int)QUANTISE_MODE_NONE, (int)QUANTISE_MODE_CHORD));
                        },
                        [=](void) -> quantise_mode_t {
                            return this->get_global_quantise_mode();
                        }
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT
                );

                register_setting(
                    new LSaveableSetting<int>(
                        "global_scale_root", "Key/Scale", nullptr,
                        [this](int v) { set_scale_root((int8_t)v); },
                        [this]() -> int { return (int)get_scale_root(); }
                    ),
                    SL_SCOPE_PROJECT
                );
                register_setting(
                    new LSaveableSetting<int>(
                        "global_scale_type", "Key/Scale", nullptr,
                        [this](int v) { set_scale_type((scale_index_t)v); },
                        [this]() -> int { return (int)get_scale_type(); }
                    ),
                    SL_SCOPE_PROJECT
                );
                register_setting(
                    new LSaveableSetting<int>(
                        "chord_degree", "Global chord", nullptr,
                        [this](int v) { set_chord_degree((int8_t)v); },
                        [this]() -> int { return (int)get_chord_degree(); }
                    ),
                    SL_SCOPE_PROJECT
                );
                register_setting(
                    new LSaveableSetting<int>(
                        "chord_type", "Global chord", nullptr,
                        [this](int v) { set_chord_type((CHORD::Type)v); },
                        [this]() -> int { return (int)get_chord_type(); }
                    ),
                    SL_SCOPE_PROJECT
                );
                register_setting(
                    new LSaveableSetting<int>(
                        "chord_inversion", "Global chord", nullptr,
                        [this](int v) { set_chord_inversion((int8_t)v); },
                        [this]() -> int { return (int)get_chord_inversion(); }
                    ),
                    SL_SCOPE_PROJECT
                );

                // global chord settings
                register_setting(new LSaveableSetting<CHORD::Type>(
                        "Global chord type",
                        "Chord",
                        &this->global_chord_identity.type
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow global chord type state to be saved at scene or project level, since it's more of a preference setting than a performance setting
                );
                register_setting(new LSaveableSetting<int8_t>(
                        "Global chord degree",
                        "Chord",
                        &this->global_chord_identity.degree
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow global chord degree state to be saved at scene or project level, since it's more of a preference setting than a performance setting
                );
                register_setting(new LSaveableSetting<int8_t>(
                        "Global chord inversion",
                        "Chord",
                        &this->global_chord_identity.inversion
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow global chord inversion state to be saved at scene or project level, since it's more of a preference setting than a performance setting
                );
            #endif // ENABLE_SCALES
        }
    #endif // ENABLE_STORAGE

// private:
    #ifdef ENABLE_TIME_SIGNATURE
        LinkedList<time_sig_change_cb_t> _time_sig_callbacks;

        void notify_time_sig_changed(time_sig_t ts) {
            for (uint8_t i = 0; i < _time_sig_callbacks.size(); i++)
                _time_sig_callbacks.get(i)(ts);
        }
    #endif // ENABLE_TIME_SIGNATURE

    #ifdef ENABLE_SCALES
        quantise_mode_t global_quantise_mode = QUANTISE_MODE_NONE;
        scale_identity_t global_scale_identity = {SCALE_MAJOR, SCALE_ROOT_C};
        chord_identity_t global_chord_identity = {CHORD::TRIAD, -1, 0};

        LinkedList<harmony_change_cb_t> _harmony_callbacks;

        void notify_harmony_changed() {
            for (uint8_t i = 0; i < _harmony_callbacks.size(); i++)
                _harmony_callbacks.get(i)(global_scale_identity, global_chord_identity);
        }
    #endif
};

// Global singleton instance — define in exactly one .cpp:
//   #include "conductor.h"
//   Conductor *conductor = nullptr;
//
// Then declare extern in other files:
//   extern Conductor *conductor;
extern Conductor *conductor;
