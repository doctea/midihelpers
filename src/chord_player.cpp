#include "chord_player.h"

#if defined(ENABLE_SCREEN) && defined(ENABLE_SCALES)
    // linker may need these static definitions; however C++17 allows `inline` in the class def instead
    #if __cplusplus < 201703L
        LinkedList<LambdaSelectorControl<int32_t>::option> *ChordPlayer::length_ticks_control_options;
        LinkedList<LambdaSelectorControl<int32_t>::option> *ChordPlayer::trigger_ticks_control_options;
        LinkedList<LambdaSelectorControl<int32_t>::option> *ChordPlayer::trigger_delay_ticks_control_options;
    #endif
#endif