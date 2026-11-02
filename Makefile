# GCC build for the STM32 firmware (Keil Project.uvprojx is still supported).
#   make            -> build/manbo.elf / .hex / .bin
#   make V2=1       -> new carrier PCB pin map (MANBO_BOARD_V2)
#   make LITE=1     -> low-cost voice + motion build (MANBO_LITE, hardware/lite)
#   make flash      -> st-flash write build/manbo.bin 0x08000000
#   make test       -> host unit tests
# Requires arm-none-eabi-gcc (Ubuntu: apt install gcc-arm-none-eabi).

PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size
BUILD   := build
TARGET  := $(BUILD)/manbo

# Same C files as Project.uvprojx; the Keil startup .s is ARMCC-only, so the
# GCC build uses gcc/startup_stm32f103xb.c instead.
SRCS := $(shell grep -oP '(?<=<FilePath>)[^<]+\.c(?=</FilePath>)' Project.uvprojx | tr '\\' '/' | sed 's|^\./||')
SRCS += gcc/startup_stm32f103xb.c gcc/syscalls.c

DEFS := -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD
ifeq ($(V2),1)
DEFS += -DMANBO_BOARD_V2
endif
ifeq ($(LITE),1)
DEFS += -DMANBO_LITE
endif

INCS := -IStart -ILibrary -IUser -ISystem -IHardware
ARCH := -mcpu=cortex-m3 -mthumb
CFLAGS := $(ARCH) $(DEFS) $(INCS) -std=gnu99 -Os -g3 \
          -ffunction-sections -fdata-sections -fno-common \
          -Wall -Wextra -Wno-unused-parameter -Wno-missing-braces \
          -Werror=implicit-function-declaration
LDFLAGS := $(ARCH) -Tgcc/STM32F103C8Tx.ld -Wl,--gc-sections \
           -Wl,-Map=$(TARGET).map --specs=nano.specs --specs=nosys.specs
LDLIBS := -lm

OBJS := $(patsubst %.c,$(BUILD)/%.o,$(SRCS))

all: $(TARGET).hex $(TARGET).bin
	@$(SIZE) $(TARGET).elf

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(TARGET).bin
	st-flash write $< 0x08000000

test:
	bash tests/host/run_tests.sh

clean:
	rm -rf $(BUILD)

.PHONY: all flash test clean
