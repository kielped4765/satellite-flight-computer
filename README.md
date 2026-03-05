# RTOS Satellite Flight Computer

FreeRTOS firmware simulating a real satellite attitude determination and control system, running on a QEMU-emulated ARM Cortex-M3. Includes binary UART telemetry and a live Python ground station GUI.

> Built and debugged on Windows 11 with the ARM GCC toolchain, CMake/Ninja, and QEMU.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    FreeRTOS Scheduler                       │
│              ARM Cortex-M3 — QEMU lm3s6965evb              │
├──────────────┬──────────────┬─────────────┬─────────────────┤
│  ADCS Task   │Health Monitor│  Telemetry  │ Command Handler │
│    50 Hz     │    10 Hz     │    1 Hz     │  event-driven   │
│  Priority 4  │  Priority 5  │ Priority 2  │   Priority 3    │
└──────┬───────┴──────┬───────┴──────┬──────┴────────┬────────┘
       │ mutex        │ heartbeat    │ UART TX        │ UART RX
       │ protected    │ watchdog     │ binary packet  │ cmd byte
       ▼              ▼              ▼                ▼
  g_adcs_state   g_fault_flags   TCP:5555 ──────► Python GUI
                                                  (Tkinter + matplotlib)
```

---

## Features

- **Deterministic 50 Hz ADCS** using `vTaskDelayUntil` — eliminates timing drift that `vTaskDelay` would introduce
- **Priority-inheritance mutexes** protecting shared ADCS state, preventing priority inversion
- **Software watchdog timer** with 2-second expiry using a FreeRTOS one-shot timer — simulates hardware reset on expiry
- **Health monitor task** with ADCS heartbeat checking, stack high-water mark monitoring, and a fault bitmask
- **Binary telemetry over UART** with `0xA5C3` magic number and XOR checksum at 1 Hz
- **Python ground station GUI** with live matplotlib Roll/Pitch/Yaw plots, fault status, and command uplink buttons
- **pytest-verified packet parser** with 5 unit tests covering valid packets, bad magic, corrupted checksums, and short buffers

---

## Stack

`FreeRTOS` | `C` | `ARM Cortex-M3` | `QEMU` | `CMake + Ninja` | `Python` | `Tkinter` | `matplotlib` | `pytest`

---

## How to Build and Run

### Prerequisites

| Tool | Windows Install |
|------|----------------|
| ARM GCC | [developer.arm.com](https://developer.arm.com/downloads/-/gnu-rm) — tick "Add to PATH" |
| CMake | [cmake.org](https://cmake.org) — select "Add to system PATH" |
| Ninja | bundled with CMake or install separately |
| QEMU | [qemu.org/download/#windows](https://www.qemu.org/download/#windows) — add install folder to PATH |
| Python 3.10+ | [python.org](https://www.python.org) — tick "Add Python to PATH" |

### 1. Clone the repository

```bash
git clone https://github.com/YOUR_USERNAME/satellite-flight-computer.git
cd satellite-flight-computer
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git FreeRTOS
```

### 2. Copy FreeRTOSConfig.h into the kernel include path

```bash
copy src\FreeRTOSConfig.h FreeRTOS\Source\include\FreeRTOSConfig.h
```

### 3. Install Python dependencies

```bash
pip install pyserial matplotlib pytest
```

### 4. Build the firmware

```bash
"C:\Program Files\CMake\bin\cmake.exe" -B build -G "Ninja"
"C:\Program Files\CMake\bin\cmake.exe" --build build
```

Expected output:
```
   text    data     bss     dec     hex filename
  19600      84   42820   62504    f428 satellite_fc.elf
```

### 5. Run the full system (2 terminals)

**Terminal 1 — Start QEMU:**
```bash
qemu-system-arm -M lm3s6965evb -kernel build\satellite_fc.elf -serial tcp::5555,server,nowait -display none
```

**Terminal 2 — Launch the ground station GUI:**
```bash
python ground_station/gui.py --port socket://localhost:5555 --baud 115200
```

Click **CONNECT** in the GUI. Roll/Pitch/Yaw values will begin updating every second.

### 6. Run the parser unit tests

```bash
pytest tests/ -v
```

---

## Concepts Demonstrated

| Concept | Where |
|--------|-------|
| Task scheduling and priorities | `src/main.c`, all task files |
| Deterministic timing with `vTaskDelayUntil` | `src/tasks/adcs_task.c` |
| Priority-inversion avoidance via mutex | `src/drivers/uart_driver.c`, `src/tasks/adcs_task.c` |
| ISR vs task separation | `src/startup.c`, FreeRTOS port |
| Software watchdog fault recovery | `src/drivers/watchdog.c`, `src/tasks/health_monitor.c` |
| Binary protocol design with checksum | `src/common/flight_types.h`, `src/tasks/telemetry_task.c` |
| SIL (Software-in-the-Loop) testing | `tests/test_parser.py` |

---

## Key Bugs Fixed During Development

This project required significant debugging beyond the original design. Each issue is a real embedded systems lesson.

### 1. `-lm` placement (sinf/cosf undefined reference)
**Problem:** `target_link_options` placed `-lm` before object files. GCC's linker resolves libraries left-to-right, so `libm` was scanned before the objects that referenced it.
**Fix:** Use `target_link_libraries(satellite_fc.elf PRIVATE m)` which appends `-lm` after all object files.

### 2. Linker script errors (section overlap + `RAM` not declared)
**Problem:** The `MEMORY` block named the region `SRAM` but sections referenced `RAM`. There were also duplicate `.data` sections and conflicting `.init`/`.fini` sections.
**Fix:** Renamed `SRAM` → `RAM`, removed the duplicate `.data` section, and folded `.init`/`.fini` into `.text` with `KEEP()`.

### 3. `-mthumb-interwork` causing dangerous relocations
**Problem:** This flag is for ARMv4/v5 chips mixing ARM and Thumb state. On Cortex-M3 (Thumb-2 only) it confuses the linker, producing "Unknown destination type (ARM/Thumb)" errors.
**Fix:** Removed `-mthumb-interwork` entirely from CMakeLists.txt.

### 4. FreeRTOS interrupt handlers not linked (HardFault on scheduler start)
**Problem:** FreeRTOS ARM_CM3 port defines handlers as `vPortSVCHandler`, `xPortPendSVHandler`, `xPortSysTickHandler`. Without mapping these to the vector table names, all three pointed to `Default_Handler`, causing a HardFault when the scheduler first fired.
**Fix:** Added to `FreeRTOSConfig.h`:
```c
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
```

### 5. Wrong interrupt priority values (tasks never ran after scheduler start)
**Problem:** `configKERNEL_INTERRUPT_PRIORITY 255` used a raw 8-bit value. On Cortex-M3 with 3 priority bits, NVIC priorities must be shifted. Raw `255` masked all interrupts at the wrong level, preventing SVC from ever firing.
**Fix:**
```c
#define configKERNEL_INTERRUPT_PRIORITY      ( 7 << 5 )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY ( 5 << 5 )
```

### 6. Duplicate `FreeRTOSConfig.h` (config changes silently ignored)
**Problem:** The FreeRTOS kernel ships with its own `FreeRTOS/Source/include/FreeRTOSConfig.h` which was found first by the compiler, overriding `src/FreeRTOSConfig.h`.
**Fix:** Deleted the kernel copy and always sync after edits:
```bash
copy src\FreeRTOSConfig.h FreeRTOS\Source\include\FreeRTOSConfig.h
```

### 7. Watchdog firing before health monitor could kick it
**Problem:** `watchdog_init()` was called before `xTaskCreate()`, starting the 2-second countdown before the scheduler launched. The health monitor never got a chance to call `watchdog_kick()`.
**Fix:** Moved `watchdog_init()` to just before `vTaskStartScheduler()`.

### 8. Vector table stripping Thumb bit
**Problem:** Casting function addresses to `uint32_t` strips the Thumb LSB. On Cortex-M3, all function pointers in the vector table must have bit 0 set.
**Fix:** Changed the vector table type to `void (* const isr_vectors[])(void)`, which automatically preserves the Thumb bit.

### 9. `socat` not available on Windows
**Problem:** The original guide used `socat` to create a virtual serial port pair. `socat` is Linux-only.
**Fix:** Used QEMU's built-in TCP serial server (`-serial tcp::5555,server,nowait`) and modified the GUI to connect via raw Python socket.

### 10. pyserial `socket://` URL not supported on Windows
**Problem:** pyserial's `socket://localhost:5555` handler raised `OSError(22)` on Windows, treating the URL as a filesystem path.
**Fix:** Replaced the `_rx` method in `gui.py` to use `socket.create_connection()` directly, bypassing pyserial for the TCP connection.

### 11. Telemetry packets split across TCP reads
**Problem:** TCP delivers data in arbitrary chunks. A 46-byte telemetry packet often arrived across 3-4 separate `recv()` calls, so `find_packet` never saw a complete packet.
**Fix:** Accumulate all incoming bytes into a `bytearray` buffer and only attempt parsing when `len(buf) >= 46`.

---

## Fault Detection Tests

### Test 1 — Watchdog expiry
Comment out `watchdog_kick()` in `health_monitor.c`, rebuild, and run. After 2 seconds:
```
[WDT] WATCHDOG EXPIRED — system reset!
```

### Test 2 — ADCS timeout
Add `vTaskDelay(pdMS_TO_TICKS(5000))` inside the `adcs_task` loop, rebuild, and run:
```
[HM] FAULT: ADCS heartbeat missed!
[HM] tick=... faults=0x01 ...
```

---

## Project Structure

```
satellite-flight-computer/
├── src/
│   ├── common/
│   │   └── flight_types.h        # Shared structs, fault flags, telemetry packet
│   ├── drivers/
│   │   ├── uart_driver.c/h       # UART0 driver with mutex-protected printf
│   │   └── watchdog.c/h          # FreeRTOS one-shot timer watchdog
│   ├── tasks/
│   │   ├── adcs_task.c           # 50 Hz attitude simulation
│   │   ├── telemetry_task.c      # 1 Hz binary downlink
│   │   ├── health_monitor.c      # 10 Hz fault detection + watchdog kick
│   │   └── command_handler.c     # UART command uplink
│   ├── FreeRTOSConfig.h          # Kernel configuration
│   ├── startup.c                 # Vector table + Reset_Handler
│   └── main.c                    # Task creation + scheduler start
├── FreeRTOS/                     # FreeRTOS kernel (git cloned)
├── ground_station/
│   ├── gui.py                    # Tkinter + matplotlib dashboard
│   └── telemetry_parser.py       # Binary packet parser
├── tests/
│   └── test_parser.py            # pytest unit tests
├── linker.ld                     # Memory map for lm3s6965evb
└── CMakeLists.txt                # Build system
```

---

## Resume Line

> "Developed real-time embedded software using FreeRTOS on a QEMU-simulated ARM Cortex-M3 with deterministic 50 Hz task scheduling, priority-inheritance mutex design, software watchdog fault recovery, and binary UART telemetry — validated via a Python ground station GUI with live attitude plots and pytest-verified packet parsing. Debugged and resolved 11 platform-specific issues including interrupt priority misconfiguration, linker script errors, FreeRTOS handler name mismatches, and Windows TCP serial transport."
