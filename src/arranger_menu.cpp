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

// Direct-read playlist row widget. my_slot is fixed at construction; each playlist
// slot gets its own instance so scroll can start at any row without stale-slot bugs.
class PlaylistRowMenuItem : public MenuItem {
public:
    enum Field : uint8_t { FIELD_SECTION = 0, FIELD_REPEATS = 1, FIELD_MAXBARS = 2, NUM_FIELDS = 3 };
    int8_t my_slot;                    // fixed at construction
    Field  active_field = FIELD_SECTION;

    PlaylistRowMenuItem(int8_t slot) : MenuItem("Slot  0", true, false), my_slot(slot) {
        snprintf(label, MAX_LABEL_LENGTH, "Slot %2d", (int)slot);
    }

    virtual bool action_opened() override {
        active_field = FIELD_SECTION;
        return true;
    }
    virtual bool is_openable()   override { return true; }

    // Skip drawing for off-screen or partially-clipped rows. Still returns a
    // correct Y so panel_bottom[] is computed accurately for scroll accounting.
    virtual int display(Coord pos, bool selected, bool opened) override {
        tft->setTextSize(2);
        const int row_h = tft->getRowHeight();
        tft->setTextSize(0);
        if (pos.y + row_h > tft->height()) {
            // Row would be partially or fully off-screen: skip all TFT calls.
            return pos.y + row_h;
        }
        return MenuItem::display(pos, selected, opened);
    }

    virtual bool button_select() override {
        active_field = (Field)((active_field + 1) % NUM_FIELDS);
        return false; // stay open
    }
    virtual bool button_back() override { return false; }

    virtual bool knob_left() override {
        playlist_entry_t &e = arranger->playlist.entries[my_slot];
        switch (active_field) {
            case FIELD_SECTION: e.section  = (int8_t)constrain(e.section  - 1, 0, NUM_SONG_SECTIONS - 1); break;
            case FIELD_REPEATS: e.repeats  = (int8_t)constrain(e.repeats  - 1, 1, MAX_REPEATS);          break;
            case FIELD_MAXBARS: e.max_bars = (uint8_t)constrain((int)e.max_bars - 1, 0, CHORDS_PER_SECTION); break;
            default: break;
        }
        arranger->mark_as_modified();
        return true;
    }
    virtual bool knob_right() override {
        playlist_entry_t &e = arranger->playlist.entries[my_slot];
        switch (active_field) {
            case FIELD_SECTION: e.section  = (int8_t)constrain(e.section  + 1, 0, NUM_SONG_SECTIONS - 1); break;
            case FIELD_REPEATS: e.repeats  = (int8_t)constrain(e.repeats  + 1, 1, MAX_REPEATS);          break;
            case FIELD_MAXBARS: e.max_bars = (uint8_t)constrain((int)e.max_bars + 1, 0, CHORDS_PER_SECTION); break;
            default: break;
        }
        arranger->mark_as_modified();
        return true;
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        const playlist_entry_t &e = arranger->playlist.entries[my_slot];
        bool wrap_was = tft->isTextWrap();
        tft->setTextWrap(false);
        tft->setTextSize(2);

        // opened=true is only passed by the menu for the one correct index occurrence,
        // so no extra guard is needed — same pattern as ChordBarMenuItem.
        auto fcol = [this, selected, opened](Field f) {
            if (opened && active_field == f)
                tft->setTextColor(GREEN, default_bg);
            else
                colours(selected || opened);
        };

        // Fixed column widths for textsize=2 (20 chars per line on this screen).
        // cs = 2-digit slot index + space (e.g. " 0 ")
        // c0 = section name, c1 = repeats, c2 = max_bars, c3 = active indicator
        // cs(3) + c0(8) + c1(3) + c2(3) + c3(3) = 20
        const int cs =  3;   // slot index    (e.g. " 0 ")
        const int c0 =  8;   // section name  (e.g. "Chorus 2")
        const int c1 =  3;   // repeats       (e.g. " 64")
        const int c2 =  3;   // max_bars      (e.g. "  8")
        const int c3 =  3;   // active indicator + padding

        char slot_buf[8];
        colours(selected || opened);
        snprintf(slot_buf, sizeof(slot_buf), "%*d ", cs - 1, (int)my_slot);
        tft->print(slot_buf);
        char sec_buf[16];
        fcol(FIELD_SECTION);
        snprintf(sec_buf, sizeof(sec_buf), "%-*s", c0, get_section_name(e.section));
        tft->print(sec_buf);
        char rep_buf[8];
        fcol(FIELD_REPEATS);
        snprintf(rep_buf, sizeof(rep_buf), "%*d", c1, (int)e.repeats);
        tft->print(rep_buf);
        char mb_buf[8];
        fcol(FIELD_MAXBARS);
        snprintf(mb_buf, sizeof(mb_buf), "%*d", c2, (int)e.max_bars);
        tft->print(mb_buf);
        colours(selected || opened);
        // Active indicator in remaining space
        char ind_buf[8];
        snprintf(ind_buf, sizeof(ind_buf), "%-*s", c3, (arranger->playlist_position == my_slot) ? " * " : "   ");
        tft->print(ind_buf);

        tft->setTextWrap(wrap_was);
        return tft->getCursorY();
    }
};



// Invisible menu item: when display() is called it updates shared_edit_section and
// every shared ChordBarMenuItem's my_section field to this page's section index.
// No screen space is consumed (returns pos.y unchanged). selectable=false so the
// menu navigator skips it entirely.
class SectionPageSwitcherMenuItem : public MenuItem {
public:
    int8_t           section_idx;
    int8_t          *target;
    ChordBarMenuItem **chord_rows;
    int              num_rows;

    SectionPageSwitcherMenuItem(
        int8_t section_idx, int8_t *target,
        ChordBarMenuItem **chord_rows, int num_rows
    ) : MenuItem("", false, false),
        section_idx(section_idx), target(target),
        chord_rows(chord_rows), num_rows(num_rows)
    {
        this->selectable = false;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        *target = section_idx;
        for (int j = 0; j < num_rows; j++) {
            if (chord_rows[j]) chord_rows[j]->my_section = section_idx;
        }
        return pos.y;
    }
};

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

    // Single shared buffer for all five overview lines — safe since only one renders per frame
    static char arranger_overview_buf[64];

    menu->add(new CallbackMenuItem(
        "Overview line 1",
        []() -> const char * {
            snprintf(arranger_overview_buf, sizeof(arranger_overview_buf),
                "Play S:%d B:%d Slot:%d",
                arranger->current_section, arranger->current_bar, arranger->playlist_position);
            return arranger_overview_buf;
        }, false
    ));

    menu->add(new CallbackMenuItem(
        "Overview line 2",
        []() -> const char * {
            snprintf(arranger_overview_buf, sizeof(arranger_overview_buf),
                "%s | %s | %s",
                arranger->is_enabled() ? "Enabled" : "Disabled",
                label_playback_mode(arranger->get_playback_mode()),
                label_progression_cadence(arranger->get_progression_cadence()));
            return arranger_overview_buf;
        }, false
    ));

    menu->add(new CallbackMenuItem(
        "Overview line 3",
        []() -> const char * {
            const chord_identity_t &ch = arranger->get_current_chord();
            snprintf(arranger_overview_buf, sizeof(arranger_overview_buf),
                "Chord d:%d t:%d i:%d", ch.degree, (int)ch.type, ch.inversion);
            return arranger_overview_buf;
        }, false
    ));

    menu->add(new SeparatorMenuItem("Chord progression", C_WHITE));

    menu->add(new CallbackMenuItem(
        "Overview line 4",
        []() -> const char * {
            for (int i = 0; i < CHORDS_PER_SECTION/2; i++) {
                const chord_identity_t &ch = arranger->song_sections[arranger->current_section].grid[i];
                arranger_overview_buf[i*7+0] = arranger->current_bar == i ? '>' : ' ';
                arranger_overview_buf[i*7+1] = ch.degree >= 0 ? '0' + ch.degree : '-';
                arranger_overview_buf[i*7+2] = ':';
                arranger_overview_buf[i*7+3] = 'A' + (int)ch.type;
                arranger_overview_buf[i*7+4] = 'i';
                arranger_overview_buf[i*7+5] = '0' + ch.inversion;
                arranger_overview_buf[i*7+6] = ' ';
            }
            arranger_overview_buf[CHORDS_PER_SECTION/2 * 7] = '\0';
            return arranger_overview_buf;
        }, false
    ));

    menu->add(new CallbackMenuItem(
        "Overview line 5",
        []() -> const char * {
            for (int i = CHORDS_PER_SECTION/2; i < CHORDS_PER_SECTION; i++) {
                const chord_identity_t &ch = arranger->song_sections[arranger->current_section].grid[i];
                arranger_overview_buf[(i-CHORDS_PER_SECTION/2)*7+0] = arranger->current_bar == i ? '>' : ' ';
                arranger_overview_buf[(i-CHORDS_PER_SECTION/2)*7+1] = ch.degree >= 0 ? '0' + ch.degree : '-';
                arranger_overview_buf[(i-CHORDS_PER_SECTION/2)*7+2] = ':';
                arranger_overview_buf[(i-CHORDS_PER_SECTION/2)*7+3] = 'A' + (int)ch.type;
                arranger_overview_buf[(i-CHORDS_PER_SECTION/2)*7+4] = 'i';
                arranger_overview_buf[(i-CHORDS_PER_SECTION/2)*7+5] = '0' + ch.inversion;
                arranger_overview_buf[(i-CHORDS_PER_SECTION/2)*7+6] = ' ';
            }
            arranger_overview_buf[CHORDS_PER_SECTION/2 * 7] = '\0';
            return arranger_overview_buf;
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

    // Playlist page: shared-bar pattern — 16 lightweight switcher items + ONE shared editor bar.
    menu->add_page("Playlist", colour, true);
    menu->set_page_header("## Section Rep Max  ");
    {
        for (int i = 0; i < NUM_PLAYLIST_SLOTS; i++) {
            menu->add(new PlaylistRowMenuItem((int8_t)i));
        }

        if (save_load_bar) menu->add(save_load_bar);
        menu->add(advance_bar);
    }

    // Per-section pages: shared-object approach — one set of controls shared across all
    // NUM_SONG_SECTIONS pages. A SectionPageSwitcherMenuItem at the top of each page
    // hot-swaps shared_edit_section and each ChordBarMenuItem's my_section before
    // any row renders, so every page shows and edits the correct section's data.
    if (!compact_sections) {
        static int8_t shared_edit_section = 0;
        const int num_shared_rows = two_column ? CHORDS_PER_SECTION / 2 : CHORDS_PER_SECTION;
        ChordBarMenuItem **shared_chord_rows = new ChordBarMenuItem*[num_shared_rows];

        // Shared metadata bar — lambdas access shared_edit_section directly (static local)
        SubMenuItemBar *shared_meta_bar = new SubMenuItemBar("Section props", true, true);
        shared_meta_bar->add(new LambdaNumberControl<uint8_t>(
            "Length",
            [](uint8_t v) { arranger->song_sections[shared_edit_section].length = (uint8_t)constrain((int)v, 1, CHORDS_PER_SECTION); arranger->mark_as_modified(); },
            []() -> uint8_t { return arranger->song_sections[shared_edit_section].length; },
            nullptr, (uint8_t)1, (uint8_t)CHORDS_PER_SECTION, true, true
        ));
        shared_meta_bar->add(new LambdaNumberControl<uint8_t>(
            "Bars-per-phrase",
            [](uint8_t v) { arranger->song_sections[shared_edit_section].bars_per_phrase = (uint8_t)constrain((int)v, 1, 64); arranger->mark_as_modified(); },
            []() -> uint8_t { return arranger->song_sections[shared_edit_section].bars_per_phrase; },
            nullptr, (uint8_t)1, (uint8_t)64, true, true
        ));

        #ifdef ENABLE_TIME_SIGNATURE
            SubMenuItemBar *shared_ts_bar = new SubMenuItemBar("Time sig", true, true);
            shared_ts_bar->add(new LambdaNumberControl<uint8_t>(
                "Numerator",
                [](uint8_t v) { arranger->song_sections[shared_edit_section].time_signature.numerator = (uint8_t)constrain((int)v, 1, TIME_SIG_MAX_STEPS_PER_BAR); arranger->mark_as_modified(); },
                []() -> uint8_t { return arranger->song_sections[shared_edit_section].time_signature.numerator; },
                nullptr, (uint8_t)1, (uint8_t)TIME_SIG_MAX_STEPS_PER_BAR, true, true
            ));
            shared_ts_bar->add(new LambdaNumberControl<uint8_t>(
                "Denominator",
                [](uint8_t v) { arranger->song_sections[shared_edit_section].time_signature.denominator = (uint8_t)constrain((int)v, 1, 32); arranger->mark_as_modified(); },
                []() -> uint8_t { return arranger->song_sections[shared_edit_section].time_signature.denominator; },
                nullptr, (uint8_t)1, (uint8_t)32, true, true
            ));
        #endif

        // Shared copy/paste bar
        SubMenuItemBar *shared_cp_bar = new SubMenuItemBar("Copy/Paste", false, true);
        shared_cp_bar->add(new LambdaActionItem("Copy",  []() { arranger->copy_section(shared_edit_section); }));
        shared_cp_bar->add(new LambdaActionItem("Paste", []() { arranger->paste_section(shared_edit_section); }));

        // Shared chord rows and column header
        MenuItem *shared_header_row;
        if (two_column) {
            shared_header_row = new SeparatorMenuItem("D Chord i D Chord i", menu->tft->halfbright_565(C_WHITE), 2, false);
            for (int k = 0; k < num_shared_rows; k++) {
                const int j  = k * 2;
                const int j2 = j + 1;
                char lbl[12];
                snprintf(lbl, sizeof(lbl), "Bars %d+%d", j, j2);
                shared_chord_rows[k] = new ChordBarMenuItem(
                    lbl,
                    // left bar (j) — capture j by value; shared_edit_section is static, no capture needed
                    [j](int8_t d)        { arranger->song_sections[shared_edit_section].grid[j].degree = d; arranger->mark_as_modified(); },
                    [j]() -> int8_t      { return arranger->song_sections[shared_edit_section].grid[j].degree; },
                    [j](CHORD::Type t)   { arranger->song_sections[shared_edit_section].grid[j].type = t; arranger->mark_as_modified(); },
                    [j]() -> CHORD::Type { return arranger->song_sections[shared_edit_section].grid[j].type; },
                    [j](int8_t inv)      { arranger->song_sections[shared_edit_section].grid[j].inversion = inv; arranger->mark_as_modified(); },
                    [j]() -> int8_t      { return arranger->song_sections[shared_edit_section].grid[j].inversion; },
                    (int8_t)0, (int8_t)j,  // my_section=0 placeholder; updated by SectionPageSwitcherMenuItem
                    // right bar (j2)
                    [j2](int8_t d)        { arranger->song_sections[shared_edit_section].grid[j2].degree = d; arranger->mark_as_modified(); },
                    [j2]() -> int8_t      { return arranger->song_sections[shared_edit_section].grid[j2].degree; },
                    [j2](CHORD::Type t)   { arranger->song_sections[shared_edit_section].grid[j2].type = t; arranger->mark_as_modified(); },
                    [j2]() -> CHORD::Type { return arranger->song_sections[shared_edit_section].grid[j2].type; },
                    [j2](int8_t inv)      { arranger->song_sections[shared_edit_section].grid[j2].inversion = inv; arranger->mark_as_modified(); },
                    [j2]() -> int8_t      { return arranger->song_sections[shared_edit_section].grid[j2].inversion; },
                    (int8_t)j2,
                    &arranger->current_section, &arranger->current_bar
                );
            }
        } else {
            shared_header_row = new SeparatorMenuItem("D Chord Inv", menu->tft->halfbright_565(C_WHITE), 2, false);
            for (int j = 0; j < num_shared_rows; j++) {
                char lbl[8];
                snprintf(lbl, sizeof(lbl), "Bar %d", j);
                shared_chord_rows[j] = new ChordBarMenuItem(
                    lbl,
                    [j](int8_t d)        { arranger->song_sections[shared_edit_section].grid[j].degree = d; arranger->mark_as_modified(); },
                    [j]() -> int8_t      { return arranger->song_sections[shared_edit_section].grid[j].degree; },
                    [j](CHORD::Type t)   { arranger->song_sections[shared_edit_section].grid[j].type = t; arranger->mark_as_modified(); },
                    [j]() -> CHORD::Type { return arranger->song_sections[shared_edit_section].grid[j].type; },
                    [j](int8_t inv)      { arranger->song_sections[shared_edit_section].grid[j].inversion = inv; arranger->mark_as_modified(); },
                    [j]() -> int8_t      { return arranger->song_sections[shared_edit_section].grid[j].inversion; },
                    (int8_t)0, (int8_t)j,  // my_section=0 placeholder; updated by SectionPageSwitcherMenuItem
                    &arranger->current_section, &arranger->current_bar
                );
            }
        }

        // Section pages: the loop is now cheap — only switcher objects are allocated per page
        for (int i = 0; i < NUM_SONG_SECTIONS; i++) {
            menu->add_page(get_section_name(i), colour, true);
            menu->add(new SectionPageSwitcherMenuItem(
                (int8_t)i, &shared_edit_section, shared_chord_rows, num_shared_rows
            ));
            menu->add(shared_meta_bar);
            #ifdef ENABLE_TIME_SIGNATURE
                menu->add(shared_ts_bar);
            #endif
            menu->add(shared_cp_bar);
            menu->add(shared_header_row);
            for (int k = 0; k < num_shared_rows; k++) {
                menu->add(shared_chord_rows[k]);
            }
            if (save_load_bar) menu->add(save_load_bar);
            menu->add(advance_bar);
        }
    }
}

#endif
#endif
