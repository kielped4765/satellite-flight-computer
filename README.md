## RTOS Satellite Flight Computer

FreeRTOS firmware simulating a real satellite attitude determination and control system, running on a QEMU-emulated ARM Cortex-M3. Includes binary UART telemetry and a live Python ground station GUI.

Architecture
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
  g_adcs_state   g_fault_flags   UART ──────────► Python GUI
                                                  (Tkinter + matplotlib)
Features
Deterministic 50 Hz ADCS using vTaskDelayUntil — eliminates timing drift that vTaskDelay would introduce
Priority-inheritance mutexes protecting shared ADCS state, preventing priority inversion
Software watchdog timer with 2-second expiry using a FreeRTOS one-shot timer — simulates hardware reset on expiry
Health monitor task with ADCS heartbeat checking, stack high-water mark monitoring, and a fault bitmask
Binary telemetry over UART with 0xA5C3 magic number and XOR checksum at 1 Hz
Python ground station GUI with live matplotlib Roll/Pitch/Yaw plots, fault status, and command uplink buttons
pytest-verified packet parser — 6 unit tests covering valid packets, bad magic, corrupted checksums, and short buffers
Stack

FreeRTOS | C | ARM Cortex-M3 | QEMU | CMake | Python | Tkinter | matplotlib | pytest

Prerequisites
ARM GCC toolchain (arm-none-eabi-gcc)
QEMU (qemu-system-arm)
CMake + Make
Python 3.10+ with pyserial and matplotlib
Git Bash (Windows)
Build & Run

## TEST 1 — Verify all tools are installed
bash
arm-none-eabi-gcc --version
cmake --version
make --version
qemu-system-arm --version
python --version
git --version

Every command should print a version number with no errors.

## TEST 2 — Build from scratch
bash
cd ~/satellite-flight-computer
rm -rf build
cmake -B build -G "MSYS Makefiles"
cmake --build build

Expected output at the end:

   text    data     bss     dec
  19668      84   42820   62572   satellite_fc.elf

## TEST 3 — Run firmware in QEMU
bash
qemu-system-arm -M lm3s6965evb -kernel build/satellite_fc.elf -serial mon:stdio -nographic

Expected output:

=== Satellite Flight Computer Booting ===
Tasks created. Starting scheduler...
[CMD] Ready. Listening for uplink commands...
[HM] tick=1000 faults=0x00 adcs_hb=49
[HM] tick=2000 faults=0x00 adcs_hb=99
Binary telemetry characters appear between health monitor lines — this is correct
adcs_hb increments by ~50 every second confirming the 50 Hz ADCS task is running

Press Ctrl+A then X to quit QEMU.

## TEST 4 — Trigger watchdog expiry

1. Edit src/drivers/watchdog.c:

c
#define WDT_TIMEOUT_MS  500   // changed from 2000

2. Comment out watchdog_kick() in src/tasks/health_monitor.c:

c
/* watchdog_kick(); */

3. Rebuild and run:

bash
cmake --build build
qemu-system-arm -M lm3s6965evb -kernel build/satellite_fc.elf -serial mon:stdio -nographic

Expected output after 500ms:

[WDT] WATCHDOG EXPIRED — system reset!

4. Revert both changes and rebuild:

bash
# Restore WDT_TIMEOUT_MS to 2000
# Uncomment watchdog_kick()
cmake --build build

## TEST 5 — Trigger ADCS fault

1. Add a delay inside the for(;;) loop in src/tasks/adcs_task.c:

c
vTaskDelay(pdMS_TO_TICKS(5000));

2. Rebuild and run:

bash
cmake --build build
qemu-system-arm -M lm3s6965evb -kernel build/satellite_fc.elf -serial mon:stdio -nographic

Expected output:

[HM] FAULT: ADCS heartbeat missed!
[HM] tick=2000 faults=0x01 adcs_hb=0

3. Revert and rebuild:

bash
# Remove the vTaskDelay line
cmake --build build

## TEST 6 — Run pytest suite
bash
cd ~/satellite-flight-computer
pytest tests/ -v

Expected output:

tests/test_parser.py::test_valid_packet                        PASSED
tests/test_parser.py::test_bad_magic_rejected                  PASSED
tests/test_parser.py::test_bad_checksum_rejected               PASSED
tests/test_parser.py::test_too_short_returns_none              PASSED
tests/test_parser.py::test_find_packet_with_garbage_prefix     PASSED
tests/test_parser.py::test_find_packet_incomplete_returns_none PASSED

6 passed

## TEST 7 — Run the ground station GUI
bash
python ground_station/gui.py

On Windows if python does not work, use the full path:

bash
/c/Users/YOUR_USERNAME/AppData/Local/Programs/Python/Python313/python.exe ground_station/gui.py

Exmaple: /c/Users/peder/AppData/Local/Programs/Python/Python313/python.exe ground_station/gui.py

Click CONNECT and verify:

Status changes to ONLINE in green
Roll, Pitch, Yaw values update every second
Attitude plot draws live lines
Mode shows NOMINAL
Faults show NOMINAL
Packet count increments every second


## Docker Hub setup & Test

Pre-Built images are pushed and available on docker hub:

docker pull kielped4765/satellite-firmware:latest
docker pull kielped4765/satellite-groundstation:latest

Then run with commands:

docker-compose up

Open browser at http://localhost:6080/vnc.html to view GUI

## Azure Deployment

The ground station GUI is deployed to Azure Container Instances and accessible publicly.

### Public Access
- **URL:** http://satellite-kiel.eastus.azurecontainer.io:6080/vnc.html
- **IP:** 20.75.152.132
- **Port:** 6080

Open the URL in any browser and click **Connect** to view the live ground station dashboard.

### Azure Details
- **Resource Group:** satellite-rg
- **Container:** satellite-flight-computer
- **Image:** kielped4765/satellite-groundstation:latest
- **Region:** East US
- **CPU:** 1 vCPU
- **Memory:** 1 GB

### Managing the Deployment

**Check status:**
az container show --resource-group satellite-rg --name satellite-flight-computer --query instanceView.state

**View logs:**
az container logs --resource-group satellite-rg --name satellite-flight-computer

**Stop the container:**
az container stop --resource-group satellite-rg --name satellite-flight-computer

**Start the container:**
az container start --resource-group satellite-rg --name satellite-flight-computer