#pragma once

#ifdef ENABLE_SCREEN
#ifdef ENABLE_ARRANGER

#include "menuitems_lambda_selector.h"
#include "arranger.h"

// SongSectionSelectorItem: a LambdaSelectorControl<int8_t> that presents
// song sections by name (via get_section_name()).
//
// A single static available_values list is built lazily the first time an
// instance is constructed, then shared with every subsequent instance via
// set_available_values() — saving NUM_SONG_SECTIONS heap allocations per
// extra instance.
//
// Usage:
//   menu->add(new SongSectionSelectorItem(
//       "Section",
//       [=](int8_t v) { my_section = v; },
//       [=]() -> int8_t { return my_section; }
//   ));
class SongSectionSelectorItem : public LambdaSelectorControl<int8_t> {
public:
    static LinkedList<LambdaSelectorControl<int8_t>::option> *song_section_options;

    SongSectionSelectorItem(
        const char *label,
        vl::Func<void(int8_t)>  setter_func,
        vl::Func<int8_t(void)>  getter_func,
        bool go_back_on_select = true,
        bool direct            = true
    ) : LambdaSelectorControl<int8_t>(label, setter_func, getter_func, nullptr, go_back_on_select, direct)
    {
        if (SongSectionSelectorItem::song_section_options == nullptr) {
            for (int i = 0; i < NUM_SONG_SECTIONS; i++) {
                this->add_available_value((int8_t)i, get_section_name(i));
            }
            SongSectionSelectorItem::song_section_options = this->get_available_values();
        } else {
            this->set_available_values(SongSectionSelectorItem::song_section_options);
        }
    }
};

#endif  // ENABLE_ARRANGER
#endif  // ENABLE_SCREEN
