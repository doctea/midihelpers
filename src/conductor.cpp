// conductor.cpp
// Provides the global Conductor singleton.
// Include this in exactly one translation unit per project.
#include "conductor.h"

Conductor *conductor = nullptr;  // defined here, declared extern in conductor.h

// Defines Conductor::create_menu_items().
// Kept in a separate translation unit so that the menu headers (which have
// ordering dependencies) are never pulled into conductor.h.

#ifdef ENABLE_SCREEN

// Menu prerequisites — order matters
#include "menu.h"
#include "submenuitem_bar.h"
#include "menuitems_action.h"
#include "mymenu/menu_bpm.h"
#include "mymenu/menu_clock_source.h"
#ifdef ENABLE_SCALES
    #include "mymenu/menuitems_scale.h"
    #include "mymenu/menuitems_harmony.h"
#endif

// Now include the conductor types (all menu types already defined above)
#include "mymenu/menu_conductor.h"  // ConductorTransportBar + conductor extern

void Conductor::make_menu_items(Menu *menu, conductor_menu_options_t options) {
    // BPM + song position display
    menu->add(new BPMPositionIndicator());
    // Clock source selector
    menu->add(new ClockSourceSelectorControl("Clock source", clock_mode));
    // Transport controls (Start / Stop / Continue / Restart)
    menu->add(new ConductorTransportBar());

    #if defined(ENABLE_TIME_SIGNATURE) || defined(ENABLE_SCALES)
        if (!(options & COMBINE_MUSE))
            menu->add_page("Muse", C_WHITE, false);

        #ifdef ENABLE_TIME_SIGNATURE
            menu->add(new TimeSignatureIndicator());
        #endif

        #ifdef ENABLE_SCALES
            menu->add(new LambdaScaleMenuItemBar(
                "Global Scale",
                [](scale_index_t t)   { conductor->set_scale_type(t); },
                []() -> scale_index_t { return conductor->get_scale_type(); },
                [](int8_t r)          { conductor->set_scale_root(r); },
                []() -> int8_t        { return conductor->get_scale_root(); }
            ));

            menu->add(new HarmonyDisplay(
                "Output", 
                get_global_scale_identity(),
                nullptr, 
                nullptr
            ));
        #endif
    #endif
}

#endif // ENABLE_SCREEN
