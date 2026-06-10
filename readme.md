# 🏭 Rejection Bin Interlocking System
## ESP32-Based Industrial Part Rejection control and Rejection count Logging system

---

## 📖 Overview

This project implements an ESP32-based industrial rejection bin interlocking system. It stops the machine when a reject is triggered, waits for the operator to confirm the rejected part in the bin, and then resumes production after the confirmation sequence is validated.

**Key Features:**
- Three detection slots with dual-sensor confirmation (A→B sequence)
- **Hybrid storage system** using EEPROM for critical state and SPIFFS for session history
- **Boot-based session tracking** with a separate count for each power cycle
- **Lifetime reject count** (persists forever)
- **Automatic file management** that keeps the last 100 boot sessions
- State persistence across power cycles
- Visual/audio alerts for operator awareness
- 500 ms confirmation beep for sensor events and alerts

---

## 🆕 What's New in v4.0

### **🛡️ Runtime Hardening (Current Build)**

The current firmware includes additional runtime protection features:

- **Non-blocking serial command parser** to avoid loop stalls
- **I2C retry logic** for PCF8574 input/output transactions
- **I2C fail-safe lock** after repeated consecutive bus failures
- **Button debounce filtering** for AUTO/REJECT inputs
- **Reduced web heap churn** through response pre-allocation and streaming

These changes are aimed at long-run stability in production environments.

### **🚀 Hybrid Storage Architecture (EEPROM + SPIFFS)**

**EEPROM Storage (Fast & Reliable):**
| Data              | Purpose                              | Write Speed |
|-------------------|--------------------------------------|-------------|
| `machine_mode`    | AUTO/REJECT state (critical)         | ~3-4ms      |
| `lifetime_count`  | Total rejects (never resets)         | ~3-4ms      |
| `boot_number`     | Power cycles (increments per boot)   | ~3-4ms      |

**SPIFFS Storage (Historical Data):**
| File Name          | Purpose                              | Retention      |
|--------------------|--------------------------------------|----------------|
| `/start_X.txt`     | Rejects per boot session             | Last 100 boots |
| `/version.txt`     | Firmware version tracking            | Current version|

### **Why Hybrid Storage?**

✅ **10x faster writes** - EEPROM writes in 3-4ms vs SPIFFS 50-100ms  
✅ **Power-loss safe** - EEPROM writes are atomic and instant  
✅ **Better wear leveling** - EEPROM has ~100,000 write cycles vs SPIFFS flash ~10,000  
✅ **Reliable state recovery** - Critical data survives sudden power loss  
✅ **Historical tracking** - SPIFFS still maintains session logs for analysis  

### **Storage Architecture**
```
EEPROM (512 bytes)
   Address 0-1   : Magic number (0xABCD)
   Address 2     : machine_mode (AUTO / REJECT)
   Address 3-6   : lifetime_count
   Address 7-10  : boot_number
   Address 11-12 : firmware version

SPIFFS (~1.3 MB available on the selected partition)
   /version.txt  : Firmware version tracking
   /start_1.txt  : Boot 1 session log
   /start_2.txt  : Boot 2 session log
   ...
   /start_100.txt: Boot 100 session log
```

### **Automatic File Management**
- Creates new session file on each ESP32 restart
- Tracks rejects separately for each boot session
- Batch deletes old sessions at boot 101, 201, 301... (keeps last 100)
- Space efficient: ~1 KB for 100 session files

### **Example Timeline**
```
Boot 1:   start_1.txt (5 rejects)    Total: 5
Boot 2:   start_2.txt (3 rejects)    Total: 8
...
Boot 100: start_100.txt (2 rejects)  Total: 5,243
Boot 101: ⚡ DELETE start_1 to start_100
          start_101.txt (7 rejects)  Total: 5,250
```

---

## 📚 Libraries Used

| Library Name | Purpose                                          |
|--------------|--------------------------------------------------|
| `Wire.h`     | I2C communication with PCF8574 expanders         |
| `EEPROM.h`   | Non-volatile storage for critical data           |
| `SPIFFS.h`   | Internal flash file system for session logs      |
| `FS.h`       | Base file system operations                      |
| `WiFi.h`     | WiFi AP mode for web server access               |
| `WebServer.h`| HTTP web server for remote monitoring            |

---

## 🧰 Hardware Components

| Component              | Quantity | Description                                      |
|------------------------|----------|--------------------------------------------------|
| ESP32 DevKit           | 1        | Main microcontroller (1.3 MB SPIFFS + 512B EEPROM) |
| PCF8574 I/O Expander   | 2        | I2C input/output expansion (0x25, 0x26)          |
| Push Button (AUTO)     | 1        | Start machine operation                          |
| Push Button (REJECT)   | 1        | Stop machine and enter reject mode               |
| Proximity Sensor       | 6        | Part detection (2 sensors per slot: A & B)       |
| Relay Module           | 1        | Controls machine AUTO signal to PLC              |
| LED (Machine ON)       | 1        | GREEN LED - indicates when machine is running    |
| LED + Buzzer (Alert)   | 1        | RED LED/Buzzer - active during reject mode       |

---

## 📂 Project Files Reference

### **Header Files (`.h`)**

| File                  | Purpose                                                     |
|-----------------------|-------------------------------------------------------------|
| `config.h`            | Pin definitions, I2C addresses, global variables, constants |
| `eeprom_operations.h` | EEPROM management for machine_mode, boot, lifetime count    |
| `file_operations.h`   | SPIFFS init, session file management, batch deletion        |
| `io_operations.h`     | PCF8574 read/write, output control, beep timing             |
| `process.h`           | Button event handlers (AUTO, REJECT)                        |
| `reject.h`            | Dual-sensor slot detection, rejection sequence handling     |
| `serial_cmd.h`        | Serial command interface (LIST, READ, HELP, RST)            |
| `web_server.h`        | WiFi AP mode, web dashboard, file management interface      |

### **Main Program**

| File                      | Purpose                                                    |
|---------------------------|------------------------------------------------------------|
| `r_bin_interlock_sys.ino` | Main program loop, initialization sequence, input polling  |

---

## 🔌 Pin Mapping

### **ESP32 Hardware Pins**

| Pin    | Function | Description          |
|--------|----------|----------------------|
| GPIO21 | SDA      | I2C data line        |
| GPIO22 | SCL      | I2C clock line       |

### **PCF8574 #1 (Address 0x25) - Input Expander**

| Pin | Function       | Description                      |
|-----|----------------|----------------------------------|
| 0   | SLOT3_B        | Slot 3 Sensor B (back)           |
| 1   | SLOT3_A        | Slot 3 Sensor A (front)          |
| 2   | SLOT2_B        | Slot 2 Sensor B (back)           |
| 3   | SLOT2_A        | Slot 2 Sensor A (front)          |
| 4   | SLOT1_B        | Slot 1 Sensor B (back)           |
| 5   | SLOT1_A        | Slot 1 Sensor A (front)          |
| 6   | BTN_REJECT     | REJECT/E-STOP button input       |
| 7   | BTN_AUTO       | AUTO button input                |

### **PCF8574 #2 (Address 0x26) - Output Expander**

| Pin | Function       | Description                      |
|-----|----------------|----------------------------------|
| 0   | RELAY_AUTO     | AUTO relay output to PLC         |
| 1   | LED_MACHINE_ON | GREEN LED - Machine running      |
| 2   | BUZZER_LED     | RED LED + Buzzer (500ms beeps)   |
| 3-7 | UNUSED         | Available for future expansion   |

---

## 🛠️ Software Requirements

| Software/Tool          | Version | Description                                                |
|------------------------|---------|-------------------------------------------------------------|
| Arduino IDE            | 2.x+    | Development environment for ESP32 programming              |
| ESP32 Board Package    | 2.x-3.x | ESP32 board support for Arduino IDE                        |
| Serial Monitor         | Any     | Debug and monitor system events (115200 baud)              |
| WiFi Client (Browser)  | Any     | Access web dashboard (Chrome, Firefox, Safari, Edge, etc.) |

---

## ⚙️ System Operation Modes

### **AUTO Mode (Normal Production)**
```
✓ Machine relay: ON (energized)
✓ GREEN LED: ON (Machine running indicator)
✓ Alerts: OFF (silent operation)
✓ Monitoring: Sensors active, waiting for part detection
✓ Serial Output: Normal operation messages
```

### **REJECT Mode (Waiting for Part Confirmation)**
```
⚠ Machine relay: OFF (de-energized, PLC signal halted)
⚠ RED LED/Buzzer: ACTIVE (500ms beeps on sensor triggers)
⚠ GREEN LED: OFF
⚠ System State: Locked, waiting for part confirmation
⚠ AUTO button: IGNORED until part confirmed
⚠ Serial Output: Alert and waiting messages
```

---

## 📊 Part Detection Sequence

### **Valid Rejection Workflow**
```
1. REJECT button pressed (operator triggers rejection)
   ↓ Machine stops, enters REJECT mode
   ↓ Machine mode saved to EEPROM (instant, power-safe)
   
2. Operator places rejected part in bin
   ↓ Part enters slot (Sensor A triggers)
   ↓ BEEP notification (500ms)
   
3. Part fully seats (Sensor B triggers)
   ↓ BEEP notification (500ms)
   
4. A→B sequence confirmed
   ↓ Machine mode = AUTO (saved to EEPROM)
   ↓ Continuous anti-cheat monitoring remains active
   
5. The part is counted when the next cycle is finalized
   ↓ Lifetime count incremented (saved to EEPROM)
   ↓ Session count incremented (saved to SPIFFS)
   ↓ Ready for the next operator action
```

### **Cheat Detection (Anti-Removal)**
```
While continuous monitoring is active after A→B confirmation:
  - If Sensor A triggers again after Sensor B (B→A) = CHEATING DETECTED
  - Count NOT incremented
  - Machine stops immediately
  - System enters REJECT mode (saved to EEPROM)
  - Triple beep alert (emergency notification)
  - Part must be re-confirmed in bin before restart
```

### **I2C Fail-safe Behavior**
```
If I2C communication repeatedly fails:
   - Each read/write is retried (I2C_RETRY_COUNT)
   - Consecutive failures are counted
   - At threshold (I2C_FAILSAFE_THRESHOLD), system enters fail-safe
   - Machine is forced to REJECT mode
   - Relay is turned OFF
   - Mode is persisted to EEPROM
   - Triple beep alert is triggered
```

---

## 💾 Data Persistence Architecture

### **EEPROM Storage (Critical Data)**

| Address | Data              | Size    | Purpose                          | Write Frequency |
|---------|-------------------|---------|----------------------------------|-----------------|
| 0-1     | Magic Number      | 2 bytes | First boot detection (0xABCD)    | Once (first boot) |
| 2       | machine_mode      | 1 byte  | AUTO/REJECT state                | Every reject cycle |
| 3-6     | lifetime_count    | 4 bytes | Total rejects (never resets)     | Every part confirmation |
| 7-10    | boot_number       | 4 bytes | Power cycles                     | Every boot |
| 11-12   | firmware version  | 2 bytes | Detect firmware changes          | On firmware update |

**EEPROM Characteristics:**
- Write time: 3-4ms (instant)
- Write cycles: ~100,000 per cell
- Power-loss safe: Atomic writes
- Survives: Power glitches, brown-outs, sudden shutdowns

### **SPIFFS Storage (Session History)**

| File Name          | Purpose                        | Retention      | Contents        |
|--------------------|--------------------------------|----------------|-----------------|
| `/start_X.txt`     | Reject count per boot session  | Last 100 boots | Single integer  |
| `/version.txt`     | Firmware version tracking      | Current version| Single integer  |

**SPIFFS Characteristics:**
- Write time: 50-100ms
- Total capacity: 1.3 MB
- Session logs: ~10 bytes each
- Batch management: Auto-deletes old files

### **Automatic File Management**

**Session File Organization:**
- Each ESP32 restart creates new `/start_X.txt` file
- Tracks rejects separately for each boot session
- Batch deletion triggers every 100 boots (at boot 101, 201, 301...)
- Always maintains last 100 session files

**Example Timeline:**
```
Boot 1:   /start_1.txt (5 rejects)     Lifetime Total: 5 (EEPROM)
Boot 2:   /start_2.txt (3 rejects)     Lifetime Total: 8 (EEPROM)
Boot 3:   /start_3.txt (7 rejects)     Lifetime Total: 15 (EEPROM)
...
Boot 100: /start_100.txt (2 rejects)   Lifetime Total: 5,243 (EEPROM)
Boot 101: ⚡ BATCH DELETE: /start_1.txt through /start_100.txt removed
          /start_101.txt (0 rejects)   Lifetime Total: 5,243 (EEPROM)
Boot 102: /start_102.txt (4 rejects)   Lifetime Total: 5,247 (EEPROM)
```

### **Power Loss Recovery**

**Scenario 1: Power loss during reject cycle**
```
✅ machine_mode saved in EEPROM before power loss
✅ System boots in REJECT mode
✅ Operator must confirm part in bin
✅ No data corruption possible
```

**Scenario 2: Power loss during count increment**
```
If power lost BEFORE EEPROM write:
  ⚠ That one reject might not be counted (rare)
  
If power lost AFTER EEPROM write:
  ✅ Count is saved (typical case)
  ✅ System recovers correctly
```

### **Storage Capacity**

| Parameter            | Value                              |
|----------------------|------------------------------------|
| EEPROM size          | 512 bytes                          |
| EEPROM used          | 13 bytes (3% utilization)          |
| Session file size    | ~10 bytes per file                 |
| Max session files    | 100 (configurable in `config.h`)   |
| Total SPIFFS used    | ~1 KB for all session data         |
| SPIFFS capacity      | 1.3 MB                             |
| Space utilization    | 0.08% (extremely efficient)        |

---

## 🌐 Web Server & WiFi

### **WiFi AP Configuration**
```
Network Name (SSID): RejectionBin_AP
Password:            rejectionbin
IP Address:          192.168.1.21
Gateway:             192.168.1.21
Subnet:              255.255.255.0
```

### **Web Dashboard Features**
```
URL: http://192.168.1.21

Routes:
├── /                 Dashboard
├── /sessions         All boot session files
├── /files            All files currently in SPIFFS
├── /view?file=...    View a file in the browser
├── /download?file=... Download a single file
└── /downloadall      Export all available data as plain text
```

### **Web Dashboard Display**
- Current boot number stored in EEPROM
- Current session reject count stored in the current `/start_X.txt`
- Lifetime total rejects stored in EEPROM
- Machine mode stored in EEPROM (`REJECT` / `AUTO`)
- Recent session history and file browser
- File download functionality

### **Important note**
The dashboard shows placeholders for `/boot_number.txt`, `/total_count.txt`, and `/state.txt` in the code, but those files are not created by the firmware. The actual persistent files are `/version.txt` and the `/start_X.txt` session logs.

---

## 🖥️ Serial Command Interface

### **Serial Port Settings**
```
Baud Rate: 115200
Data Bits: 8
Stop Bits: 1
Parity:    None
```

### **Available Serial Commands**

| Command | Parameters | Purpose | Example |
|---------|------------|---------|---------|
| `LIST` | None | Display EEPROM values and SPIFFS files | `LIST` |
| `READ` | `<file_path>` | Display specific file contents | `READ /start_1.txt` |
| `RST` | None | Restart ESP32 | `RST` |
| `HELP` | None | Show the help menu and AP details | `HELP` |

**Serial handling note:** Command parsing is non-blocking and line-buffered for better loop responsiveness.

### **Serial Output During Operation**

**Boot Sequence:**
```
WELCOME ESP32 : REJECTION BIN INTERLOCKING SYSTEM
=== EEPROM INIT ===
EEPROM already initialized
[EEPROM] Boot Number: 25
[EEPROM] Lifetime Count: 1847
[EEPROM] Machine Mode: AUTO
===================
=== SPIFFS INIT ===
SPIFFS Mounted successfully
Total: 1.30 MB | Used: 0.05 MB | Free: 1.25 MB
===================
[SPIFFS] Created session file: /start_25.txt
-------------SYSTEM STATUS-------------
Boot Number:          25
Session Count:        0
Lifetime rejection:   1847
Machine Mode:         AUTO
----------------------------------------
SYSTEM READY
Press AUTO button to start machine
```

**During Operation:**
```
AUTO MODE - MACHINE RUNNING
[SENSOR] Slot 1A detected - waiting for 1B...
[SENSOR] Slot 1B detected - A→B sequence complete
[EEPROM] Saved Machine Mode: AUTO
[MONITORING] Started continuous monitoring for Slot 1
[MONITORING] Part confirmed in Slot 1
Part confirmed in Slot 1 (no B→A removal detected)
Session Count (Boot #25): 1
Lifetime Total Count: 1848
[CONTINUOUS CHECK] Monitoring for post-count removal attempts...
Ready for next cycle
```

**Cheat Detection:**
```
[CHEATING DETECTED!] B→A removal sequence - Part being removed
SYSTEM STOPPING - Part already counted, cannot be removed!
[SYSTEM STOPPED] Machine halted due to cheating detection!
Session Count: 5
Lifetime Count: 1852
[EEPROM] Saved Machine Mode: REJECT
```

---

## 🚀 Getting Started

### **Setup Instructions**

1. **Hardware Assembly**
   - Connect ESP32 to PCF8574 modules via I2C (GPIO21=SDA, GPIO22=SCL)
   - Connect buttons, sensors, and relays to appropriate PCF8574 pins
   - Power up the system

2. **Firmware Upload**
   - Open `r_bin_interlock_sys.ino` in Arduino IDE
   - Select Board: ESP32 Dev Module
   - Install ESP32 board package if not present
   - Upload sketch to device

3. **Initial Testing**
   - Open Serial Monitor (115200 baud)
   - Verify EEPROM initialization (first boot will show "initializing EEPROM")
   - Test AUTO button (should start machine)
   - Test REJECT button (should stop machine)
   - Test sensor sequences (A then B)
   - Verify counts increment in EEPROM

4. **Web Dashboard Access**
   - Connect to WiFi: `RejectionBin_AP`
   - Navigate to: `http://192.168.1.21`
   - View system status and session files

---

## 🔧 Configuration

### **Modifiable Parameters in `config.h`**

```cpp
#define MAX_START_FILES       100   // Keep last N boot sessions
#define POLL_INTERVAL         20    // Input polling frequency (ms)
#define I2C_RETRY_COUNT       3     // Retries per I2C transaction
#define I2C_FAILSAFE_THRESHOLD 5    // Consecutive failures before lock
#define BUTTON_DEBOUNCE_MS    120   // Min interval between valid button presses
#define BEEP_DURATION         500   // Beep length (ms)
#define FIRMWARE_VERSION      4     // Bump to reset EEPROM + SPIFFS on update
```

### **EEPROM Memory Map in `eeprom_operations.h`**

```cpp
#define EEPROM_SIZE           512     // Total EEPROM allocation
#define EEPROM_MAGIC          0xABCD  // First boot detection
#define ADDR_MAGIC            0       // Magic number address
#define ADDR_MACHINE_MODE     2       // Machine mode address
#define ADDR_LIFETIME_COUNT   3       // Lifetime count address
#define ADDR_BOOT_NUMBER      7       // Boot number address
#define ADDR_FW_VERSION      11       // Firmware version address
```

---

## 📝 System Features Summary

| Feature | Details |
|---------|---------|
| **Hybrid Storage** | EEPROM for critical data + SPIFFS for history |
| **Multi-Slot Detection** | 3 slots, each with dual sensors (A & B) |
| **Anti-Cheat Monitoring** | Continuous surveillance after A→B sequence |
| **Boot Tracking** | Counts power cycles and stores per-boot data |
| **Lifetime Tracking** | Never-reset total reject counter in EEPROM |
| **Automatic Cleanup** | Batch deletes old session files every 100 boots |
| **Power-Loss Safe** | EEPROM ensures data integrity |
| **Fast Writes** | 3-4ms EEPROM writes (10x faster than SPIFFS) |
| **Web Interface** | WiFi AP mode with file browser and downloader |
| **Serial Debugging** | Full command interface for monitoring |
| **LED/Buzzer Alerts** | Visual and audio feedback for reject mode |
| **Relay Control** | Machine start/stop via digital relay output |
| **I2C Expansion** | 8 inputs + 8 outputs via PCF8574 modules |
| **I2C Fail-safe Lock** | Forces REJECT mode after repeated I2C faults |
| **Button Debounce** | Filters bounce on AUTO/REJECT button edges |
| **Non-blocking Serial** | Prevents command input from stalling control loop |

---

## 🐛 Troubleshooting

| Issue                       | Solution                                  |
|-----------------------------|-------------------------------------------|
| "EEPROM not initialized"    | System will auto-initialize on first boot |
| "SPIFFS Mount failed"       | Format SPIFFS from Arduino IDE Tools menu |
| Files not persisting        | Check SPIFFS partition in board settings  |
| EEPROM data corrupted       | Magic number mismatch will trigger reset  |
| I2C errors                  | Verify PCF8574 addresses (0x25, 0x26)     |
| Counters reset unexpectedly | Check EEPROM initialization messages      |
| Sensors not detecting       | Verify PCF1 wiring and sensor power       |
| Web dashboard not accessible| Check WiFi AP settings (SSID/Password)   |

---

## 📈 Performance Metrics

| Metric                | Value                 |
|-----------------------|-----------------------|
| Sensor poll rate      | 50 Hz (20ms interval) |
| Edge detection        | Rising edge only      |
| Button debounce       | 120 ms                |
| Beep duration         | 500ms                 |
| EEPROM write time     | 3-4ms                 |
| SPIFFS write time     | 50-100ms              |
| I2C retries/op        | 3                     |
| I2C fail-safe threshold | 5 consecutive failures |
| I2C Speed             | 100 kHz               |
| EEPROM write cycles   | ~100,000 per cell     |
| Flash write cycles    | ~10,000 per block     |

---

## 🔒 Safety Features

✅ **Machine interlock** - Cannot start until rejected part confirmed  
✅ **State persistence** - EEPROM ensures state survives power loss  
✅ **Dual-sensor confirmation** - Prevents false positives  
✅ **Anti-cheat monitoring** - Detects part removal attempts (B→A sequence)  
✅ **Visual/audio alerts** - Operator awareness  
✅ **Data redundancy** - EEPROM + SPIFFS backup  
✅ **Automatic cleanup** - Prevents filesystem overflow  
✅ **Power-loss recovery** - Atomic EEPROM writes  
✅ **Fast response** - Critical writes complete in 3-4ms  
✅ **I2C fail-safe lock** - Stops machine safely on repeated bus errors  
✅ **Button debounce filter** - Reduces false trigger events  

---

## 🗂️ Change Tracking

For a chronological list of firmware updates, see:

- `readme_updates.md`

---

## 📄 License

This project is proprietary software. All rights reserved.

---

## 👨‍💻 Development Notes

- **Code Organization:** Modular header-based design for easy maintenance
- **Hybrid Storage:** EEPROM for speed/reliability, SPIFFS for history
- **I2C Clock Speed:** 100 kHz for stable communication
- **Edge Detection:** Rising edge detection for button/sensor inputs
- **Non-blocking Beep:** Beep timing managed in `update_outputs()` loop
- **EEPROM Magic Number:** 0xABCD for first boot detection
- **Wear Leveling:** EEPROM handles frequent writes better than flash

---

## 🔄 Migration from v2.0 to v3.0

If upgrading from SPIFFS-only version:

1. **First boot will auto-migrate:**
   - EEPROM initializes with default values
   - Existing SPIFFS files remain intact
   - Boot number continues from last value
   - Lifetime count may reset (one-time)

2. **Manual migration (optional):**
   - Note your current boot_number and total_count
   - Upload new firmware
   - Use serial commands to verify EEPROM values
   - Adjust if needed via code modification

---

## ⚠️ Disclaimer

This system is designed as a safety interlock and should not be the sole safety mechanism. Always follow proper industrial safety protocols and implement redundant safety systems for critical applications.