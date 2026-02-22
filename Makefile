# -------------------------------------------------
# Project
# -------------------------------------------------
PROJECT := webserver
BUILD   := build

.DEFAULT_GOAL := all

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
WIZNET   := third_party/Wiznet-ioLibrary/Ethernet
PLATFORM := platform/nrf52840

# -------------------------------------------------
# CPU flags (nRF52840 = Cortex-M4F)
# -------------------------------------------------
CFLAGS_COMMON  := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS_COMMON  += -O2 -g3 -ffunction-sections -fdata-sections

# -------------------------------------------------
# Startup overrides
# -------------------------------------------------
CFLAGS_COMMON += -DNRF52840_XXAA
CFLAGS_COMMON += -D__HEAP_SIZE=0
CFLAGS_COMMON += -D__STACK_SIZE=2048

# -------------------------------------------------
# Include paths
# -------------------------------------------------
INCLUDES  := -Iinclude
INCLUDES  += -I$(FREERTOS)/include
INCLUDES  += -I$(FREERTOS)/portable/GCC/ARM_CM4F
INCLUDES  += -I$(NRFX_MDK)
INCLUDES  += -I$(CMSIS)
INCLUDES  += -I$(WIZNET)
INCLUDES  += -I$(WIZNET)/W5500

# -------------------------------------------------
# Warnings policy
#   - Strict for my code (src/*)
#   - Relaxed for third-party + RTOS
# -------------------------------------------------
APP_WARN    := -Wall -Wextra -Wmissing-field-initializers -Wpedantic
VENDOR_WARN := -w

# Default flags (not used by build rules directly, but kept for convenience)
CFLAGS := $(CFLAGS_COMMON) $(INCLUDES) $(VENDOR_WARN)

WIZNET_CFLAGS := -D_WIZCHIP_=W5500 -D_WIZCHIP_IO_MODE_=_WIZCHIP_IO_MODE_SPI_

# -------------------------------------------------
# Linker
# -------------------------------------------------
LDSCRIPT := $(PLATFORM)/linker.ld

LDFLAGS  := $(CFLAGS_COMMON)
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

WIZNET_SRCS := \
	$(WIZNET)/wizchip_conf.c \
	$(WIZNET)/socket.c \
	$(WIZNET)/W5500/w5500.c

SRCS := $(APP_SRCS) $(FREERTOS_SRCS) $(NRFX_SRCS) $(WIZNET_SRCS)
OBJS := $(SRCS:%.c=$(BUILD)/%.o) $(STARTUP:%.S=$(BUILD)/%.o)

# -------------------------------------------------
# Build rules
# -------------------------------------------------
# strict warnings only for files under src/
define PICK_WARN
$(if $(filter src/%,$(1)),$(APP_WARN),$(VENDOR_WARN))
endef

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_COMMON) $(INCLUDES) $(WIZNET_CFLAGS) $(call PICK_WARN,$<) -c $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_COMMON) $(INCLUDES) $(WIZNET_CFLAGS) $(VENDOR_WARN) -c $< -o $@

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
