#pragma once

#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/Peripheral/Serial.h>

#include "binfos.h"
#include "EasyCon_API.h"

// Start script.
#define Device_Connected EasyCon_script_auto_start
#define Echo_Report EasyCon_decrease_report_echo

#define Max(a, b) ((a > b) ? (a) : (b))
#define Min(a, b) ((a < b) ? (a) : (b))
//#define _BV(n) (1<<(n))

#define BADUD_RATE 115200

#define LED_DURATION 50
#define TurnOnLED LEDs_TurnOnLEDs
#define TurnOffLED LEDs_TurnOffLEDs

extern volatile uint8_t led_ms;

// UART && LEDS Init
void CommonInit(void);
void BlinkLED(void);
void BlinkLEDTick(void);
void Serial_Send(const char DataByte);