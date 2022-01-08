#pragma once

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/power.h>

#include <stdbool.h>

#include <LUFA/Common/Common.h>

void SystemInit(void);
