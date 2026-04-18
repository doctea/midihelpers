// menu_conductor.h
// Ready-made menu items for editing Conductor settings.
// Requires ENABLE_SCREEN.
//
// The main entry point is Conductor::create_menu_items(menu), which adds:
//   - LoopMarkerPanel (pinned)
//   - BPMPositionIndicator
//   - ClockSourceSelectorControl
//   - ConductorTransportBar  (Start / Stop / Continue / Restart)
//   - TimeSignatureIndicator  (if ENABLE_TIME_SIGNATURE)
//   - LambdaScaleMenuItemBar  (if ENABLE_SCALES)
//
// Include this file before calling conductor->make_menu_items().
#pragma once

#ifdef ENABLE_SCREEN

#include "../conductor.h"
#include "menu_bpm.h"           // BPMPositionIndicator, LoopMarkerPanel
#include "menu_clock_source.h"  // ClockSourceSelectorControl
#include "submenuitem_bar.h"    // SubMenuItemBar
#include "menuitems_action.h"   // ActionItem, ActionFeedbackItem

#ifdef ENABLE_SCALES
  #include "menuitems_scale.h"  // LambdaScaleMenuItemBar
#endif

#ifdef ENABLE_TIME_SIGNATURE
    #include <functional-vlpp.h>
    #include "menuitems_lambda_selector.h"
    // TimeSignatureIndicator — routes through conductor so changes trigger a single
    // notification and keep both this->time_signature and the global cache in sync.
    class TimeSignatureIndicator : public SubMenuItemBar {
    public:
        TimeSignatureIndicator() : SubMenuItemBar("Time signature", true, true) {
            LambdaSelectorControl<uint8_t> *timesig_numerator_control = new LambdaSelectorControl<uint8_t>(
                "Numerator",
                [](uint8_t v) { conductor->set_numerator(v); },
                []() -> uint8_t { return conductor->get_numerator(); },
                nullptr, true, false
            );
            for (int i = 1; i < 21; i++)
                timesig_numerator_control->add_available_value(i, (new String(i))->c_str());
            this->add(timesig_numerator_control);

            LambdaSelectorControl<uint8_t> *timesig_denominator_control = new LambdaSelectorControl<uint8_t>(
                "Denominator",
                [](uint8_t v) { conductor->set_denominator(v); },
                []() -> uint8_t { return conductor->get_denominator(); },
                nullptr, true, false
            );
            for (int i = 2; i < 16; i += 2)
                timesig_denominator_control->add_available_value(i, (new String(i))->c_str());
            this->add(timesig_denominator_control);
        }
    };
#endif // ENABLE_TIME_SIGNATURE

// Transport bar: Start / Stop / Continue / Restart grouped in a SubMenuItemBar.
class ConductorTransportBar : public SubMenuItemBar {
public:
    ConductorTransportBar() : SubMenuItemBar("Transport", false) {
        this->add(new ActionItem("Start",    clock_start));
        this->add(new ActionItem("Stop",     clock_stop));
        this->add(new ActionItem("Continue", clock_continue));
        this->add(new ActionFeedbackItem(
            "Restart",
            (ActionFeedbackItem::setter_def_2)set_restart_on_next_bar_on,
            is_restart_on_next_bar,
            "Restarting..", "Restart"
        ));
    }
};

#endif // ENABLE_SCREEN
