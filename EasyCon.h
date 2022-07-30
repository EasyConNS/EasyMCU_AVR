#ifndef _EASY_CON_H_
#define _EASY_CON_H_

#include "EasyCon_API.h"

/**********************************************************************/
// It's core of EasyCon, there is no need to change in most situations
/**********************************************************************/

/**********************************************************************/
// EasyCon version, need check Whether PC could communicate
/**********************************************************************/
#define VERSION 0x46

// if lost key too many, could increase it,recommand default value
#define ECHO_TIMES 3

// constants
#define SERIAL_BUFFER_SIZE 20
#define DIRECTION_OFFSET 20
#define VARSPACE_OFFSET 90
#define KEYCODE_OFFSET 90
#define KEYCODE_MAX 33
#define REGISTER_OFFSET 130
#define STACK_OFFSET 180
#define CALLSTACK_OFFSET 230
#define FORSTACK_OFFSET 290
#define INS_OFFSET 410
#define SEED_OFFSET MEM_SIZE + 0
#define LED_SETTING MEM_SIZE + 2

// serial protocal control bytes and replies
#define CMD_READY 0xA5
#define CMD_DEBUG 0x80
#define CMD_HELLO 0x81
#define CMD_FLASH 0x82
#define CMD_SCRIPTSTART 0x83
#define CMD_SCRIPTSTOP 0x84
#define CMD_VERSION 0x85
#define CMD_LED 0x86
#define REPLY_ERROR 0x00
#define REPLY_ACK 0xFF
#define REPLY_BUSY 0xFE
#define REPLY_HELLO 0x80
#define REPLY_FLASHSTART 0x81
#define REPLY_FLASHEND 0x82
#define REPLY_SCRIPTACK 0x83

// indexed variables and inline functions
#define SERIAL_BUFFER(i) mem[(i)]
#define KEY(keycode) mem[KEYCODE_OFFSET + (keycode)]
#define REG(i) *(int16_t *)(mem + REGISTER_OFFSET + ((i) << 1))
#define STACK(i) *(int16_t *)(mem + STACK_OFFSET + ((i) << 1))
#define CALLSTACK(i) *(uint16_t *)(mem + CALLSTACK_OFFSET + ((i) << 1))
#define DX(i) mem[DIRECTION_OFFSET + ((i) << 1)]
#define DY(i) mem[DIRECTION_OFFSET + ((i) << 1) + 1]
#define FOR_I(i) *(int32_t *)(mem + FORSTACK_OFFSET + (i)*12)
#define FOR_C(i) *(int32_t *)(mem + FORSTACK_OFFSET + (i)*12 + 4)
#define FOR_ADDR(i) *(uint16_t *)(mem + FORSTACK_OFFSET + (i)*12 + 8)
#define FOR_NEXT(i) *(uint16_t *)(mem + FORSTACK_OFFSET + (i)*12 + 10)
#define SETWAIT(time) wait_ms = (time)
#define RESETAFTER(keycode, n) KEY(keycode) = n
#define JUMP(addr) script_addr = (uint8_t *)(addr)
#define JUMPNEAR(addr) script_addr = (uint8_t *)((uint16_t)script_addr + (addr))
#define E(val) _e_set = 1, _e_val = (val)
#define E_SET ((_e_set_t = _e_set), (_e_set = 0), _e_set_t)

// single-byte variables
#define _ins3 mem[INS_OFFSET]
#define _ins2 mem[INS_OFFSET + 1]
#define _ins1 mem[INS_OFFSET + 2]
#define _ins0 mem[INS_OFFSET + 3]
#define _ins *(uint16_t *)(mem + INS_OFFSET + 2)
#define _insEx *(uint32_t *)(mem + INS_OFFSET)
#define _code mem[INS_OFFSET + 4]
#define _keycode mem[INS_OFFSET + 5]
#define _lr mem[INS_OFFSET + 6]
#define _direction mem[INS_OFFSET + 7]
#define _addr *(uint16_t *)(mem + INS_OFFSET + 8)
#define _stackindex mem[INS_OFFSET + 10]
#define _callstackindex mem[INS_OFFSET + 11]
#define _forstackindex mem[INS_OFFSET + 12]
#define _report_echo mem[INS_OFFSET + 13]
#define _e_set mem[INS_OFFSET + 14]
#define _e_set_t mem[INS_OFFSET + 15]
#define _e_val *(uint16_t *)(mem + INS_OFFSET + 16) // external argument for next instruction (used for dynamic for-loop, wait etc.)
#define _script_running mem[INS_OFFSET + 18]
#define _ri0 mem[INS_OFFSET + 19]
#define _ri1 mem[INS_OFFSET + 20]
#define _v mem[INS_OFFSET + 21]
#define _flag mem[INS_OFFSET + 22]
#define _seed *(uint16_t *)(mem + INS_OFFSET + 23)

#endif