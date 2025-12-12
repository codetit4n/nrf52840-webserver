cmake_minimum_required(VERSION 3.20)

project(nrf52840_blinky C ASM)

# Hard-reset any inherited flags (to kill bad -march= etc)
set(CMAKE_C_FLAGS "" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS "" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "" CACHE STRING "" FORCE)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

set(SOURCES
    ${SRC_DIR}/startup.s
    ${SRC_DIR}/main.c
    ${SRC_DIR}/memutils.c
)

add_executable(blinky ${SOURCES})

# ---- Compile options (each flag is its own argument) ----
target_compile_options(blinky PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -Wall
    -Wextra
    -O0
    -g3
    -ffreestanding
)

# ---- Linker options ----
target_link_options(blinky PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -nostartfiles
    -nostdlib
    -Wl,-T${CMAKE_SOURCE_DIR}/linker.ld
)

set_target_properties(blinky PROPERTIES OUTPUT_NAME "blinky.elf")

# ===== FreeRTOS setup =====
set(FREERTOS_DIR "${CMAKE_SOURCE_DIR}/freertos/FreeRTOS-Kernel")

set(FREERTOS_KERNEL_SRC
    ${FREERTOS_DIR}/list.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/timers.c
    ${FREERTOS_DIR}/event_groups.c
    ${FREERTOS_DIR}/stream_buffer.c
)

set(FREERTOS_PORT_SRC
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
)

target_sources(blinky PRIVATE
    ${FREERTOS_KERNEL_SRC}
    ${FREERTOS_PORT_SRC}
)

target_include_directories(blinky PRIVATE
    ${FREERTOS_DIR}/include
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F
    ${CMAKE_SOURCE_DIR}/config
)

# Print ELF size after build
add_custom_command(TARGET blinky POST_BUILD
    COMMAND arm-none-eabi-size $<TARGET_FILE:blinky>
    COMMENT "Printing firmware size"
)
