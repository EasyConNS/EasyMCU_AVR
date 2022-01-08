#pragma once

#if (REAL_BOARD == BOARD_UNKNOWN)
    static inline void pinMode(void);
#elif (REAL_BOARD == Leonardo)
    #include "pins/pins_leonardo.h"
#elif (REAL_BOARD == TEENSY2pp)
    #include "pins/pins_leonardo.h"
#endif