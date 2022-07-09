#pragma once

#define BUF_SIZE 16

#define OP_VER 0x10 // 0001
#define OP_PNG 0x80 // 1000
#define OP_ACT 0x60 // 0110
#define OP_FLH 0xD0 // 1101

#define RLY_PNG 0x7F
#define RLY_ERR 0xB7
#define RLY_FLH 0xD1
#define RLY_FLH_END 0xD2

typedef void (*CmdActionFuncType) (const uint8_t* data);

typedef enum
{
    IDLE,
    BUTTON,
    FLASH,
    FLASH_START,
}CommandAction_t;

typedef struct {
    uint8_t OP;
    uint8_t CmdDataLength;
    CmdActionFuncType ExeFunc;
}CommandEntryType;

void CommandTick(void);
void CommandTask(uint8_t byte);
void SerialSend(uint8_t b);

// button
void ExeResetButton(const uint8_t* data);
void ExePressButton(const uint8_t* data);
void ExeReleaseButton(const uint8_t* data);
// hat
void ExeSetHAT(const uint8_t* data);
// L/R stick
void ExeSetLeftStick(const uint8_t* data);
void ExeSetRightStick(const uint8_t* data);
