#pragma once

#if (REAL_BOARD == Leonardo)
    #include "pins/pins_leonardo.h"
#elif (REAL_BOARD == TEENSY2pp)
    #include "pins/pins_teensy.h"
#endif