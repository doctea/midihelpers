#pragma once

#ifdef ENABLE_SCREEN
#ifdef ENABLE_ARRANGER

#include <functional-vlpp.h>
#include "menu.h"

// Adds arranger/progression UI pages to the existing menu.
// colour:   page header colour (e.g. this->colour from a behaviour, or C_WHITE).
// save_cb / load_cb: optional save/load callbacks; omit to suppress Save/Load controls.
void arranger_make_menu_items(Menu *menu,
    uint16_t colour = C_WHITE,
    vl::Func<void()> save_cb = vl::Func<void()>(),
    vl::Func<void()> load_cb = vl::Func<void()>());

#endif
#endif
