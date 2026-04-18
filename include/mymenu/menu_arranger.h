#pragma once

#ifdef ENABLE_SCREEN
#ifdef ENABLE_ARRANGER

class Menu;

// Adds arranger/progression UI pages to the existing menu.
void arranger_make_menu_items(Menu *menu);

#endif
#endif
