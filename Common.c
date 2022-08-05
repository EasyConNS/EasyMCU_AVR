#include "Common.h"

volatile uint8_t led_ms = 0; // transmission LED countdown

inline void disable_rx_isr(void)
{
    UCSR1B &= ~_BV(RXCIE1);
}

inline void enable_rx_isr(void)
{
    UCSR1B |= _BV(RXCIE1);
}

void CommonInit(void)
{
    // Initialize serial port.
    Serial_Init(BADUD_RATE, false);
    enable_rx_isr();
    // Initialize LEDs.
    LEDs_Init();
}

void BlinkLEDTick(void)
{
    // decrement LED counter
    if (led_ms != 0)
    {
        led_ms--;
        if (led_ms == 0)
            LEDs_TurnOffLEDs(LED_TX);
    }
}

void Serial_Send(const char DataByte)
{
    Serial_SendByte(DataByte);
    BlinkLED();
}