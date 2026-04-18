// arranger.h
// Song-structure / chord-progression engine.
// Manages sections, playlists, and auto-advance logic, independent of any
// specific MIDI output or UI.
//
// Guard with #define ENABLE_ARRANGER in your project config.
// Requires ENABLE_SCALES.
#pragma once

#ifdef ENABLE_ARRANGER

#include "scales.h"

#include <LinkedList.h>
#include <functional-vlpp.h>
#include "saveloadlib.h"
#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
#endif

// ── Tuneable constants (override before #include if desired) ────────────

#ifndef NUM_SONG_SECTIONS
#define NUM_SONG_SECTIONS 4
#endif

#ifndef CHORDS_PER_SECTION
#define CHORDS_PER_SECTION 8
#endif

#ifndef NUM_PLAYLIST_SLOTS
#define NUM_PLAYLIST_SLOTS 8
#endif

#ifndef MAX_REPEATS
#define MAX_REPEATS 64
#endif

// ── Data types ──────────────────────────────────────────────────────────

struct song_section_t {
    chord_identity_t grid[CHORDS_PER_SECTION];

    void add_section_add_lines(LinkedList<String> *lines) {
        for (int i = 0 ; i < CHORDS_PER_SECTION ; i++) {
            lines->add(String("grid_")+String(i)+String("_degree=")+String(grid[i].degree));
            lines->add(String("grid_")+String(i)+String("_type=")+String(grid[i].type));
            lines->add(String("grid_")+String(i)+String("_inversion=")+String(grid[i].inversion));
        }
    }
    bool parse_section_line(String key, String value) {
        if (key.startsWith("grid_")) {
            int8_t grid_index = key.substring(5,6).toInt();
            if (grid_index>=0 && grid_index<CHORDS_PER_SECTION) {
                if (key.endsWith("_degree")) {
                    grid[grid_index].degree = value.toInt();
                } else if (key.endsWith("_type")) {
                    grid[grid_index].type = (CHORD::Type)value.toInt();
                } else if (key.endsWith("_inversion")) {
                    grid[grid_index].inversion = value.toInt();
                }
            }
            return true;
        }
        return false;
    }
};

struct playlist_entry_t {
    int8_t section;
    int8_t repeats;
};

struct playlist_t {
    playlist_entry_t entries[NUM_PLAYLIST_SLOTS] = {
        { 0, 2 },
        { 1, 2 },
        { 2, 2 },
        { 3, 2 },
        { 0, 2 },
        { 1, 2 },
        { 2, 2 },
        { 3, 2 },
    };
    void save_project_add_lines(LinkedList<String> *lines) {
        for (int i = 0 ; i < NUM_PLAYLIST_SLOTS ; i++) {
            lines->add(String("section_")+String(i)+String("=")+String(entries[i].section));
            lines->add(String("repeats_")+String(i)+String("=")+String(entries[i].repeats));
        }
    }
    bool parse_key_value(String key, String value) {
        if (key.startsWith("section_")) {
            int8_t slot = key.substring(8,9).toInt();
            if (slot>=0 && slot<NUM_PLAYLIST_SLOTS) {
                entries[slot].section = value.toInt();
                return true;
            }
        } else if (key.startsWith("repeats_")) {
            int8_t slot = key.substring(8,9).toInt();
            if (slot>=0 && slot<NUM_PLAYLIST_SLOTS) {
                entries[slot].repeats = value.toInt();
                return true;
            }
        }
        return false;
    }
};

// ── Saveable setting subclasses for song data ───────────────────────────

// Saves/loads a single song_section_t grid as a compact hex string.
// Format: 3 hex-byte pairs per chord (degree, type, inversion) × CHORDS_PER_SECTION
// e.g. "010002050001..." = chord0{degree=1,type=0,inversion=0}, chord1{degree=2,type=5,inversion=0}, ...
// degree is offset by +1 so that -1 (use global) maps to 0x00.
class SaveableSectionGridSetting : public SaveableSettingBase {
public:
    song_section_t* section;

    SaveableSectionGridSetting(const char* lbl, const char* cat, song_section_t* sec)
        : section(sec) {
        set_label(lbl);
        set_category(cat ? cat : "");
    }

    const char* get_line() override {
        int pos = snprintf(linebuf, SL_MAX_LINE, "%s=", label);
        for (int i = 0; i < CHORDS_PER_SECTION && pos < SL_MAX_LINE - 6; i++) {
            uint8_t d = (uint8_t)(section->grid[i].degree + 1);  // offset: -1→0, 0→1, 7→8
            uint8_t t = (uint8_t)section->grid[i].type;
            uint8_t v = (uint8_t)(section->grid[i].inversion + 1); // offset for potential negatives
            pos += snprintf(linebuf + pos, SL_MAX_LINE - pos, "%02x%02x%02x", d, t, v);
        }
        linebuf[pos] = '\0';
        return linebuf;
    }

    bool parse_key_value(const char* key, const char* value) override {
        if (strcmp(key, label) != 0) return false;
        for (int i = 0; i < CHORDS_PER_SECTION; i++) {
            int base = i * 6; // 3 bytes × 2 hex chars each
            if (!value[base] || !value[base+1] || !value[base+2] ||
                !value[base+3] || !value[base+4] || !value[base+5]) break;
            char hex[3] = {0};
            hex[0] = value[base];   hex[1] = value[base+1];
            section->grid[i].degree = (int8_t)(strtol(hex, nullptr, 16) - 1);
            hex[0] = value[base+2]; hex[1] = value[base+3];
            section->grid[i].type = (CHORD::Type)strtol(hex, nullptr, 16);
            hex[0] = value[base+4]; hex[1] = value[base+5];
            section->grid[i].inversion = (int8_t)(strtol(hex, nullptr, 16) - 1);
        }
        return true;
    }

    size_t heap_size() const override { return sizeof(SaveableSectionGridSetting); }
};

// Saves/loads the entire playlist as a compact hex string.
// Format: 2 hex-byte pairs per entry (section, repeats) × NUM_PLAYLIST_SLOTS
// e.g. "0002010201020302" = slot0{section=0,repeats=2}, slot1{section=1,repeats=2}, ...
class SaveablePlaylistSetting : public SaveableSettingBase {
public:
    playlist_t* playlist;

    SaveablePlaylistSetting(const char* lbl, const char* cat, playlist_t* pl)
        : playlist(pl) {
        set_label(lbl);
        set_category(cat ? cat : "");
    }

    const char* get_line() override {
        int pos = snprintf(linebuf, SL_MAX_LINE, "%s=", label);
        for (int i = 0; i < NUM_PLAYLIST_SLOTS && pos < SL_MAX_LINE - 4; i++) {
            uint8_t s = (uint8_t)playlist->entries[i].section;
            uint8_t r = (uint8_t)playlist->entries[i].repeats;
            pos += snprintf(linebuf + pos, SL_MAX_LINE - pos, "%02x%02x", s, r);
        }
        linebuf[pos] = '\0';
        return linebuf;
    }

    bool parse_key_value(const char* key, const char* value) override {
        if (strcmp(key, label) != 0) return false;
        for (int i = 0; i < NUM_PLAYLIST_SLOTS; i++) {
            int base = i * 4; // 2 bytes × 2 hex chars each
            if (!value[base] || !value[base+1] || !value[base+2] || !value[base+3]) break;
            char hex[3] = {0};
            hex[0] = value[base];   hex[1] = value[base+1];
            playlist->entries[i].section = (int8_t)strtol(hex, nullptr, 16);
            hex[0] = value[base+2]; hex[1] = value[base+3];
            playlist->entries[i].repeats = (int8_t)strtol(hex, nullptr, 16);
        }
        return true;
    }

    size_t heap_size() const override { return sizeof(SaveablePlaylistSetting); }
};

// ── Arranger ────────────────────────────────────────────────────────────

/*
    Song structure:
        - Playlist has NUM_PLAYLIST_SLOTS slots
        - Each playlist slot points to a section, and has a number of repeats
        - Song has NUM_SONG_SECTIONS possible sections
        - Each section has CHORDS_PER_SECTION chords (ie bars per section)
*/

class Arranger
#ifdef ENABLE_STORAGE
    : public SHDynamic<16, 2>
#endif
{
public:
    Arranger() {
        #ifdef ENABLE_STORAGE
            this->set_path_segment("Arranger");
        #endif
    }

    enum progression_cadence_t : int8_t {
        PROGRESSION_PER_BAR = 0,
        PROGRESSION_PER_PHRASE = 1,
    };

    enum playback_mode_t : int8_t {
        LOOP_SECTION = 0,
        PLAYLIST = 1,
    };

    // ── Callbacks ───────────────────────────────────────────────────────
    // Fired when the current chord changes (e.g. bar advance or manual move).
    // The bool indicates whether downstream should requantise immediately.
    using chord_changed_cb_t = vl::Func<void(const chord_identity_t&, bool requantise)>;

    // Fired when a phrase ends and the playlist advances to a new section.
    // Consumer can use this to e.g. stop the chord player before the new section starts.
    using section_changed_cb_t = vl::Func<void(int8_t new_section)>;

    // Optional direct sink for applications that want Arranger to drive
    // a harmony destination directly (e.g. Conductor) without external glue.
    using chord_sink_cb_t = vl::Func<void(const chord_identity_t&, bool requantise)>;

    void on_chord_changed(chord_changed_cb_t cb)     { _chord_cbs.add(cb); }
    void on_section_changed(section_changed_cb_t cb)  { _section_cbs.add(cb); }
    void set_chord_sink(chord_sink_cb_t cb)          { _chord_sink_cb = cb; }

    // ── Song data ───────────────────────────────────────────────────────
    song_section_t song_sections[NUM_SONG_SECTIONS];
    playlist_t     playlist;

    // ── Playback state ──────────────────────────────────────────────────
    int8_t current_bar            = -1;   // -1 so first on_bar() advances to 0
    int8_t current_section        = 0;
    int8_t playlist_position      = 0;
    int8_t current_section_plays  = 0;
    chord_identity_t current_chord;

    bool advance_bar      = true;
    bool advance_playlist = true;
    bool enabled = true;

    progression_cadence_t progression_cadence = PROGRESSION_PER_BAR;
    playback_mode_t playback_mode = LOOP_SECTION;

    bool debug = false;

    // dirty flag for save tracking
    bool modified_since_save = true;
    bool has_changes_to_save() const    { return modified_since_save; }
    void mark_save_done()               { modified_since_save = false; }
    void mark_as_modified()             { modified_since_save = true; }

    // ── Mode/Query helpers ─────────────────────────────────────────────
    void set_progression_cadence(progression_cadence_t mode) { progression_cadence = mode; mark_as_modified(); }
    progression_cadence_t get_progression_cadence() const { return progression_cadence; }

    void set_playback_mode(playback_mode_t mode) { playback_mode = mode; mark_as_modified(); }
    playback_mode_t get_playback_mode() const { return playback_mode; }

    void set_enabled(bool state) { enabled = state; mark_as_modified(); }
    bool is_enabled() const { return enabled; }

    bool is_playlist_mode() const { return playback_mode == PLAYLIST; }
    bool is_loop_section_mode() const { return playback_mode == LOOP_SECTION; }

    const chord_identity_t& get_current_chord() const { return current_chord; }
    const chord_identity_t& get_chord_at(int8_t section, int8_t bar) const {
        static chord_identity_t invalid;
        if (section < 0 || section >= NUM_SONG_SECTIONS) return invalid;
        if (bar < 0 || bar >= CHORDS_PER_SECTION) return invalid;
        return song_sections[section].grid[bar];
    }

    int8_t get_current_playlist_repeats() const {
        int8_t repeats = playlist.entries[playlist_position].repeats;
        return repeats < 1 ? 1 : repeats;
    }

    // ── Navigation ──────────────────────────────────────────────────────

    void move_bar(int new_bar) {
        if (new_bar >= CHORDS_PER_SECTION) {
            new_bar = 0;
        } else if (new_bar < 0) {
            new_bar = CHORDS_PER_SECTION - 1;
        }
        current_bar = new_bar;
        current_chord = song_sections[current_section].grid[current_bar];
        mark_as_modified();
        notify_chord_changed(current_chord, true);
    }

    void move_next_playlist() {
        move_playlist(playlist_position + 1);
    }

    void move_playlist(int8_t pos) {
        playlist_position = pos;
        if (playlist_position >= NUM_PLAYLIST_SLOTS) playlist_position = 0;
        if (playlist_position < 0)                   playlist_position = NUM_PLAYLIST_SLOTS - 1;
        mark_as_modified();
        change_section(playlist.entries[playlist_position].section);
    }

    // Changes section, but leaves resetting the bar to the caller.
    void change_section(int section_number) {
        if (section_number >= NUM_SONG_SECTIONS)
            section_number = 0;
        else if (section_number < 0)
            section_number = NUM_SONG_SECTIONS - 1;

        current_section_plays = 0;
        current_section = section_number;
        mark_as_modified();
        notify_section_changed(current_section);
    }

    // ── Clock-driven callbacks (call from your behaviour) ───────────────

    void on_bar(int bar_number) {
        if (!enabled) return;

        if (progression_cadence == PROGRESSION_PER_BAR) {
            if (advance_bar) {
                move_bar(current_bar + 1);
            } else {
                move_bar(current_bar);
            }
        }
    }

    void on_end_phrase(uint32_t phrase_number) {
        if (!enabled) return;

        if (progression_cadence == PROGRESSION_PER_PHRASE) {
            if (advance_bar) {
                move_bar(current_bar + 1);
            } else {
                move_bar(current_bar);
            }
        }

        if (playback_mode == PLAYLIST && advance_playlist) {
            current_section_plays++;

            if (current_section_plays >= get_current_playlist_repeats()) {
                current_section_plays = 0;
                move_next_playlist();
            }
        }
    }

    void on_restart() {
        playlist_position = 0;
        current_section_plays = 0;
        if (playlist_position >= 0 && playlist_position < NUM_PLAYLIST_SLOTS)
            current_section = playlist.entries[playlist_position].section;
        current_bar = -1;
        mark_as_modified();
    }

    #ifdef ENABLE_STORAGE
    virtual void setup_saveable_settings() override {
        SHDynamic<16, 2>::setup_saveable_settings();

        register_setting(
            new LSaveableSetting<int>(
                "progression_cadence", "Arranger", nullptr,
                [this](int v) {
                    this->set_progression_cadence((progression_cadence_t)constrain(v, 0, 1));
                },
                [this]() -> int {
                    return (int)this->get_progression_cadence();
                }
            ),
            SL_SCOPE_PROJECT
        );

        register_setting(
            new LSaveableSetting<int>(
                "playback_mode", "Arranger", nullptr,
                [this](int v) {
                    this->set_playback_mode((playback_mode_t)constrain(v, 0, 1));
                },
                [this]() -> int {
                    return (int)this->get_playback_mode();
                }
            ),
            SL_SCOPE_PROJECT
        );

        register_setting(
            new LSaveableSetting<bool>(
                "enabled", "Arranger", nullptr,
                [this](bool v) {
                    this->set_enabled(v);
                },
                [this]() -> bool {
                    return this->is_enabled();
                }
            ),
            SL_SCOPE_PROJECT
        );

        register_setting(
            new SaveablePlaylistSetting("playlist_grid", "Arranger", &this->playlist),
            SL_SCOPE_PROJECT
        );

        for (int8_t i = 0; i < NUM_SONG_SECTIONS; i++) {
            char label[20];
            snprintf(label, sizeof(label), "section_%d_grid", i);
            register_setting(
                new SaveableSectionGridSetting(label, "Arranger", &this->song_sections[i]),
                SL_SCOPE_PROJECT
            );
        }
    }
    #endif

private:
    LinkedList<chord_changed_cb_t>   _chord_cbs;
    LinkedList<section_changed_cb_t> _section_cbs;
    chord_sink_cb_t _chord_sink_cb;

    void notify_chord_changed(const chord_identity_t& chord, bool requantise) {
        if (_chord_sink_cb) {
            _chord_sink_cb(chord, requantise);
        }
        for (uint8_t i = 0; i < _chord_cbs.size(); i++)
            _chord_cbs.get(i)(chord, requantise);
    }

    void notify_section_changed(int8_t section) {
        for (uint8_t i = 0; i < _section_cbs.size(); i++)
            _section_cbs.get(i)(section);
    }
};

// Global singleton — create in your setup(), declare extern elsewhere.
extern Arranger *arranger;

#endif // ENABLE_ARRANGER
