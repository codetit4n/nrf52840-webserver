# -------------------------------------------------
# Project
# -------------------------------------------------
PROJECT := webserver
BUILD   := build

# -------------------------------------------------
# Toolchain
# -------------------------------------------------
CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

# -------------------------------------------------
# Paths
# -------------------------------------------------
FREERTOS := freertos/FreeRTOS-Kernel
NRFX_MDK := third_party/nrfx/mdk
CMSIS    := third_party/cmsis/include
WIZNET   := third_party/Wiznet_W5500
PLATFORM := platform/nrf52840

# -------------------------------------------------
# CPU flags (nRF52840 = Cortex-M4F)
# -------------------------------------------------
CFLAGS  := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS  += -O2 -g3 -ffunction-sections -fdata-sections
CFLAGS  += -Wall -Wextra -Werror -Wmissing-field-initializers -Wpedantic

# -------------------------------------------------
# Startup overrides
# -------------------------------------------------
CFLAGS += -DNRF52840_XXAA
CFLAGS  += -D__HEAP_SIZE=0
CFLAGS  += -D__STACK_SIZE=2048

# -------------------------------------------------
# Include paths
# -------------------------------------------------
CFLAGS  += -Iinclude
CFLAGS  += -I$(FREERTOS)/include
CFLAGS  += -I$(FREERTOS)/portable/GCC/ARM_CM4F
CFLAGS  += -I$(NRFX_MDK)
CFLAGS  += -I$(CMSIS)
CFLAGS  += -I$(WIZNET)

# -------------------------------------------------
# Linker
# -------------------------------------------------
LDSCRIPT := $(PLATFORM)/linker.ld

LDFLAGS  := $(CFLAGS)
LDFLAGS  += -T$(LDSCRIPT)
LDFLAGS  += -Wl,--gc-sections
LDFLAGS  += -Wl,-Map=$(BUILD)/$(PROJECT).map

# Allow INCLUDE "nrf_common.ld" to be resolved
LDFLAGS  += -L$(PLATFORM)

# -------------------------------------------------
# Sources
# -------------------------------------------------
APP_SRCS := $(wildcard src/*.c) \
            $(wildcard src/*/*.c) \
            $(wildcard src/*/*/*.c)


FREERTOS_SRCS := \
	$(FREERTOS)/list.c \
	$(FREERTOS)/queue.c \
	$(FREERTOS)/tasks.c \
	$(FREERTOS)/portable/GCC/ARM_CM4F/port.c \
	$(FREERTOS)/portable/MemMang/heap_4.c

NRFX_SRCS := \
	$(NRFX_MDK)/system_nrf52840.c

STARTUP := \
	$(NRFX_MDK)/gcc_startup_nrf52840.S

SRCS := $(APP_SRCS) $(FREERTOS_SRCS) $(NRFX_SRCS)
OBJS := $(SRCS:%.c=$(BUILD)/%.o) $(STARTUP:%.S=$(BUILD)/%.o)

# -------------------------------------------------
# Build rules
# -------------------------------------------------
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(PROJECT).elf: $(OBJS)
	@mkdir -p $(BUILD)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/$(PROJECT).hex: $(BUILD)/$(PROJECT).elf
	$(OBJCOPY) -O ihex $< $@

# -------------------------------------------------
# Targets
# -------------------------------------------------
all: $(BUILD)/$(PROJECT).hex

flash: all
	nrfjprog --program $(BUILD)/$(PROJECT).hex --chiperase --verify --reset

clean:
	rm -rf $(BUILD)

