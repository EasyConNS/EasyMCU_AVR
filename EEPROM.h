#pragma once

#include <avr/eeprom.h>

#include <stdint.h>
#include <stdbool.h>

#include "ubda/binfos.h"

#define EMEM_SIZE MEM_SIZE

// (uint8_t)
#define EEP_Write_Byte eeprom_write_byte
#define EEP_Read_Byte eeprom_read_byte
// (uint16_t)
#define EEP_Write_Word eeprom_write_word
#define EEP_Read_Word eeprom_read_word