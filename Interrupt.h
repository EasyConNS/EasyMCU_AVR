#pragma once

#include <avr/interrupt.h>

#define read_bit(r,b) ((r) & (1u << (b)))
#define reset_bit(r,b) ((r) &= ~(1u << (b)))
#define set_bit(r,b) ((r) |= (1u << (b)))

static inline void PCIInit(void)
{
    // pinMode(PB0, INPUT_PULLUP);
    reset_bit(DDRB, PB5);
    /* Set PB5 as input */
    set_bit(PORTB, PB5);
    /* Activate PULL UP resistor */
    PCMSK0 |= _BV(PCINT5); /* Enable PCINT5 */
    // Activate PCINT7-0
    PCICR |= _BV(PCIE0);   /* Activate interrupt on enabled PCINT7-0 */
}
