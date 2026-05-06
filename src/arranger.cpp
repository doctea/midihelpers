// arranger.cpp
// Provides the global Arranger singleton.
#include "arranger.h"

#ifdef ENABLE_ARRANGER
Arranger *arranger = nullptr;
#endif

playlist_t* get_default_playlist() {
    static playlist_t default_playlist;
    return &default_playlist;
}