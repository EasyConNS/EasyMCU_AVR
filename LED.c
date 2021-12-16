#include "LED.h"

volatile uint8_t led_ms = 0; // transmission LED countdown

void BlinkLED(void)
{
    led_ms = LED_DURATION;
    LEDs_TurnOnLEDs(LEDMASK_TX);
}
void BlinkLEDTick(void)
{
    // decrement LED counter
    if (led_ms != 0)
    {
        led_ms--;
        if (led_ms == 0)
            LEDs_TurnOffLEDs(LEDMASK_TX);
    }
}
