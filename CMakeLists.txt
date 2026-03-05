cmake_minimum_required(VERSION 3.18)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER
    "C:/arm-toolchain/arm-none-eabi/bin/arm-none-eabi-gcc.exe")
set(CMAKE_OBJCOPY
    "C:/arm-toolchain/arm-none-eabi/bin/arm-none-eabi-objcopy.exe")

project(SatelliteFC C)

set(CPU_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft")

set(CMAKE_C_FLAGS
    "${CPU_FLAGS} -O2 -Wall -Wextra -Wno-unused-parameter -ffunction-sections -fdata-sections -fno-common")

set(CMAKE_EXE_LINKER_FLAGS
    "${CPU_FLAGS} -T ${CMAKE_SOURCE_DIR}/linker.ld --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections -Wl,-Map=satellite_fc.map")
#   ^^^ removed -lm and -mthumb-interwork from here

set(FREERTOS_SRC
    FreeRTOS/Source/tasks.c
    FreeRTOS/Source/queue.c
    FreeRTOS/Source/list.c
    FreeRTOS/Source/timers.c
    FreeRTOS/Source/event_groups.c
    FreeRTOS/Source/portable/GCC/ARM_CM3/port.c
    FreeRTOS/Source/portable/MemMang/heap_4.c)

set(APP_SRC
    src/startup.c
    src/main.c
    src/tasks/adcs_task.c
    src/tasks/telemetry_task.c
    src/tasks/health_monitor.c
    src/tasks/command_handler.c
    src/drivers/uart_driver.c
    src/drivers/watchdog.c)

add_executable(satellite_fc.elf ${APP_SRC} ${FREERTOS_SRC})

target_include_directories(satellite_fc.elf PRIVATE
    src/common
    src
    src/drivers
    src/tasks
    FreeRTOS/Source/include
    FreeRTOS/Source/portable/GCC/ARM_CM3)

# -lm goes here — after objects, so the linker can resolve sinf/cosf
target_link_libraries(satellite_fc.elf PRIVATE m)

add_custom_command(TARGET satellite_fc.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary satellite_fc.elf satellite_fc.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   satellite_fc.elf satellite_fc.hex
    COMMAND "C:/arm-toolchain/arm-none-eabi/bin/arm-none-eabi-size.exe" satellite_fc.elf
    COMMENT "Post-build: binary, hex, and size")