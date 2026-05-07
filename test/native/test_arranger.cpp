// test_arranger.cpp
// Native Unity tests for the Arranger state machine.

#include <unity.h>

// Stubs must come before any midihelpers headers
#include "Arduino.h"

// Pull in arranger with full feature set
// (these are already set via build_flags but guard them here for clarity)

#include "arranger.h"

// ── Test helper globals ─────────────────────────────────────────────────

// Advance ticks by one full bar and call on_bar() on the arranger.
// bar_number arg mirrors what the real clock passes (BPM_CURRENT_BAR_OF_PHRASE).
static void tick_one_bar(Arranger* a) {
    ticks += (uint32_t)TICKS_PER_BAR;
    a->on_bar(0 /* bar_number not used by implementation logic */);
}

static void tick_bars(Arranger* a, int n) {
    for (int i = 0; i < n; i++) tick_one_bar(a);
}

// ── Unity lifecycle ────────────────────────────────────────────────────

void setUp() {
    // Reset global bpm state before every test
    ticks           = 0;
    ts_phase_offset = 0;
#ifdef ENABLE_TIME_SIGNATURE
    current_time_signature = { DEFAULT_TIME_SIGNATURE_NUMERATOR, DEFAULT_TIME_SIGNATURE_DENOMINATOR };
#endif
}

void tearDown() {}

// ── Tests ──────────────────────────────────────────────────────────────

// After on_restart() the arranger is at playlist slot 0, section comes from
// that slot, and current_bar is -1 (will become 0 on first on_bar()).
void test_on_restart_initial_state() {
    Arranger a;
    a.setup_saveable_settings();
    a.playlist.entries[0] = { 2, 1, 0 };   // slot 0 → section 2
    a.on_restart();

    TEST_ASSERT_EQUAL_INT(0,  a.playlist_position);
    TEST_ASSERT_EQUAL_INT(2,  a.current_section);
    TEST_ASSERT_EQUAL_INT(-1, a.current_bar);
    TEST_ASSERT_EQUAL_INT(0,  a.current_section_plays);
    TEST_ASSERT_EQUAL_INT(-1, a.pending_jump_section);
    TEST_ASSERT_EQUAL_INT(-1, a.pending_jump_bar);
}

// After one on_bar() following on_restart(), current_bar should be 0 (not 1).
void test_first_bar_after_restart_is_zero() {
    Arranger a;
    a.setup_saveable_settings();
    a.on_restart();

    tick_one_bar(&a);
    TEST_ASSERT_EQUAL_INT(0, a.current_bar);
}

// Basic bar advance: 8-bar section, bars should cycle 0..7 and wrap back.
void test_basic_bar_advance() {
    Arranger a;
    a.setup_saveable_settings();
    a.song_sections[0].length = 8;
    a.advance_bar = true;
    a.on_restart();

    for (int expected = 0; expected < 8; expected++) {
        tick_one_bar(&a);
        TEST_ASSERT_EQUAL_INT(expected, a.current_bar);
    }
    // Bar 8 wraps back to 0
    tick_one_bar(&a);
    TEST_ASSERT_EQUAL_INT(0, a.current_bar);
}

// With advance_bar=false, current_bar stays at 0 after on_restart().
void test_advance_bar_false_stays_at_zero() {
    Arranger a;
    a.setup_saveable_settings();
    a.advance_bar = false;
    a.on_restart();

    tick_bars(&a, 8);
    TEST_ASSERT_EQUAL_INT(0, a.current_bar);
}

// Playlist advances after section length bars, not bars_per_phrase.
// Standard section: length=8, bars_per_phrase=4 (2 phrases per section play).
// Advance fires after all 8 bars, NOT at the phrase boundary at bar 4.
void test_section_length_triggers_playlist_advance() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;  // 2 phrases per section — must NOT trigger advance
    a.playlist.entries[0] = { 0, 1, 0 };
    a.playlist.entries[1] = { 1, 1, 0 };
    a.on_restart();

    tick_bars(&a, 4);  // first phrase done — must NOT advance yet
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);
    tick_bars(&a, 3);  // 7 bars total — still not done
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);
    tick_bars(&a, 1);  // 8th bar — advance
    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
}

// max_bars on a playlist entry overrides section length.
// Section has length=8 but entry caps it at 4: advance fires after 4 bars.
void test_max_bars_override() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;  // full section is 8 bars
    a.song_sections[0].bars_per_phrase = 4;
    a.playlist.entries[0] = { 0, 1, 4 };    // max_bars=4: exit after half the section
    a.playlist.entries[1] = { 1, 1, 0 };
    a.on_restart();

    tick_bars(&a, 3);  // not yet
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);
    tick_bars(&a, 1);  // 4th bar → advance (ignoring remaining 4 bars of full section)
    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
}

// Section with repeats=3: playlist should advance only after the 3rd full section play.
// Standard section: length=8, bars_per_phrase=4 → 3×8=24 bars total before advance.
void test_multiple_repeats() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;
    a.playlist.entries[0] = { 0, 3, 0 };  // repeat 3 times → 3×8=24 bars total
    a.playlist.entries[1] = { 1, 1, 0 };
    a.on_restart();

    // Play 1 (bars 1-8)
    tick_bars(&a, 8);
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);

    // Play 2 (bars 9-16)
    tick_bars(&a, 8);
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);

    // Play 3 (bars 17-24) — advances on the 24th bar
    tick_bars(&a, 8);
    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
}

// Last playlist slot wraps back around to slot 0.
// Standard section length=8; after NUM_PLAYLIST_SLOTS×8 bars we're back at slot 0.
void test_playlist_wraparound() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;
    for (int i = 0; i < NUM_PLAYLIST_SLOTS; i++) {
        a.playlist.entries[i] = { 0, 1, 0 };
    }
    a.on_restart();

    // Advance through all 16 slots (16×8=128 bars) and confirm wraparound
    tick_bars(&a, NUM_PLAYLIST_SLOTS * 8);
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);
}

// advance_playlist=false: section should not advance no matter how many bars pass.
void test_advance_playlist_false() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = false;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;
    a.playlist.entries[0] = { 0, 1, 0 };
    a.playlist.entries[1] = { 1, 1, 0 };
    a.on_restart();

    tick_bars(&a, 40);  // 5 full sections worth — must never advance
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);
}

// pending_jump_section is consumed at the next on_bar() and the correct
// section is loaded.
void test_pending_jump_section() {
    Arranger a;
    a.setup_saveable_settings();
    a.on_restart();
    tick_one_bar(&a);  // current_bar = 0

    a.pending_jump_section = 3;
    tick_one_bar(&a);

    TEST_ASSERT_EQUAL_INT(3,  a.current_section);
    TEST_ASSERT_EQUAL_INT(-1, a.pending_jump_section);
    // current_bar should now be -1 (change_section resets it); next tick → 0
    TEST_ASSERT_EQUAL_INT(-1, a.current_bar);
}

// pending_jump_bar is consumed at the next on_bar() and the correct bar is set.
void test_pending_jump_bar() {
    Arranger a;
    a.setup_saveable_settings();
    a.song_sections[0].length = 8;
    a.on_restart();
    tick_bars(&a, 4);  // current_bar = 3

    a.pending_jump_bar = 6;
    tick_one_bar(&a);

    TEST_ASSERT_EQUAL_INT(6, a.current_bar);
    TEST_ASSERT_EQUAL_INT(-1, a.pending_jump_bar);
}

// change_section() applies the section's time signature.
void test_time_sig_applied_on_section_change() {
    Arranger a;
    a.setup_saveable_settings();
    a.song_sections[2].time_signature = { 3, 4 };
    a.on_restart();

    a.change_section(2);

    TEST_ASSERT_EQUAL_UINT8(3, current_time_signature.numerator);
    TEST_ASSERT_EQUAL_UINT8(4, current_time_signature.denominator);
}

// on_after_load() reapplies the active section's time signature.
void test_on_after_load_applies_time_sig() {
    Arranger a;
    a.setup_saveable_settings();
    a.song_sections[1].time_signature = { 6, 8 };
    a.current_section = 1;
    a.on_after_load();

    TEST_ASSERT_EQUAL_UINT8(6, current_time_signature.numerator);
    TEST_ASSERT_EQUAL_UINT8(8, current_time_signature.denominator);
}

// When disabled, on_bar() is a no-op.
void test_disabled_noop() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_enabled(false);
    a.on_restart();
    tick_bars(&a, 8);

    // current_bar should remain -1 (change_section sets it, but on_bar is skipped)
    TEST_ASSERT_EQUAL_INT(-1, a.current_bar);
}

// Every section name must resolve to a valid (non-negative) index within bounds.
// Regression for NUM_SONG_SECTIONS being one less than the names array length,
// causing "Outro" (the last name) to return -1 and silently play the wrong section.
void test_all_section_names_resolve() {
    for (int i = 0; i < NUM_SONG_SECTIONS; i++) {
        const char* name = get_section_name(i);
        TEST_ASSERT_NOT_NULL(name);
        int8_t idx = get_section_idx_for_name(name);
        char msg[64];
        snprintf(msg, sizeof(msg), "Section %d ('%s') resolves to %d", i, name, (int)idx);
        TEST_ASSERT_EQUAL_INT_MESSAGE(i, (int)idx, msg);
    }
}

// The default playlist_t (constructed without changes) must have no -1 section
// indices — a -1 causes change_section() to wrap to the last section silently.
void test_default_playlist_no_invalid_sections() {
    playlist_t p;
    for (int i = 0; i < NUM_PLAYLIST_SLOTS; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Slot %d has invalid section index %d", i, (int)p.entries[i].section);
        TEST_ASSERT_TRUE_MESSAGE(p.entries[i].section >= 0, msg);
        TEST_ASSERT_TRUE_MESSAGE(p.entries[i].section < NUM_SONG_SECTIONS, msg);
    }
}

// Walk through each section in PLAYLIST mode using tick_bars only.
// Verifies that bars advance within each section and the playlist slot increments
// after the phrase boundary.
void test_full_playlist() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_bar      = true;
    a.advance_playlist = true;
    for (int i = 0; i < NUM_SONG_SECTIONS; i++) {
        a.song_sections[i].length         = 8;  // standard section length
        a.song_sections[i].bars_per_phrase = 4;  // 2 phrases per play — must not trigger advance mid-section
        a.playlist.entries[i] = { (int8_t)i, 1, 0 };
    }
    // Remaining slots reference section 0 so wraparound is safe
    for (int i = NUM_SONG_SECTIONS; i < NUM_PLAYLIST_SLOTS; i++) {
        a.playlist.entries[i] = { 0, 1, 0 };
    }
    a.on_restart();

    for (int slot = 0; slot < NUM_SONG_SECTIONS; slot++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Expected playlist slot %d", slot);
        TEST_ASSERT_EQUAL_INT_MESSAGE(slot, a.playlist_position, msg);

        // Mid-section phrase boundary must NOT fire the advance
        tick_bars(&a, 4);
        TEST_ASSERT_EQUAL_INT_MESSAGE(slot, a.playlist_position, msg);

        // Second half of section — advance fires on bar 8
        tick_bars(&a, 4);
    }
    // After all 10 sections, should be sitting at the first overflow slot
    TEST_ASSERT_EQUAL_INT(NUM_SONG_SECTIONS, a.playlist_position);
}

// ── Inactive slot tests ────────────────────────────────────────────────

// A slot with repeats==0 is inactive: move_next_playlist() must skip it.
void test_inactive_slot_skipped() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;
    a.song_sections[2].length         = 8;
    a.song_sections[2].bars_per_phrase = 4;
    a.playlist.entries[0] = { 0, 1, 0 };   // active
    a.playlist.entries[1] = { 1, 0, 0 };   // inactive (repeats=0) — must be skipped
    a.playlist.entries[2] = { 2, 1, 0 };   // active
    // rest stay at default
    a.on_restart();
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);

    tick_bars(&a, 4);   // mid-section phrase boundary — must NOT advance
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);
    tick_bars(&a, 4);   // completes section → skips inactive slot 1, lands on slot 2
    TEST_ASSERT_EQUAL_INT(2, a.playlist_position);
}

// Multiple consecutive inactive slots are all skipped.
void test_multiple_inactive_slots_skipped() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;
    a.song_sections[4].length         = 8;
    a.song_sections[4].bars_per_phrase = 4;
    a.playlist.entries[0] = { 0, 1, 0 };   // active
    a.playlist.entries[1] = { 1, 0, 0 };   // inactive
    a.playlist.entries[2] = { 2, 0, 0 };   // inactive
    a.playlist.entries[3] = { 3, 0, 0 };   // inactive
    a.playlist.entries[4] = { 4, 1, 0 };   // active
    a.on_restart();

    tick_bars(&a, 8);  // complete section 0 → skip slots 1-3, land on slot 4
    TEST_ASSERT_EQUAL_INT(4, a.playlist_position);
}

// If all slots are inactive, move_next_playlist() stays put rather than crashing.
void test_all_inactive_stays_put() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    for (int i = 0; i < NUM_PLAYLIST_SLOTS; i++) {
        a.playlist.entries[i] = { 0, 0, 0 };  // all inactive
    }
    // on_restart() will land on slot 0 (fallback) and call change_section
    a.on_restart();
    int pos_before = a.playlist_position;

    a.move_next_playlist();
    TEST_ASSERT_EQUAL_INT(pos_before, a.playlist_position);
}

// on_restart() skips leading inactive slots and starts at the first active one.
void test_restart_skips_inactive_leading_slots() {
    Arranger a;
    a.setup_saveable_settings();
    a.playlist.entries[0] = { 0, 0, 0 };   // inactive
    a.playlist.entries[1] = { 0, 0, 0 };   // inactive
    a.playlist.entries[2] = { 3, 1, 0 };   // first active
    a.on_restart();
    TEST_ASSERT_EQUAL_INT(2, a.playlist_position);
    TEST_ASSERT_EQUAL_INT(3, a.current_section);
}

// After a playlist advance current_bar must be 0 immediately, not -1.
// set_time_signature() inside change_section() anchors ts_phase_offset = ticks,
// so LoopMarkerPanel considers this bar as bar 1 of the new section.
// If current_bar stayed at -1 the bar indicator would lag one bar behind.
void test_current_bar_synced_after_playlist_advance() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_bar      = true;
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;
    a.song_sections[1].length         = 8;
    a.song_sections[1].bars_per_phrase = 4;
    a.playlist.entries[0] = { 0, 1, 0 };
    a.playlist.entries[1] = { 1, 1, 0 };
    a.on_restart();

    tick_bars(&a, 8);  // completes section 0 → advance to slot 1

    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
    // current_bar must already be 0 (not -1) on the very bar of the advance.
    TEST_ASSERT_EQUAL_INT(0, a.current_bar);
}

// ── Uncommon value tests ───────────────────────────────────────────────

// bars_per_phrase is purely a musical subdivision and must never control
// when the playlist advances, even when set to a value that evenly divides
// the section length.
void test_bars_per_phrase_does_not_control_advance() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 2;  // 4 phrases per section — none should fire advance
    a.playlist.entries[0] = { 0, 1, 0 };
    a.playlist.entries[1] = { 1, 1, 0 };
    a.on_restart();

    // Check at every phrase boundary: 2, 4, 6 bars — all must still be slot 0
    for (int bar = 2; bar < 8; bar += 2) {
        tick_bars(&a, 2);
        char msg[64];
        snprintf(msg, sizeof(msg), "Should still be slot 0 after %d bars", bar);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, a.playlist_position, msg);
    }
    // 8th bar — advance fires
    tick_bars(&a, 2);
    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
}

// repeats=2 with a standard section: 2×8=16 bars before advance.
void test_two_repeats_standard_section() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;
    a.playlist.entries[0] = { 0, 2, 0 };  // 2 repeats → 16 bars
    a.playlist.entries[1] = { 1, 1, 0 };
    a.on_restart();

    tick_bars(&a, 8);   // first play done — must still be slot 0
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);
    tick_bars(&a, 7);   // 15 bars — one short
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);
    tick_bars(&a, 1);   // 16th bar — advance
    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
}

// max_bars smaller than bars_per_phrase: exits mid-phrase cleanly.
void test_max_bars_smaller_than_bars_per_phrase() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;
    a.playlist.entries[0] = { 0, 1, 2 };  // exit after just 2 bars (half a phrase)
    a.playlist.entries[1] = { 1, 1, 0 };
    a.on_restart();

    tick_bars(&a, 1);
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);
    tick_bars(&a, 1);  // 2nd bar → advance
    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
}

// max_bars larger than section length: treats max_bars as the actual limit,
// allowing the section to play longer than its normal chord-grid length.
void test_max_bars_larger_than_section_length() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 4;
    a.song_sections[0].bars_per_phrase = 4;
    a.playlist.entries[0] = { 0, 1, 12 }; // max_bars=12 overrides length=4
    a.playlist.entries[1] = { 1, 1, 0 };
    a.on_restart();

    tick_bars(&a, 4);   // section length reached — must NOT advance (max_bars wins)
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);
    tick_bars(&a, 8);   // 12 bars total → advance
    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
}

// ── Time signature tests ───────────────────────────────────────────────

// change_section() applies the new section's time signature immediately.
// Verify ts_phase_offset is anchored at the current tick so LoopMarkerPanel
// starts at position 0 for the new metre.
void test_time_sig_applied_on_playlist_advance() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;
    a.song_sections[0].time_signature  = { 4, 4 };
    a.song_sections[1].length         = 8;
    a.song_sections[1].bars_per_phrase = 4;
    a.song_sections[1].time_signature  = { 3, 4 };
    a.playlist.entries[0] = { 0, 1, 0 };
    a.playlist.entries[1] = { 1, 1, 0 };
    a.on_restart();

    TEST_ASSERT_EQUAL_UINT8(4, current_time_signature.numerator);

    tick_bars(&a, 8);   // complete section 0 → advance to section 1 (3/4)

    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
    TEST_ASSERT_EQUAL_UINT8(3, current_time_signature.numerator);
    TEST_ASSERT_EQUAL_UINT8(4, current_time_signature.denominator);
    // ts_phase_offset must be anchored at the tick of the advance so that
    // BPM_PHASE_TICKS(ticks) resets to 0 for the new section.
    TEST_ASSERT_EQUAL_UINT32(ticks, ts_phase_offset);
}

// Walk 4/4 → 3/4 → 5/4 matching the user's hardware test.
// Each section must apply its own time signature and bar-count correctly.
void test_time_sig_sequence_4_4_to_3_4_to_5_4() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;

    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;
    a.song_sections[0].time_signature  = { 4, 4 };

    a.song_sections[1].length         = 8;
    a.song_sections[1].bars_per_phrase = 4;
    a.song_sections[1].time_signature  = { 3, 4 };

    a.song_sections[2].length         = 8;
    a.song_sections[2].bars_per_phrase = 4;
    a.song_sections[2].time_signature  = { 5, 4 };

    a.playlist.entries[0] = { 0, 1, 0 };
    a.playlist.entries[1] = { 1, 1, 0 };
    a.playlist.entries[2] = { 2, 1, 0 };
    a.on_restart();

    // Section 0: 4/4 — TICKS_PER_BAR = 24*4 = 96 ticks/bar
    TEST_ASSERT_EQUAL_UINT8(4, current_time_signature.numerator);
    tick_bars(&a, 8);
    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);

    // Section 1: 3/4 — TICKS_PER_BAR = 24*3 = 72 ticks/bar
    TEST_ASSERT_EQUAL_UINT8(3, current_time_signature.numerator);
    TEST_ASSERT_EQUAL_UINT8(4, current_time_signature.denominator);
    tick_bars(&a, 7);   // 7 bars of 3/4 — not yet
    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
    tick_bars(&a, 1);   // 8th bar → advance
    TEST_ASSERT_EQUAL_INT(2, a.playlist_position);

    // Section 2: 5/4
    TEST_ASSERT_EQUAL_UINT8(5, current_time_signature.numerator);
    TEST_ASSERT_EQUAL_UINT8(4, current_time_signature.denominator);
    tick_bars(&a, 8);
    TEST_ASSERT_EQUAL_INT(3, a.playlist_position);
}

// ts_phase_offset reset: after each section change LoopMarkerPanel's
// BPM_PHASE_TICKS(ticks) % TICKS_PER_PHRASE must be 0 at the section boundary.
// This test simply verifies that ts_phase_offset equals the current ticks value
// immediately after each playlist advance.
void test_ts_phase_offset_reset_on_section_change() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;

    a.song_sections[0].length        = 8;
    a.song_sections[0].time_signature = { 4, 4 };
    a.song_sections[1].length        = 8;
    a.song_sections[1].time_signature = { 3, 4 };
    a.song_sections[2].length        = 8;
    a.song_sections[2].time_signature = { 5, 4 };

    a.playlist.entries[0] = { 0, 1, 0 };
    a.playlist.entries[1] = { 1, 1, 0 };
    a.playlist.entries[2] = { 2, 1, 0 };
    a.on_restart();

    tick_bars(&a, 8);
    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
    TEST_ASSERT_EQUAL_UINT32(ticks, ts_phase_offset);  // anchored at section 1 start

    tick_bars(&a, 8);
    TEST_ASSERT_EQUAL_INT(2, a.playlist_position);
    TEST_ASSERT_EQUAL_UINT32(ticks, ts_phase_offset);  // anchored at section 2 start
}

// Bar counting is correct in non-4/4 time: the arranger counts on_bar() calls
// (not ticks), so the section length in bars behaves the same regardless of metre.
void test_bar_counting_in_3_4() {
    Arranger a;
    a.setup_saveable_settings();
    a.set_playback_mode(Arranger::PLAYLIST);
    a.advance_playlist = true;
    a.song_sections[0].length         = 8;
    a.song_sections[0].bars_per_phrase = 4;
    a.song_sections[0].time_signature  = { 3, 4 };
    a.playlist.entries[0] = { 0, 1, 0 };
    a.playlist.entries[1] = { 1, 1, 0 };
    a.on_restart();

    tick_bars(&a, 4);   // half done — must NOT advance
    TEST_ASSERT_EQUAL_INT(0, a.playlist_position);
    TEST_ASSERT_EQUAL_INT(3, a.current_bar);

    tick_bars(&a, 4);   // 8 bars total → advance
    TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
    TEST_ASSERT_EQUAL_INT(0, a.current_bar);
}

// ── Entry point ────────────────────────────────────────────────────────

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_on_restart_initial_state);
    RUN_TEST(test_first_bar_after_restart_is_zero);
    RUN_TEST(test_basic_bar_advance);
    RUN_TEST(test_advance_bar_false_stays_at_zero);
    RUN_TEST(test_section_length_triggers_playlist_advance);
    RUN_TEST(test_max_bars_override);
    RUN_TEST(test_multiple_repeats);
    RUN_TEST(test_playlist_wraparound);
    RUN_TEST(test_advance_playlist_false);
    RUN_TEST(test_pending_jump_section);
    RUN_TEST(test_pending_jump_bar);
    RUN_TEST(test_time_sig_applied_on_section_change);
    RUN_TEST(test_on_after_load_applies_time_sig);
    RUN_TEST(test_disabled_noop);
    RUN_TEST(test_all_section_names_resolve);
    RUN_TEST(test_default_playlist_no_invalid_sections);
    RUN_TEST(test_full_playlist);
    RUN_TEST(test_inactive_slot_skipped);
    RUN_TEST(test_multiple_inactive_slots_skipped);
    RUN_TEST(test_all_inactive_stays_put);
    RUN_TEST(test_restart_skips_inactive_leading_slots);
    RUN_TEST(test_current_bar_synced_after_playlist_advance);
    RUN_TEST(test_bars_per_phrase_does_not_control_advance);
    RUN_TEST(test_two_repeats_standard_section);
    RUN_TEST(test_max_bars_smaller_than_bars_per_phrase);
    RUN_TEST(test_max_bars_larger_than_section_length);
    RUN_TEST(test_time_sig_applied_on_playlist_advance);
    RUN_TEST(test_time_sig_sequence_4_4_to_3_4_to_5_4);
    RUN_TEST(test_ts_phase_offset_reset_on_section_change);
    RUN_TEST(test_bar_counting_in_3_4);
    return UNITY_END();
}
