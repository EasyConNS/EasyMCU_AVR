#include <LUFA/Drivers/Peripheral/Serial.h>

#include "Common.h"
#include "HID.h"
#include "LED.h"
// #include "Serial.h"
#include "Command.h"

const PROGMEM CommandEntryType CommandTable[] = {
  {
    .OP = 0x00,
    .CmdDataLength = 1,
    .ExeFunc = ExeResetButton
  },
  {
    .OP = 0x01,
    .CmdDataLength = 2,
    .ExeFunc = ExePressButton
  },
  {
    .OP = 0x02,
    .CmdDataLength = 2,
    .ExeFunc = ExeReleaseButton
  },
  {
    .OP = 0x03,
    .CmdDataLength = 1,
    .ExeFunc = ExeSetHAT
  },
  {
    .OP = 0x04,
    .CmdDataLength = 2,
    .ExeFunc = ExeSetLeftStick
  },
  {
    .OP = 0x05,
    .CmdDataLength = 2,
    .ExeFunc = ExeSetRightStick
  }
};

#define OPCODE opbuf

uint8_t opbuf = 0;
uint8_t serialbuf[BUF_SIZE] = {0};
size_t buffer_length = 0;
CommandAction_t cmd_state = IDLE;

/* TODO:
jc action
script action
flash
*/
void ExeResetButton(const uint8_t* data) {
  if(data[0] == 0)
  {
    ResetReport();
  }
}
void ExePressButton(const uint8_t* data) {
  PressButtons(*(uint16_t *)data);
}
void ExeReleaseButton(const uint8_t* data) {
  ReleaseButtons(*(uint16_t *)data);
}
void ExeSetHAT(const uint8_t* data) {
  SetHATSwitch(data[0]);
}
void ExeSetLeftStick(const uint8_t* data) {
  SetLeftStick(data[0], data[1]);
}
void ExeSetRightStick(const uint8_t* data) {
  SetRightStick(data[0], data[1]);
}

void CommandTick(void) {
  int16_t byte =  Serial_ReceiveByte();
  if (byte < 0) return;
  // SerialRingAdd(&sri,(uint8_t)byte);

  CommandTask(byte);
}

static void OPTask(uint8_t op) {
  if ((op & 0xF0) == OP_ACT)
  {
    cmd_state = BUTTON;
    buffer_length = 0;
    return;
  }
  switch(op)
  {
    case OP_VER:
      SerialSend(MCU_VERSION);
    break;
    case OP_PNG:
      SerialSend(RLY_PNG);
    break;
    case OP_FLH:
      cmd_state = FLASH;
    break;
    default:
      cmd_state = IDLE;
      SerialSend(RLY_ERR);
    break;
  }
}

static void CallCommandFunc(const CommandEntryType* CommandEntry) {
  CmdActionFuncType ExeFunc = pgm_read_ptr(&CommandEntry->ExeFunc);
  ExeFunc(serialbuf);
}

static void ButtonTask(void) {
  uint8_t i;
  bool CommandFound = false;
  uint8_t bt = OPCODE & 0x0F;
  for (i = 0; i < ARRAY_COUNT(CommandTable); i++) {
    if (bt == pgm_read_byte(&CommandTable[i].OP)) {
      CommandFound = true;

      if(buffer_length == pgm_read_byte(&CommandTable[i].CmdDataLength)) {
        CallCommandFunc(&CommandTable[i]);
        cmd_state = IDLE;
        BlinkLED();
      }else {
        SerialSend(0xCC);
      }

      break;
    }
  }
  if (! CommandFound) {
    cmd_state = IDLE;
    SerialSend(RLY_ERR);
    return;
  }
}

void CommandTask(uint8_t byte)
{
  switch(cmd_state)
  {
    case IDLE:
      OPCODE = byte;
      OPTask(byte);
    break;
    case BUTTON:
      serialbuf[buffer_length++] = byte;
      ButtonTask();
    break;
    case FLASH:
      buffer_length = BUF_SIZE;
      memset(serialbuf, 0, BUF_SIZE);
    break;
  }

  if (buffer_length >= BUF_SIZE)
  {
    buffer_length = 0;
  }
}

void SerialSend(uint8_t b)
{
    Serial_SendByte(b);
    BlinkLED();
}