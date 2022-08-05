ifeq ($(REAL_BOARD),UNO)
  MCU		  = atmega16u2
  #REALBOARD1 = UNO 
endif
ifeq  ($(REAL_BOARD),Teensy2pp)
  MCU		  = at90usb1286
  #REALBOARD1 = Teensy2pp
endif

#REALBOARD1  ?= Leonardo
MCU          ?= atmega32u4
ARCH         = AVR8
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = ./$(REAL_BOARD)/$(REAL_BOARD)
SRC          += Joystick.c LUFADescriptors.c
SRC		 	 += HID.c System.c Common.c
SRC		 	 += EasyCon_API.c
SRC		 	 += EasyCon.c
SRC			 += $(LUFA_SRC_USB)
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -IConfig -D$(REAL_BOARD)
LD_FLAGS     =

# Default target
default: all

mkdir:
	@mkdir -p ./$(REAL_BOARD)

all:mkdir

# Include LUFA build script makefiles
LUFA_PATH    = ./lufa/LUFA
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk

# Target for LED/buzzer to alert when print is done
with-alert: all
with-alert: CC_FLAGS += -DALERT_WHEN_DONE