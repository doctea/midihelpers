#pragma once
// Minimal scales.h stub for native testing — only the types used by arranger.h.

#include <cstdint>

// Needed by chord_identity_t
#define NOTE_OFF 127

namespace CHORD {
    typedef int Type;
    const int
        GLOBAL   = -1,
        TRIAD    =  0,
        SUS2     =  1,
        SUS4     =  2,
        SEVENTH  =  3,
        NINTH    =  4,
        OCTAVE_1 =  5,
        OCTAVE_2 =  6,
        OCTAVE_3 =  7,
        NONE     =  8;
}

class chord_identity_t {
public:
    CHORD::Type type      = CHORD::TRIAD;
    int8_t      degree    = -1;
    int8_t      inversion =  0;

    bool diff(chord_identity_t other) const {
        return type != other.type || degree != other.degree || inversion != other.inversion;
    }
};
