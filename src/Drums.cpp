#include "Drums.h"

const char *gm_drum_names[(GM_NOTE_MAXIMUM-GM_NOTE_MINIMUM)+1] = {
    "BDa", // GM_NOTE_ACOUSTIC_BASS_DRUM 35
    "BDe", // GM_NOTE_ELECTRIC_BASS_DRUM 36
    "SS ", // GM_NOTE_SIDE_STICK 37
    "SNa", // GM_NOTE_ACOUSTIC_SNARE 38
    "Clp", // GM_NOTE_HAND_CLAP 39
    "SNe", // GM_NOTE_ELECTRIC_SNARE 40
    "LFT", // GM_NOTE_LOW_FLOOR_TOM 41
    "CHH", // GM_NOTE_CLOSED_HI_HAT 42
    "TmH", // GM_NOTE_HIGH_FLOOR_TOM 43
    "PHH", // GM_NOTE_PEDAL_HI_HAT 44
    "LoT", // GM_NOTE_LOW_TOM 45
    "OHH", // GM_NOTE_OPEN_HI_HAT 46
    "LMT", // GM_NOTE_LOW_MID_TOM 47
    "HMT", // GM_NOTE_HI_MID_TOM 48
    "Cr1", // GM_NOTE_CRASH_CYMBAL_1 49
    "HiT", // GM_NOTE_HIGH_TOM 50
    "RCy", // GM_NOTE_RIDE_CYMBAL_1 51
    "CCy", // GM_NOTE_CHINESE_CYMBAL 52
    "Rbl", // GM_NOTE_RIDE_BELL 53
    "Tmb", // GM_NOTE_TAMBOURINE 54
    "SCy", // GM_NOTE_SPLASH_CYMBAL 55
    "Cow", // GM_NOTE_COWBELL 56
    "Cr2", // GM_NOTE_CRASH_CYMBAL_2 57
    "Vib", // GM_NOTE_VIBRA_SLAP 58
    "RC2", // GM_NOTE_RIDE_CYMBAL_2 59
    "HBo", // GM_NOTE_HIGH_BONGO 60
    "LBo", // GM_NOTE_LOW_BONGO 61
    "MHC", // GM_NOTE_MUTE_HIGH_CONGA 62
    "OHC", // GM_NOTE_OPEN_HIGH_CONGA 63
    "LoC", // GM_NOTE_LOW_CONGA 64
    "HTi", // GM_NOTE_HIGH_TIMBALE 65
    "LTi", // GM_NOTE_LOW_TIMBALE 66
    "HAg", // GM_NOTE_HIGH_AGOGO 67
    "LAg", // GM_NOTE_LOW_AGOGO 68
    "Cab", // GM_NOTE_CABASA 69
    "Mcs", // GM_NOTE_MARACAS 70
    "SWh", // GM_NOTE_SHORT_WHISTLE 71
    "LWh", // GM_NOTE_LONG_WHISTLE 72
    "SGu", // GM_NOTE_SHORT_GUIRO 73
    "LGu", // GM_NOTE_LONG_GUIRO 74
    "Clv", // GM_NOTE_CLAVES 75
    "HWB", // GM_NOTE_HIGH_WOODBLOCK 76
    "LWB", // GM_NOTE_LOW_WOODBLOCK 77
    "MCu", // GM_NOTE_MUTE_CUICA 78
    "OCu", // GM_NOTE_OPEN_CUICA 79
    "MTr", // GM_NOTE_MUTE_TRIANGLE 80
    "OTr"  // GM_NOTE_OPEN_TRIANGLE 81
};