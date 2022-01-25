/*
Nintendo Switch Fightstick - Proof-of-

Based on the LUFA library's Low-Level Joystick 
    (C) Dean 
Based on the HORI's Pokken Tournament Pro Pad 
    (C) 

This project implements a modified version of HORI's Pokken Tournament Pro 
USB descriptors to allow for the creation of custom controllers for 
Nintendo Switch. This also works to a limited degree on the PS3.

Since System Update v3.0.0, the Nintendo Switch recognizes the 
Tournament Pro Pad as a Pro Controller. Physical design limitations 
the Pokken Controller from functioning at the same level as the 
Controller. However, by de...fault most of the descriptors are there, with 
exception of Home and Capture. Descriptor modification allows us to 
these buttons for our use.
*/

#include "Joystick.h"

int main(void)
{
    SystemInit();
    CommonInit();
    PCIInit();
    // Initialize script.
    ScriptInit();
    // The USB stack should be initialized last.
    HIDInit();
    // Once that's done, we'll enter an infinite loop.
    while (1)
    {
        // codes here...
        // Process local script instructions.
        ScriptTask();
        // ApplicationTask();
        HIDTask();
    }
}

ISR(TIMER0_OVF_vect) // timer0 overflow interrupt ~1ms
{
    TCNT0 += 6; // add 6 to the register (our work around)

    HIDTick();
    // script ms
    Increment_Timer();
    ScriptTick();

    BlinkLEDTick();
}

ISR(USART1_RX_vect)
{
    Serial_Task(Serial_ReceiveByte());
}

ISR(PCINT0_vect)
{
    /* This is where you get when an interrupt is happening */
    if(!read_bit(PINB, PB5))
    {
        if(isScriptRunning())
        {
            Script_Stop();
        }
        else
        {
            Script_Start();
        }
    }
}