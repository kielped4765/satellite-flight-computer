cmake_minimum_required(VERSION 3.18)

# -------------------------------------------------------------------
# Cross-compilation setup (MUST be before project())
# -------------------------------------------------------------------
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER
    "C:/toolchains/arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-eabi/bin/arm-none-eabi-gcc.exe")

set(CMAKE_OBJCOPY
    "C:/toolchains/arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-eabi/bin/arm-none-eabi-objcopy.exe")

set(CMAKE_EXE_LINKER_FLAGS
 "${CPU_FLAGS} -T ${CMAKE_SOURCE_DIR}/linker.ld -nostdlib --specs=nosys.specs -Wl,--gc-sections -Wl,-Map=satellite_fc.map -lm")

 project(SatelliteFC C)

# -------------------------------------------------------------------
# Source files
# -------------------------------------------------------------------

set(APP_SRC
    src/startup.c
    src/main.c
    src/tasks/adcs_task.c
    src/tasks/telemetry_task.c
    src/tasks/health_monitor.c
    src/tasks/command_handler.c
    src/drivers/uart_driver.c
    src/drivers/watchdog.c
)

set(FREERTOS_SRC
    FreeRTOS/Source/tasks.c
    FreeRTOS/Source/queue.c
    FreeRTOS/Source/list.c
    FreeRTOS/Source/event_groups.c
    FreeRTOS/Source/timers.c
    FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
    FreeRTOS/Source/portable/MemMang/heap_4.c
)

# -------------------------------------------------------------------
# Target
# -------------------------------------------------------------------

add_executable(satellite_fc.elf
    ${APP_SRC}
    ${FREERTOS_SRC}
    
)

target_link_libraries(satellite_fc.elf
    c
    m
    gcc
)

target_sources(satellite_fc.elf PRIVATE
    src/syscalls.c
)

# -------------------------------------------------------------------
# Include directories
# -------------------------------------------------------------------

target_include_directories(satellite_fc.elf PRIVATE
    src
    src/common
    src/drivers
    src/tasks

    FreeRTOS/Source/include
    FreeRTOS/Source/portable/GCC/ARM_CM4F
)

# -------------------------------------------------------------------
# CPU + compiler flags (TARGET-BASED — CORRECT WAY)
# -------------------------------------------------------------------

set(CPU_FLAGS
    -mcpu=cortex-m3
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=soft
)

target_compile_options(satellite_fc.elf PRIVATE
    ${CPU_FLAGS}
    -O2
    -Wall
    -Wextra
    -Wno-unused-parameter
    -ffunction-sections
    -fdata-sections
    -fno-common
)

# -------------------------------------------------------------------
# Linker flags
# -------------------------------------------------------------------

target_link_options(satellite_fc.elf PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard

    -Wl,--gc-sections
    -Wl,-Map=satellite_fc.map

    --specs=nosys.specs
    --specs=nano.specs
)


# -------------------------------------------------------------------
# Post-build artifacts
# -------------------------------------------------------------------

add_custom_command(TARGET satellite_fc.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary satellite_fc.elf satellite_fc.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   satellite_fc.elf satellite_fc.hex
    COMMAND "C:/toolchains/arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-eabi/bin/arm-none-eabi-size.exe" satellite_fc.elf
    COMMENT "Generating .bin, .hex, and size info"
)
    