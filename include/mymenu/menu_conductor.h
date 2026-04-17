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
#include "menu_bpm.h"           // BPMPositionIndicator, TimeSignatureIndicator, LoopMarkerPanel
#include "menu_clock_source.h"  // ClockSourceSelectorControl
#include "submenuitem_bar.h"    // SubMenuItemBar
#include "menuitems_action.h"   // ActionItem, ActionFeedbackItem

#ifdef ENABLE_SCALES
  #include "menuitems_scale.h"  // LambdaScaleMenuItemBar
#endif

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
