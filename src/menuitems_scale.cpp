#include "mymenu/menuitems_scale.h"

OptionList<LambdaSelectorControl<scale_index_t>::option> *LambdaScaleMenuItemBar::scale_selector_options_with_global = nullptr;
OptionList<LambdaSelectorControl<scale_index_t>::option> *LambdaScaleMenuItemBar::scale_selector_options_no_global = nullptr;
OptionList<LambdaSelectorControl<int8_t>::option> *LambdaScaleMenuItemBar::scale_root_options_with_global = nullptr;
OptionList<LambdaSelectorControl<int8_t>::option> *LambdaScaleMenuItemBar::scale_root_options_no_global = nullptr;
