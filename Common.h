#pragma once

#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/Peripheral/Serial.h>

#include "ubda/binfos.h"
#include "EasyCon.h"
// Start script.
#define Device_Connected Script_AutoStart
#define Echo_Report Decrement_Report_Echo

#define Max(a, b) ((a > b) ? (a) : (b))
#define Min(a, b) ((a < b) ? (a) : (b))

#define BADUD_RATE 9600

#define LED_DURATION 50
#define TurnOnLED LEDs_TurnOnLEDs
#define TurnOffLED LEDs_TurnOffLEDs

// UART && LEDS Init
void CommonInit(void);
void BlinkLED(void);
void BlinkLEDTick(void);
void Serial_Send(const char DataByte);

static inline void StartRunningLED(void) { LEDs_TurnOnLEDs(LEDMASK_RX); }
static inline void StopRunningLED(void) { LEDs_TurnOffLEDs(LEDMASK_RX); }