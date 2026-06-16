#include "mymenu/menuitems_scale.h"

OptionList<LambdaSelectorControl<scale_index_t>::option> *LambdaScaleMenuItemBar::scale_selector_options_with_global = nullptr;
OptionList<LambdaSelectorControl<scale_index_t>::option> *LambdaScaleMenuItemBar::scale_selector_options_no_global = nullptr;
OptionList<LambdaSelectorControl<int8_t>::option> *LambdaScaleMenuItemBar::scale_root_options_with_global = nullptr;
OptionList<LambdaSelectorControl<int8_t>::option> *LambdaScaleMenuItemBar::scale_root_options_no_global = nullptr;

labelled_value_list_t<int8_t> quantise_mode_list_with_none =
    {(labelled_value_t<int8_t>[]) {
        { QUANTISE_MODE_NONE,  "None" },
        { QUANTISE_MODE_SCALE, "Scale" },
        { QUANTISE_MODE_CHORD, "Chord" }
    }};

labelled_value_list_t<int8_t> quantise_mode_list_no_none =
    {(labelled_value_t<int8_t>[]) {
        { QUANTISE_MODE_SCALE, "Scale" },
        { QUANTISE_MODE_CHORD, "Chord" }
    }};