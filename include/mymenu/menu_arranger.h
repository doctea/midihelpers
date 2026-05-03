#pragma once

#ifdef ENABLE_SCREEN
#ifdef ENABLE_ARRANGER

#include <functional-vlpp.h>
#include "menu.h"

// Adds arranger/progression UI pages to the existing menu.
// compact_sections: when true, use a single chord-editor page instead of one page
//   per section (saves ~150+ heap objects; suitable for memory-constrained targets).
// two_column: when true (and compact_sections=false), pair each two bars onto one row
//   using ChordBarMenuItem two-column mode — halves the row count per section page.
// colour:   page header colour (e.g. this->colour from a behaviour, or C_WHITE).
// save_cb / load_cb: optional save/load callbacks; omit to suppress Save/Load controls.
void arranger_make_menu_items(Menu *menu,
    bool compact_sections = false,
    bool two_column = false,
    uint16_t colour = C_WHITE,
    vl::Func<void()> save_cb = vl::Func<void()>(),
    vl::Func<void()> load_cb = vl::Func<void()>());

#endif
#endif
