#include "arranger.h"

#ifdef ENABLE_SCREEN
#ifdef ENABLE_ARRANGER

#include "menu.h"
#include "submenuitem_bar.h"
#include "menuitems_lambda.h"
#include "menuitems_lambda_selector.h"
#include "mymenu/menuitems_scale.h"
#include "mymenu/menu_arranger.h"
#include "mymenu/menuitem_chord_bar.h"
#include "mymenu/menuitem_song_section.h"

// Out-of-class definition for SongSectionSelectorItem's shared options list
LinkedList<LambdaSelectorControl<int8_t>::option> *SongSectionSelectorItem::song_section_options = nullptr;

static const char *label_progression_cadence(Arranger::progression_cadence_t cadence) {
    switch (cadence) {
        case Arranger::PROGRESSION_PER_BAR: return "Per bar";
        case Arranger::PROGRESSION_PER_PHRASE: return "Per phrase";
        default: return "Unknown";
    }
}

static const char *label_playback_mode(Arranger::playback_mode_t mode) {
    switch (mode) {
        case Arranger::LOOP_SECTION: return "Loop section";
        case Arranger::PLAYLIST: return "Playlist";
        default: return "Unknown";
    }
}

void arranger_make_menu_items(Menu *menu, bool compact_sections, bool two_column, uint16_t colour, vl::Func<void()> save_cb, vl::Func<void()> load_cb) {
    if (menu == nullptr || arranger == nullptr) return;

    menu->add_page("Arrange", C_WHITE, false);

    menu->add(new CallbackMenuItem(
        "Overview line 1",
        [=]() -> const char * {
            static char msg[64];
            snprintf(
                msg,
                sizeof(msg),
                "Play S:%d B:%d Slot:%d",
                arranger->current_section,
                arranger->current_bar,
                arranger->playlist_position
            );
            return msg;
        }, false
    ));

    menu->add(new CallbackMenuItem(
        "Overview line 2",
        [=]() -> const char * {
            static char msg[64];
            snprintf(
                msg,
                sizeof(msg),
                "%s | %s | %s",
                arranger->is_enabled() ? "Enabled" : "Disabled",
                label_playback_mode(arranger->get_playback_mode()),
                label_progression_cadence(arranger->get_progression_cadence())
            );
            return msg;
        }, false
    ));

    menu->add(new CallbackMenuItem(
        "Overview line 3",
        [=]() -> const char * {
            static char msg[64];
            const chord_identity_t &ch = arranger->get_current_chord();
            snprintf(msg, sizeof(msg), "Chord d:%d t:%d i:%d", ch.degree, (int)ch.type, ch.inversion);
            return msg;
        }, false
    ));

    menu->add(new SeparatorMenuItem("Chord progression", C_WHITE));

    menu->add(new CallbackMenuItem(
        "Overview line 4",
        [=]() -> const char * {
            static char msg[64];
            for (int i = 0 ; i < CHORDS_PER_SECTION/2 ; i++) {
                const chord_identity_t &ch = arranger->song_sections[arranger->current_section].grid[i];
                msg[i*7 + 0] = arranger->current_bar == i ? '>' : ' ';
                msg[i*7 + 1] = ch.degree >= 0 ? '0' + ch.degree : '-';
                msg[i*7 + 2] = ':';
                msg[i*7 + 3] = 'A' + (int)ch.type;
                msg[i*7 + 4] = 'i';
                msg[i*7 + 5] = '0' + ch.inversion;
                msg[i*7 + 6] = ' ';
            }
            msg[CHORDS_PER_SECTION/2 * 7] = '\0';
            return msg;
         }, false
    ));

    menu->add(new CallbackMenuItem(
        "Overview line 5",
        [=]() -> const char * {
            static char msg[64];
            for (int i = CHORDS_PER_SECTION/2 ; i < CHORDS_PER_SECTION ; i++) {
                const chord_identity_t &ch = arranger->song_sections[arranger->current_section].grid[i];
                msg[(i - CHORDS_PER_SECTION/2)*7 + 0] = arranger->current_bar == i ? '>' : ' ';
                msg[(i - CHORDS_PER_SECTION/2)*7 + 1] = ch.degree >= 0 ? '0' + ch.degree : '-';
                msg[(i - CHORDS_PER_SECTION/2)*7 + 2] = ':';
                msg[(i - CHORDS_PER_SECTION/2)*7 + 3] = 'A' + (int)ch.type;
                msg[(i - CHORDS_PER_SECTION/2)*7 + 4] = 'i';
                msg[(i - CHORDS_PER_SECTION/2)*7 + 5] = '0' + ch.inversion;
                msg[(i - CHORDS_PER_SECTION/2)*7 + 6] = ' ';
            }
            msg[CHORDS_PER_SECTION/2 * 7] = '\0';
            return msg;
         }, false
    ));

    menu->add_page("Arrange Play", C_WHITE, false);

    menu->add(new LambdaToggleControl(
        "Enabled",
        [=](bool v) { arranger->set_enabled(v); },
        [=]() -> bool { return arranger->is_enabled(); }
    ));

    LambdaSelectorControl<int> *mode_selector = new LambdaSelectorControl<int>(
        "Mode",
        [=](int v) { arranger->set_playback_mode((Arranger::playback_mode_t)constrain(v, 0, 1)); },
        [=]() -> int { return (int)arranger->get_playback_mode(); },
        nullptr,
        true,
        true
    );
    mode_selector->add_available_value((int)Arranger::LOOP_SECTION, "Loop section");
    mode_selector->add_available_value((int)Arranger::PLAYLIST, "Playlist");
    menu->add(mode_selector);

    LambdaSelectorControl<int> *cadence_selector = new LambdaSelectorControl<int>(
        "Cadence",
        [=](int v) { arranger->set_progression_cadence((Arranger::progression_cadence_t)constrain(v, 0, 1)); },
        [=]() -> int { return (int)arranger->get_progression_cadence(); },
        nullptr,
        true,
        true
    );
    cadence_selector->add_available_value((int)Arranger::PROGRESSION_PER_BAR, "Per bar");
    cadence_selector->add_available_value((int)Arranger::PROGRESSION_PER_PHRASE, "Per phrase");
    menu->add(cadence_selector);

    menu->add_page("Arrange Advance", C_WHITE, false);

    menu->add(new LambdaToggleControl(
        "Advance chord",
        [=](bool v) { arranger->advance_bar = v; arranger->mark_as_modified(); },
        [=]() -> bool { return arranger->advance_bar; }
    ));

    menu->add(new LambdaToggleControl(
        "Advance playlist",
        [=](bool v) { arranger->advance_playlist = v; arranger->mark_as_modified(); },
        [=]() -> bool { return arranger->advance_playlist; }
    ));

    menu->add(new LambdaActionItem(
        "Restart",
        [=]() { arranger->on_restart(); }
    ));

    // ── Arrange Jump page ────────────────────────────────────────────────
    menu->add_page("Arrange Jump", C_WHITE, false);

    {
        static int8_t jump_section = 0;
        static int8_t jump_bar     = 0;

        menu->add(new SongSectionSelectorItem(
            "Jump section",
            [=](int8_t v) { jump_section = constrain((int)v, 0, NUM_SONG_SECTIONS - 1); },
            [=]() -> int8_t { return jump_section; }
        ));

        menu->add(new LambdaNumberControl<int8_t>(
            "Jump bar",
            [=](int8_t v) { jump_bar = constrain((int)v, 0, CHORDS_PER_SECTION - 1); },
            [=]() -> int8_t { return jump_bar; },
            nullptr,
            (int8_t)0, (int8_t)(CHORDS_PER_SECTION - 1), true, true
        ));

        menu->add(new LambdaActionItem(
            "Jump Now",
            [=]() { arranger->change_section(jump_section); arranger->move_bar(jump_bar); }
        ));

        menu->add(new LambdaActionItem(
            "Queue Jump (bar-sync)",
            [=]() { arranger->queue_jump(jump_section, jump_bar); }
        ));

        menu->add(new LambdaActionItem(
            "Enter Loop Section",
            [=]() { arranger->enter_loop_section(); }
        ));

        menu->add(new LambdaActionItem(
            "Loop This Bar",
            [=]() { arranger->loop_current_bar(); }
        ));

        menu->add(new LambdaNumberControl<int8_t>(
            "Loop start bar",
            [=](int8_t v) { arranger->set_loop_range(v, arranger->loop_end_bar); },
            [=]() -> int8_t { return arranger->loop_start_bar; },
            nullptr,
            (int8_t)0, (int8_t)(CHORDS_PER_SECTION - 1), true, true
        ));

        menu->add(new LambdaNumberControl<int8_t>(
            "Loop end bar",
            [=](int8_t v) { arranger->set_loop_range(arranger->loop_start_bar, v); },
            [=]() -> int8_t { return arranger->loop_end_bar; },
            nullptr,
            (int8_t)0, (int8_t)(CHORDS_PER_SECTION - 1), true, true
        ));

        menu->add(new LambdaActionItem(
            "Exit Loop -> Playlist",
            [=]() { arranger->exit_loop_to_playlist(); }
        ));
    }

    // Build reusable save/load bar (only when callbacks are provided)
    SubMenuItemBar *save_load_bar = nullptr;
    if (save_cb || load_cb) {
        save_load_bar = new SubMenuItemBar("Section controls", false, true);
        if (save_cb) save_load_bar->add(new LambdaActionConfirmItem("Save", [=]() { save_cb(); }));
        save_load_bar->add(new CallbackMenuItem(
            "State",
            [=]() -> const char* { return arranger->has_changes_to_save() ? "Unsaved" : "Saved"; },
            [=]() -> uint16_t { return arranger->has_changes_to_save() ? RED : GREEN; },
            false
        ));
        if (load_cb) save_load_bar->add(new LambdaActionConfirmItem("Load", [=]() { load_cb(); }));
    }

    // Build reusable advance-flags bar
    SubMenuItemBar *advance_bar = new SubMenuItemBar("Advance controls", false, true);
    advance_bar->add(new LambdaToggleControl("Advance bar",
        [=](bool v) { arranger->advance_bar = v; arranger->mark_as_modified(); },
        [=]() -> bool { return arranger->advance_bar; }
    ));
    advance_bar->add(new LambdaToggleControl("Advance playlist",
        [=](bool v) { arranger->advance_playlist = v; arranger->mark_as_modified(); },
        [=]() -> bool { return arranger->advance_playlist; }
    ));

    // Compact mode: single chord-editor page for all sections (memory-saving alternative)
    if (compact_sections) {
        static int8_t ui_edit_section = 0;
        static int8_t ui_edit_bar = 0;

        menu->add_page("Arrange Chord", colour, false);

        menu->add(new LambdaNumberControl<int8_t>(
            "Edit section",
            [=](int8_t v) { ui_edit_section = constrain(v, (int8_t)0, (int8_t)(NUM_SONG_SECTIONS - 1)); },
            [=]() -> int8_t { return ui_edit_section; },
            nullptr,
            (int8_t)0, (int8_t)(NUM_SONG_SECTIONS - 1), true, true
        ));

        menu->add(new LambdaNumberControl<int8_t>(
            "Edit bar",
            [=](int8_t v) { ui_edit_bar = constrain(v, (int8_t)0, (int8_t)(CHORDS_PER_SECTION - 1)); },
            [=]() -> int8_t { return ui_edit_bar; },
            nullptr,
            (int8_t)0, (int8_t)(CHORDS_PER_SECTION - 1), true, true
        ));

        menu->add(new CallbackMenuItem(
            "Chord editor status",
            [=]() -> const char * {
                static char msg[96];
                snprintf(msg, sizeof(msg), "Editing S:%d B:%d  Playing S:%d B:%d",
                    ui_edit_section, ui_edit_bar,
                    arranger->current_section, arranger->current_bar);
                return msg;
            }, false
        ));

        menu->add(new LambdaChordSubMenuItemBar(
            "Chord",
            [=](int8_t degree) { arranger->song_sections[ui_edit_section].grid[ui_edit_bar].degree = degree; arranger->mark_as_modified(); },
            [=]() -> int8_t { return arranger->song_sections[ui_edit_section].grid[ui_edit_bar].degree; },
            [=](CHORD::Type chord_type) { arranger->song_sections[ui_edit_section].grid[ui_edit_bar].type = chord_type; arranger->mark_as_modified(); },
            [=]() -> CHORD::Type { return arranger->song_sections[ui_edit_section].grid[ui_edit_bar].type; },
            [=](int8_t inversion) { arranger->song_sections[ui_edit_section].grid[ui_edit_bar].inversion = inversion; arranger->mark_as_modified(); },
            [=]() -> int8_t { return arranger->song_sections[ui_edit_section].grid[ui_edit_bar].inversion; },
            false, true, false
        ));

        if (save_load_bar) menu->add(save_load_bar);
        menu->add(advance_bar);
    }

    // Playlist page: one row per playlist slot
    menu->add_page("Playlist", colour, true);
    menu->add(new MenuItem("Section      Repeats  MaxBars  ", false, true));
    for (int i = 0; i < NUM_PLAYLIST_SLOTS; i++) {
        SubMenuItemBar *slot_bar = new SubMenuItemBar(
            (String("Slot ") + String(i)).c_str(), false, true
        );
        slot_bar->add(new SongSectionSelectorItem(
            "Section",
            [=](int8_t v) { arranger->playlist.entries[i].section = constrain((int)v, 0, NUM_SONG_SECTIONS-1); arranger->mark_as_modified(); },
            [=]() -> int8_t { return arranger->playlist.entries[i].section; }
        ));
        slot_bar->add(new LambdaNumberControl<int8_t>(
            "Repeats",
            [=](int8_t v) { arranger->playlist.entries[i].repeats = constrain((int)v, 1, MAX_REPEATS); arranger->mark_as_modified(); },
            [=]() -> int8_t { return arranger->playlist.entries[i].repeats; },
            nullptr, (int8_t)1, (int8_t)MAX_REPEATS, true, true
        ));
        slot_bar->add(new LambdaNumberControl<uint8_t>(
            "MaxBars",
            [=](uint8_t v) { arranger->playlist.entries[i].max_bars = constrain((int)v, 0, CHORDS_PER_SECTION); arranger->mark_as_modified(); },
            [=]() -> uint8_t { return arranger->playlist.entries[i].max_bars; },
            nullptr, (uint8_t)0, (uint8_t)CHORDS_PER_SECTION, true, true
        ));
        // Active indicator
        slot_bar->add(new CallbackMenuItem(
            "cur?",
            [=]() -> const char* { return (arranger->playlist_position == i) ? "*" : " "; },
            [=]() -> uint16_t    { return (arranger->playlist_position == i) ? GREEN : C_WHITE; },
            false
        ));
        menu->add(slot_bar);
    }
    if (save_load_bar) menu->add(save_load_bar);
    menu->add(advance_bar);

    // Rich mode: one page per song section, one row per bar (or two bars per row in two_column mode)
    if (!compact_sections) for (int i = 0; i < NUM_SONG_SECTIONS; i++) {
        // Page title uses hardcoded section name
        menu->add_page(get_section_name(i), colour, false);

        // Section metadata bar: length and bars_per_phrase
        {
            SubMenuItemBar *meta_bar = new SubMenuItemBar("Section props", false, true);
            meta_bar->add(new LambdaNumberControl<uint8_t>(
                "Length",
                [=](uint8_t v) { arranger->song_sections[i].length = (uint8_t)constrain((int)v, 1, CHORDS_PER_SECTION); arranger->mark_as_modified(); },
                [=]() -> uint8_t { return arranger->song_sections[i].length; },
                nullptr, (uint8_t)1, (uint8_t)CHORDS_PER_SECTION, true, true
            ));
            meta_bar->add(new LambdaNumberControl<uint8_t>(
                "Phrase",
                [=](uint8_t v) { arranger->song_sections[i].bars_per_phrase = (uint8_t)constrain((int)v, 1, 64); arranger->mark_as_modified(); },
                [=]() -> uint8_t { return arranger->song_sections[i].bars_per_phrase; },
                nullptr, (uint8_t)1, (uint8_t)64, true, true
            ));
            menu->add(meta_bar);
        }

        #ifdef ENABLE_TIME_SIGNATURE
            {
                SubMenuItemBar *ts_bar = new SubMenuItemBar("Time sig", false, true);
                ts_bar->add(new LambdaNumberControl<uint8_t>(
                    "Numerator",
                    [=](uint8_t v) { arranger->song_sections[i].time_signature.numerator = (uint8_t)constrain((int)v, 1, TIME_SIG_MAX_STEPS_PER_BAR); arranger->mark_as_modified(); },
                    [=]() -> uint8_t { return arranger->song_sections[i].time_signature.numerator; },
                    nullptr, (uint8_t)1, (uint8_t)TIME_SIG_MAX_STEPS_PER_BAR, true, true
                ));
                ts_bar->add(new LambdaNumberControl<uint8_t>(
                    "Denominator",
                    [=](uint8_t v) { arranger->song_sections[i].time_signature.denominator = (uint8_t)constrain((int)v, 1, 32); arranger->mark_as_modified(); },
                    [=]() -> uint8_t { return arranger->song_sections[i].time_signature.denominator; },
                    nullptr, (uint8_t)1, (uint8_t)32, true, true
                ));
                menu->add(ts_bar);
            }
        #endif

        // Copy / paste bar
        {
            SubMenuItemBar *cp_bar = new SubMenuItemBar("Copy/Paste", false, true);
            cp_bar->add(new LambdaActionItem("Copy",  [=]() { arranger->copy_section(i); }));
            cp_bar->add(new LambdaActionItem("Paste", [=]() { arranger->paste_section(i); }));
            menu->add(cp_bar);
        }

        if (two_column) {
            menu->add(new MenuItem("D Chord Inv|D Chord Inv", false, true));
            for (int j = 0; j < CHORDS_PER_SECTION; j += 2) {
                const int j2 = j + 1;
                menu->add(new ChordBarMenuItem(
                    (String("Bars ") + String(j) + "+" + String(j2)).c_str(),
                    // left bar (j)
                    [=](int8_t d) { arranger->song_sections[i].grid[j].degree = d; arranger->mark_as_modified(); },
                    [=]() -> int8_t { return arranger->song_sections[i].grid[j].degree; },
                    [=](CHORD::Type t) { arranger->song_sections[i].grid[j].type = t; arranger->mark_as_modified(); },
                    [=]() -> CHORD::Type { return arranger->song_sections[i].grid[j].type; },
                    [=](int8_t inv) { arranger->song_sections[i].grid[j].inversion = inv; arranger->mark_as_modified(); },
                    [=]() -> int8_t { return arranger->song_sections[i].grid[j].inversion; },
                    i, j,
                    // right bar (j2)
                    [=](int8_t d) { arranger->song_sections[i].grid[j2].degree = d; arranger->mark_as_modified(); },
                    [=]() -> int8_t { return arranger->song_sections[i].grid[j2].degree; },
                    [=](CHORD::Type t) { arranger->song_sections[i].grid[j2].type = t; arranger->mark_as_modified(); },
                    [=]() -> CHORD::Type { return arranger->song_sections[i].grid[j2].type; },
                    [=](int8_t inv) { arranger->song_sections[i].grid[j2].inversion = inv; arranger->mark_as_modified(); },
                    [=]() -> int8_t { return arranger->song_sections[i].grid[j2].inversion; },
                    j2,
                    &arranger->current_section, &arranger->current_bar
                ));
            }
        } else {
            menu->add(new MenuItem("Degree      Type        Inversion", false, true));
            for (int j = 0; j < CHORDS_PER_SECTION; j++) {
                menu->add(new ChordBarMenuItem(
                    (String("Bar ") + String(j)).c_str(),
                    [=](int8_t degree) { arranger->song_sections[i].grid[j].degree = degree; arranger->mark_as_modified(); },
                    [=]() -> int8_t { return arranger->song_sections[i].grid[j].degree; },
                    [=](CHORD::Type chord_type) { arranger->song_sections[i].grid[j].type = chord_type; arranger->mark_as_modified(); },
                    [=]() -> CHORD::Type { return arranger->song_sections[i].grid[j].type; },
                    [=](int8_t inversion) { arranger->song_sections[i].grid[j].inversion = inversion; arranger->mark_as_modified(); },
                    [=]() -> int8_t { return arranger->song_sections[i].grid[j].inversion; },
                    i, j, &arranger->current_section, &arranger->current_bar
                ));
            }
        }

        if (save_load_bar) menu->add(save_load_bar);
        menu->add(advance_bar);
    }
}

#endif
#endif
