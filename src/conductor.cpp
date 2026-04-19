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
        if (!(options & COMBINE_TIME_SIG_WITH_TRANSPORT) && (options & COMBINE_HARMONY_WITH_TIME_SIG))
            // combine time signature and harmony settings into a single "Muse" page
            menu->add_page("Muse", C_WHITE, false);
        else if (!(options & COMBINE_TIME_SIG_WITH_TRANSPORT))
            // separate time signature page
            menu->add_page("Time signature", C_WHITE, false);

        #ifdef ENABLE_TIME_SIGNATURE
            menu->add(new TimeSignatureIndicator());
        #endif

        if (!(options & COMBINE_HARMONY_WITH_TIME_SIG)) {
            menu->add_page("Harmony", C_WHITE, false);
        }

        #ifdef ENABLE_SCALES
            LambdaScaleMenuItemBar *global_quantise_bar = new LambdaScaleMenuItemBar(
                "Global Scale",
                [](scale_index_t t)   { conductor->set_scale_type(t); },
                []() -> scale_index_t { return conductor->get_scale_type(); },
                [](int8_t r)          { conductor->set_scale_root(r); },
                []() -> int8_t        { return conductor->get_scale_root(); }
            );
            LambdaQuantiseModeControl *quant_mode = new LambdaQuantiseModeControl(
                "Quantise",
                [=](int8_t v) -> void {
                    this->set_global_quantise_mode((quantise_mode_t)constrain((int)v, (int)quantise_mode_t::QUANTISE_MODE_NONE, (int)quantise_mode_t::QUANTISE_MODE_CHORD));
                },
                [=]() -> int8_t {
                    return (int8_t)this->get_global_quantise_mode();
                }
            );
            global_quantise_bar->add(quant_mode);

            menu->add(global_quantise_bar);

            LambdaChordSubMenuItemBar *global_chord_bar = new LambdaChordSubMenuItemBar(
                "Global Chord", 
                [=](int8_t degree) -> void { conductor->set_chord_degree(degree); },
                [=]() -> int8_t { return conductor->get_chord_degree(); },
                [=](CHORD::Type chord_type) -> void { conductor->set_chord_type(chord_type); }, 
                [=]() -> CHORD::Type { return conductor->get_chord_type(); },
                [=](int8_t inversion) -> void { conductor->set_chord_inversion(inversion); },
                [=]() -> int8_t { return conductor->get_chord_inversion(); },
                false, true, true
            );
            // global_chord_bar->add(new LambdaToggleControl("Quantise",
            //     [=](bool v) -> void { conductor->set_global_quantise_chord_on(v); },
            //     [=]() -> bool { return conductor->is_global_quantise_chord_on(); }
            // ));
            menu->add(global_chord_bar);

            menu->add(new HarmonyDisplay(
                "Scale", 
                get_global_scale_identity(),
                nullptr,
                &conductor->global_quantise_mode,
                &conductor->global_chord_identity
            ));
        #endif
    #endif
}

#endif // ENABLE_SCREEN
