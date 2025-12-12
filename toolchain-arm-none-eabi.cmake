# Tell CMake we are cross-compiling for a bare-metal system
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Use the ARM embedded GCC toolchain
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# We can't run test programs on the target, so don't try to run them
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
