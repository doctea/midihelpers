#pragma once

#include "chord_player.h"
#include "parameters/Parameter.h"

#ifdef ENABLE_SCALES

// Shared modulatable parameter type for CHORD::Type values.
template<class TargetClass = ChordPlayer, class DataType = CHORD::Type>
class ChordTypeParameter : public DataParameter<TargetClass, DataType> {
public:
    ChordTypeParameter(
        const char *label,
        TargetClass *target,
        void(TargetClass::*setter_func)(DataType),
        DataType(TargetClass::*getter_func)()
    )
        : DataParameter<TargetClass, DataType>(
              label,
              target,
              setter_func,
              getter_func,
              0,
              NUMBER_CHORDS - 1
          ) {}

#ifdef ENABLE_SCREEN
    virtual const char *parseFormattedDataType(DataType value) {
        static char fmt[MENU_C_MAX] = "              ";
        snprintf(fmt, MENU_C_MAX, chords[value].label);
        return fmt;
    }
#endif
};

inline void add_chord_modulation_parameters(
    LinkedList<FloatParameter *> *parameters,
    ChordPlayer *chord_player,
    const char *chord_type_label = "Chord Type",
    const char *inversion_label = "Inversion"
) {
    if (parameters == nullptr || chord_player == nullptr) {
        return;
    }

    parameters->add(
        new ChordTypeParameter<ChordPlayer>(
            chord_type_label,
            chord_player,
            &ChordPlayer::set_selected_chord,
            &ChordPlayer::get_selected_chord
        )
    );

    parameters->add(
        new LDataParameter<int8_t>(
            inversion_label,
            [=](int8_t v) -> void { chord_player->set_inversion(v); },
            [=](void) -> int8_t { return chord_player->get_inversion(); },
            0,
            MAXIMUM_INVERSIONS
        )
    );
}

#endif
