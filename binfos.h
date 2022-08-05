#ifndef BINFOS_H
#define BINFOS_H

#ifdef UNO
     #define MEM_SIZE 398
     #define LED_TX   LEDS_LED2
#endif

#ifdef Beetle
    #define LED_TX   LEDS_LED3
    #define LED_RX   LEDS_LED3
#endif

#ifdef Leonardo
    #define DEFAULT_BOARD
#endif

#ifdef Teensy2
     #define LED_TX   LEDS_LED1
#endif

#ifdef Teensy2pp
     #define LED_TX   LEDS_LED1
#endif

#if !defined(MEM_SIZE)
    #define MEM_SIZE      924
#endif

#if !defined(LED_TX)
    #define LED_TX      LEDS_LED2
#endif

#if !defined(LED_RX)
    #define LED_RX      LEDS_LED1
#endif

#endif // BINFOS_H


