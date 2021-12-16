.PHONY:mkdir
.PHONY:all

$(call ERROR_IF_EMPTY, REAL_BOARD)

# BOARD        := LEONARDO

ifeq ($(REAL_BOARD),UNO)
  MCU     = atmega16u2
endif
ifeq  ($(REAL_BOARD),TEENSY2pp)
  MCU     = at90usb1286
endif

MCU          ?= atmega32u4
ARCH         := AVR8
F_CPU        := 16000000
F_USB        := $(F_CPU)
OPTIMIZATION := s
TARGET       := ./$(REAL_BOARD)/$(REAL_BOARD)
SRC          += main.c LUFADescriptors.c $(LUFA_SRC_SERIAL)
SRC	         += HID.c System.c Command.c LED.c Script.c
SRC          += $(LUFA_SRC_USB)
LUFA_PATH     = ./lufa/LUFA
CC_FLAGS      = -DUSE_LUFA_CONFIG_HEADER -IConfig/
LD_FLAGS      =

# Default target
default: all

mkdir:
	@mkdir -p ./$(REAL_BOARD)

all:mkdir

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_doxygen.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk
