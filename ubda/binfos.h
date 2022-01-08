#pragma once

#if (REAL_BOARD == Leonardo)
    #define DEFAULT_BOARD
#elif (REAL_BOARD == TEENSY2pp)
    #define LEDMASK_TX   LEDS_LED1
#elif (REAL_BOARD == Teensy2)
    #define LEDMASK_TX   LEDS_LED1
#elif (REAL_BOARD == Beetle)
    #define LEDMASK_TX   LEDS_LED3
    #define LEDMASK_RX   LEDS_LED3
#elif (REAL_BOARD == UNO)
    #define MEM_SIZE 412
    #define LEDMASK_TX   LEDS_LED2
#endif

#if !defined(MEM_SIZE)
    #define MEM_SIZE      924
#endif
#if !defined(LEDMASK_TX)
    #define LEDMASK_TX      LEDS_LED2
#endif
#if !defined(LEDMASK_RX)
    #define LEDMASK_RX      LEDS_LED1
#endif