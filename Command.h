#pragma once

#include <LUFA/Drivers/Peripheral/Serial.h>

#include "Common.h"
#include "HID.h"
#include "LED.h"

#define BUF_SIZE 16

#define OP_VER 0x10 // 0001
#define OP_PNG 0x80 // 1000
#define OP_ACT 0x60 // 0110
#define OP_FLH 0xD0 // 1101

#define RLY_PNG 0x7F
#define RLY_ERR 0xB7

typedef enum
{
    IDLE,
    BUTTON,
    FLASH,
}CommandAction_t;

#define OPCODE opbuf

void SerialSend(uint8_t b);
void CommandTask(void);
