#include "pitch_trigger_source.h"

#if defined(ENABLE_SCREEN)
    #if __cplusplus < 201703L
        OptionList<LambdaSelectorControl<int32_t>::option> *PitchTriggerSource::length_ticks_control_options;
        OptionList<LambdaSelectorControl<int32_t>::option> *PitchTriggerSource::trigger_ticks_control_options;
        OptionList<LambdaSelectorControl<int32_t>::option> *PitchTriggerSource::trigger_delay_ticks_control_options;
    #endif
#endif
