#include "arranger.h"

#ifdef ENABLE_SCREEN
#ifdef ENABLE_ARRANGER

#include "menu.h"
#include "submenuitem_bar.h"
#include "menuitems_lambda.h"
#include "menuitems_lambda_selector.h"
#include "mymenu/menuitems_scale.h"
#include "mymenu/menu_arranger.h"

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

void arranger_make_menu_items(Menu *menu) {
    if (menu == nullptr || arranger == nullptr) return;

    static int8_t ui_edit_section = 0;
    static int8_t ui_edit_bar = 0;
    static int8_t ui_edit_playlist_slot = 0;

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

    menu->add_page("Arrange Chord", C_WHITE, false);

    menu->add(new LambdaNumberControl<int8_t>(
        "Edit section",
        [=](int8_t v) {
            ui_edit_section = constrain(v, (int8_t)0, (int8_t)(NUM_SONG_SECTIONS - 1));
        },
        [=]() -> int8_t { return ui_edit_section; },
        nullptr,
        (int8_t)0,
        (int8_t)(NUM_SONG_SECTIONS - 1),
        true,
        true
    ));

    menu->add(new LambdaNumberControl<int8_t>(
        "Edit bar",
        [=](int8_t v) {
            ui_edit_bar = constrain(v, (int8_t)0, (int8_t)(CHORDS_PER_SECTION - 1));
        },
        [=]() -> int8_t { return ui_edit_bar; },
        nullptr,
        (int8_t)0,
        (int8_t)(CHORDS_PER_SECTION - 1),
        true,
        true
    ));

    menu->add(new CallbackMenuItem(
        "Chord editor status",
        [=]() -> const char * {
            static char msg[96];
            snprintf(
                msg,
                sizeof(msg),
                "Editing S:%d B:%d  Playing S:%d B:%d",
                ui_edit_section,
                ui_edit_bar,
                arranger->current_section,
                arranger->current_bar
            );
            return msg;
        }, false
    ));

    menu->add(new LambdaChordSubMenuItemBar(
        "Chord",
        [=](int8_t degree) {
            arranger->song_sections[ui_edit_section].grid[ui_edit_bar].degree = degree;
            arranger->mark_as_modified();
        },
        [=]() -> int8_t {
            return arranger->song_sections[ui_edit_section].grid[ui_edit_bar].degree;
        },
        [=](CHORD::Type chord_type) {
            arranger->song_sections[ui_edit_section].grid[ui_edit_bar].type = chord_type;
            arranger->mark_as_modified();
        },
        [=]() -> CHORD::Type {
            return arranger->song_sections[ui_edit_section].grid[ui_edit_bar].type;
        },
        [=](int8_t inversion) {
            arranger->song_sections[ui_edit_section].grid[ui_edit_bar].inversion = inversion;
            arranger->mark_as_modified();
        },
        [=]() -> int8_t {
            return arranger->song_sections[ui_edit_section].grid[ui_edit_bar].inversion;
        },
        false,  // don't allow global
        true,
        false   // don't show header since the chord type selector already has a header, and it would be redundant to have two
    ));

    menu->add_page("Arrange Playlist", C_WHITE, false);

    menu->add(new LambdaNumberControl<int8_t>(
        "Edit slot",
        [=](int8_t v) {
            ui_edit_playlist_slot = constrain(v, (int8_t)0, (int8_t)(NUM_PLAYLIST_SLOTS - 1));
        },
        [=]() -> int8_t { return ui_edit_playlist_slot; },
        nullptr,
        (int8_t)0,
        (int8_t)(NUM_PLAYLIST_SLOTS - 1),
        true,
        true
    ));

    menu->add(new CallbackMenuItem(
        "Playlist editor status",
        [=]() -> const char * {
            static char msg[96];
            snprintf(
                msg,
                sizeof(msg),
                "Editing slot:%d  Current slot:%d",
                ui_edit_playlist_slot,
                arranger->playlist_position
            );
            return msg;
        }, false
    ));

    menu->add(new LambdaNumberControl<int8_t>(
        "Section",
        [=](int8_t section_index) {
            arranger->playlist.entries[ui_edit_playlist_slot].section = constrain(section_index, (int8_t)0, (int8_t)(NUM_SONG_SECTIONS - 1));
            arranger->mark_as_modified();
        },
        [=]() -> int8_t {
            return arranger->playlist.entries[ui_edit_playlist_slot].section;
        },
        nullptr,
        (int8_t)0,
        (int8_t)(NUM_SONG_SECTIONS - 1),
        true,
        true
    ));

    menu->add(new LambdaNumberControl<int8_t>(
        "Repeats",
        [=](int8_t repeats) {
            arranger->playlist.entries[ui_edit_playlist_slot].repeats = constrain(repeats, (int8_t)1, (int8_t)MAX_REPEATS);
            arranger->mark_as_modified();
        },
        [=]() -> int8_t {
            return arranger->playlist.entries[ui_edit_playlist_slot].repeats;
        },
        nullptr,
        (int8_t)1,
        (int8_t)MAX_REPEATS,
        true,
        true
    ));
}

#endif
#endif
