#include "System.h"

inline void timer0_init(void)
{
    // set prescaler to 64 and start the timer
    TCCR0B |= (_BV(CS01)) | (_BV(CS00)); // 16,000,000(F_CPU) / 64  = 250,000 Hz
    TCNT0 = 6;
    TIMSK0 |= _BV(TOIE0); // Initialize timer0 interrupt
}

void SystemInit(void)
{
    // We need to disable watchdog if enabled by bootloader/fuses.
    MCUSR &= ~_BV(WDRF);
    wdt_disable();
    // We need to disable clock division before initializing the USB hardware.
    clock_prescale_set(clock_div_1);
    // We need disable global interrupts first for init timer.
    GlobalInterruptDisable();
    // 8-bit TCNT0 max 255.
    timer0_init();
    // We'll then enable global interrupts for our use.
    GlobalInterruptEnable();
}