# 🏭 Rejection Bin Interlocking System
## ESP32-Based Industrial Part Rejection Control with Multi-Boot Session Tracking

---

## 📖 Overview

This project implements an advanced industrial rejection bin interlocking system using an ESP32 microcontroller. It ensures safety in manufacturing by stopping the machine when a defective part is detected and preventing restart until the rejected part is confirmed in the bin. 

**Key Features:**
- Three detection slots with dual-sensor confirmation (A→B sequence)
- **Boot-based session tracking** (separate count per power cycle)
- **Lifetime reject count** (persists forever)
- **Automatic file management** (keeps last 100 boot sessions, batch deletes old files)
- State persistence across power cycles using SPIFFS
- Visual/audio alerts for operator awareness
- 200ms confirmation pulse to PLC

---

## 🆕 What's New in v2.0

### **Multi-File Storage System**
| File Name          | Purpose                              | Retention      |
|--------------------|--------------------------------------|----------------|
| `/boot_number.txt` | Total power cycles (never resets)    | Forever        |
| `/total_count.txt` | Lifetime reject count (never resets) | Forever        |
| `/state.txt`       | Machine mode (REJECT/AUTO)           | Current state  |
| `/start_X.txt`     | Rejects per boot session             | Last 100 boots |

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
| `SPIFFS.h`   | Internal flash file system for data persistence  |
| `FS.h`       | Base file system operations                      |

---

## 🧰 Hardware Components

| Component              | Quantity | Description                                      |
|------------------------|----------|--------------------------------------------------|
| ESP32 DevKit           | 1        | Main microcontroller (1.3 MB SPIFFS)             |
| PCF8574 I/O Expander   | 2        | I2C input/output expansion (0x25, 0x26)          |
| Push Button (AUTO)     | 1        | Start machine operation                          |
| Push Button (REJECT)   | 1        | Stop machine and enter reject mode               |
| Proximity Sensor       | 6        | Part detection (2 sensors per slot: A & B)       |
| Relay Module           | 1        | Controls machine AUTO signal to PLC              |
| LED (REJECT Indicator) | 1        | Shows when system is in reject mode              |
| LED (Alert)            | 1        | Blinks during reject mode (500ms interval)       |
| Buzzer                 | 1        | Audio alert during reject mode                   |

---

## 🛠️ Software Requirements

| Software/Tool          | Description                                                        |
|------------------------|--------------------------------------------------------------------|
| Arduino IDE            | Development environment for ESP32 programming                      |
| ESP32 Board Package    | ESP32 board support (version 2.x or 3.x)                           |
| Serial Monitor         | Debug and monitor system events (115200 baud)                      |

---

## 📁 File Structure & Description

| File                  | Purpose                                                    |
|-----------------------|------------------------------------------------------------|
| `main.ino`            | Main program loop, initialization sequence, input polling  |
| `config.h`            | Pin definitions, I2C addresses, global variables, constants|
| `file_operations.h`   | SPIFFS init, boot tracking, session files, state management|
| `io_operations.h`     | PCF8574 read/write, output control, buzzer blinking        |
| `process.h`           | Button event handlers (AUTO, REJECT)                       |
| `reject.h`            | Dual-sensor slot detection, rejection sequence handling    |

**Total Project Size:** ~550 lines of code

---

## 🔌 Pin Mapping

### **PCF1 (0x25) - Input Expander**

| Pin | Function       | Description                      |
|-----|----------------|----------------------------------|
| 0   | BTN_AUTO       | AUTO button input                |
| 1   | BTN_REJECT     | REJECT/E-STOP button input       |
| 2   | SLOT1_A        | Slot 1 Sensor A (front)          |
| 3   | SLOT1_B        | Slot 1 Sensor B (back)           |
| 4   | SLOT2_A        | Slot 2 Sensor A (front)          |
| 5   | SLOT2_B        | Slot 2 Sensor B (back)           |
| 6   | SLOT3_A        | Slot 3 Sensor A (front)          |
| 7   | SLOT3_B        | Slot 3 Sensor B (back)           |

### **PCF2 (0x26) - Output Expander**

| Pin | Function       | Description                      |
|-----|----------------|----------------------------------|
| 0   | RELAY_AUTO     | AUTO relay output to PLC         |
| 1   | LED_REJECT     | REJECT mode indicator LED        |
| 2   | OUT_PULSE      | Confirmation pulse (200ms)       |
| 7   | BUZZER_LED     | Buzzer/Alert LED (blinks 500ms)  |

### **ESP32 I2C Pins**

| Pin    | Function | Description          |
|--------|----------|----------------------|
| GPIO21 | SDA      | I2C data line        |
| GPIO22 | SCL      | I2C clock line       |

---

## ⚙️ System Operation

### **AUTO Mode (Normal Operation)**
```
✓ Machine running
✓ AUTO relay energized (PLC receives signal)
✓ System monitors for REJECT button
✓ All alerts OFF
✓ Reject count increments on valid detections
```

### **REJECT Mode (Waiting for Part)**
```
⚠ Machine stopped
⚠ AUTO relay de-energized (PLC signal OFF)
⚠ REJECT LED continuously ON
⚠ Buzzer/LED blinking (500ms interval)
⚠ System waiting for part confirmation
⚠ AUTO button ignored until part confirmed
```

### **Part Detection Sequence**
1. **Sensor A triggers** → Part entering slot (front sensor)
2. **System locks to that slot** → Only monitors that slot's Sensor B
3. **Sensor B triggers** → Part fully seated (back sensor)
4. **Valid A→B sequence confirmed:**
   - Session count incremented → `/start_X.txt`
   - Total count incremented → `/total_count.txt`
   - Machine mode saved → `/state.txt`
   - 200ms confirmation pulse sent to PLC
   - System exits REJECT mode
   - Ready for AUTO restart

---

## 💾 File System Architecture

### **Boot Tracking Logic**
```
On ESP32 Power-On:
1. Read /boot_number.txt → e.g., 150
2. Increment → 151
3. Check if (boot_number > 100) AND (boot_number - 1) % 100 == 0
   → If true: Batch delete files start_51 to start_150
4. Create /start_151.txt → Initialize to 0
5. Load /total_count.txt → Lifetime total
6. Load /state.txt → Machine mode
```

### **Storage Limits**
| Parameter            | Value                              |
|----------------------|------------------------------------|
| Max session files    | 100 (configurable in `config.h`)   |
| Batch delete trigger | Every 100 boots (101, 201, 301...) |
| Space per file       | ~10 bytes                          |
| Total space used     | ~1 KB (100 files)                  |
| Available SPIFFS     | 1.3 MB                             |
| Space utilization    | 0.08%                              |

---

## 🖨️ Serial Output Format

### **Boot Sequence**
```
╔══════════════════════════════════════════════════╗
║  ESP32 REJECTION BIN INTERLOCKING SYSTEM   ║
╚══════════════════════════════════════════════════╝

--- I2C INITIALIZATION ---
✓ PCF1 configured (inputs)
✓ PCF2 configured (outputs)

=== FILESYSTEM INITIALIZATION ===
✓ SPIFFS Mounted successfully
Total: 1.30 MB | Used: 0.02 MB | Free: 1.28 MB
=====================================

[BOOT] Boot Number: 151
[SESSION] Created: /start_151.txt

[TOTAL] Lifetime Count: 12,345

[STATE] Machine Mode: AUTO

-------------------- SYSTEM STATUS ---------------------
Boot Number:          151
Session Count:        0
Lifetime Total:       12,345
Machine Mode:         AUTO

🟢 SYSTEM READY
   Press AUTO button to start machine
```

### **Operation Messages**
```
AUTO MODE - MACHINE RUNNING

REJECT MODE - MACHINE STOPPED
Waiting for part in rejection bin...

[SENSOR] Slot 2A detected - waiting for 2B...

========================================
✓ Part Confirmed in Slot 2
✓ Session Count (Boot #151): 7
✓ Lifetime Total Count: 12,352
✓ Ready for next cycle
========================================
```

### **Batch Delete Event (Boot 101, 201, etc.)**
```
=== BATCH DELETE: Removing old session files ===
  Deleted: /start_1.txt
  Deleted: /start_2.txt
  ...
  Deleted: /start_100.txt
✓ Deleted files from start_1 to start_100
================================================
```

---

## 🚀 Installation & Setup

### **1. Hardware Connections**
```
ESP32 → PCF8574 #1 (0x25):
  GPIO21 → SDA
  GPIO22 → SCL
  3.3V → VCC
  GND → GND

ESP32 → PCF8574 #2 (0x26):
  (Share same I2C bus)

PCF8574 #1 → Buttons & Sensors
PCF8574 #2 → Relays, LEDs, Buzzer
```

### **2. Software Upload**
1. Install Arduino IDE
2. Add ESP32 board support
3. Select Board: "ESP32 Dev Module"
4. Select Port: (Your ESP32 COM port)
5. Upload sketch

### **3. Initial Test**
1. Open Serial Monitor (115200 baud)
2. Verify boot number starts at 1
3. Press AUTO → Machine should start
4. Press REJECT → Machine stops, buzzer blinks
5. Place part in slot → Counters increment
6. Power cycle → Boot number increments

---

## 🔧 Configuration

### **Modify File Limits**
In `config.h`:
```cpp
#define MAX_START_FILES 100  // Change to 50, 200, etc.
```

### **Change Timing**
```cpp
#define POLL_INTERVAL   20   // Sensor polling (ms)
#define BUZZER_INTERVAL 500  // Buzzer blink rate (ms)
```

---

## 📊 Data Persistence Examples

### **Scenario 1: Normal Operation**
```
Boot 1: Machine runs, 5 rejects
  → /start_1.txt = 5
  → /total_count.txt = 5

Power Off/On

Boot 2: Machine runs, 3 rejects
  → /start_2.txt = 3
  → /total_count.txt = 8
```

### **Scenario 2: Batch Delete**
```
Boot 100: 
  Files: start_1.txt to start_100.txt
  Total count: 5,243

Power Off/On

Boot 101: ⚡ BATCH DELETE
  Files deleted: start_1.txt to start_100.txt
  Files remaining: start_101.txt
  Total count: 5,243 (preserved!)
```

---

## 🐛 Troubleshooting

| Issue                       | Solution                                  |
|-----------------------------|-------------------------------------------|
| "SPIFFS Mount failed"       | Format SPIFFS from Arduino IDE Tools menu |
| Files not persisting        | Check SPIFFS partition in board settings  |
| I2C errors                  | Verify PCF8574 addresses (0x25, 0x26)     |
| Counters reset unexpectedly | Check file system integrity               |
| Sensors not detecting       | Verify PCF1 wiring and sensor power       |

---

## 📈 Performance Metrics

| Metric             | Value                 |
|--------------------|-----------------------|
| Sensor poll rate   | 50 Hz (20ms interval) |
| Edge detection     | Rising edge only      |
| Confirmation pulse | 200ms duration        |
| Buzzer blink rate  | 1 Hz (500ms on/off)   |
| File write time    | < 10ms                |
| Boot time          | ~2 seconds            |

---

## 🔒 Safety Features

✅ **Machine interlock** - Cannot start until rejected part confirmed  
✅ **State persistence** - System remembers reject mode after power loss  
✅ **Dual-sensor confirmation** - Prevents false positives  
✅ **Visual/audio alerts** - Operator awareness  
✅ **Data redundancy** - Multiple file backups  
✅ **Automatic cleanup** - Prevents filesystem overflow  

---

## 👨‍💻 Author

**Vijay Magadum**  

---

## ⚠️ Disclaimer

This system is designed as a safety interlock and should not be the sole safety mechanism. Always follow proper industrial safety protocols and implement redundant safety systems for critical applications.