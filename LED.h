#pragma once

#include <LUFA/Drivers/Board/LEDs.h>

#include "ubda/binfos.h"

#define LED_DURATION 50

void BlinkLED(void);
void BlinkLEDTick(void);

static inline void StartRunningLED(void) { LEDs_TurnOnLEDs(LEDMASK_RX); }
static inline void StopRunningLED(void) { LEDs_TurnOffLEDs(LEDMASK_RX); }