#pragma once

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/power.h>

#include <stdbool.h>

#include <LUFA/Common/Common.h>
#include <LUFA/Drivers/Peripheral/Serial.h>
#include <LUFA/Drivers/Board/LEDs.h>

#define BADUD_RATE 9600

inline void disable_rx_isr(void)
{
    UCSR1B &= ~_BV(RXCIE1);
}

inline void enable_rx_isr(void)
{
    UCSR1B |= _BV(RXCIE1);
}

void SystemInit(void);