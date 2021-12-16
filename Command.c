#include "Command.h"

uint8_t opbuf = 0;
uint8_t serialbuf[BUF_SIZE] = {0};
size_t buffer_length = 0;
CommandAction_t cmd_state = IDLE;
/* TODO:
version
ping/pong
jc action
script action
flash
*/
void sopcode(uint8_t sop);
void sopbtn();

void CommandTask(void)
{
  int16_t byte =  Serial_ReceiveByte();
  if (byte == -1) return;

  switch(cmd_state)
  {
    case IDLE:
      OPCODE = byte;
      sopcode(byte);
    break;
    case BUTTON:
      serialbuf[buffer_length++] = byte;
      sopbtn();
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

void sopcode(uint8_t op)
{
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

// void rand_init(uint32_t *rand_state, uint32_t in1, uint32_t in2, uint32_t in3, uint32_t in4)
// and_init(rand_state, *(uint32_t *)&end[0x10], *(uint32_t *)&end[0x14], *(uint32_t *)&end[0x18], *(uint32_t *)&end[0x1C]);
void sopbtn()
{
  uint8_t bt = OPCODE & 0x0F;
  switch(bt)
  {
    case 0: // reset button
      if(buffer_length == 1 && serialbuf[0] == 0)
      {
        ResetReport();
        cmd_state = IDLE;
      }
    break;
    case 1: // press buttons
    case 2: // release buttons
      if(buffer_length == 2)
      {
        if(bt == 1)
        {
          PressButtons(*(uint16_t *)serialbuf);
        }
        else
        {
          ReleaseButtons(*(uint16_t *)serialbuf);
        }
        cmd_state = IDLE;
      }
    break;
    case 3:
      if(buffer_length == 1)
      {
        SetHATSwitch(serialbuf[0]);
        cmd_state = IDLE;
      }
    break;
    case 4: // set L Stick
    case 5: // set R Stick
      if(buffer_length == 2)
      {
        if(bt == 4)
        {
          SetLeftStick(serialbuf[0], serialbuf[1]);
        }
        else
        {
          SetRightStick(serialbuf[0], serialbuf[1]);
        }
        cmd_state = IDLE;
      }
    break;
    default:
      cmd_state = IDLE;
      return;
  }
}

void SerialSend(uint8_t b)
{
    Serial_SendByte(b);
    BlinkLED();
}