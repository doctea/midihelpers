#include "pitch_trigger_source.h"

#if defined(ENABLE_SCREEN)
    #if __cplusplus < 201703L
        LinkedList<LambdaSelectorControl<int32_t>::option> *PitchTriggerSource::length_ticks_control_options;
        LinkedList<LambdaSelectorControl<int32_t>::option> *PitchTriggerSource::trigger_ticks_control_options;
        LinkedList<LambdaSelectorControl<int32_t>::option> *PitchTriggerSource::trigger_delay_ticks_control_options;
    #endif
#endif
