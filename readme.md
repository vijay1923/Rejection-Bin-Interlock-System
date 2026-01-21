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
- 500ms confirmation beep to operator

---

## 🆕 What's New in v2.0

### **Multi-File Storage System**
| File Name          | Purpose                              | Retention      |
|--------------------|--------------------------------------|----------------|
| `/boot_number.txt` | Total power cycles (never resets)    | Forever        |
| `/total_count.txt` | Lifetime reject count (never resets) | Forever        |
| `/state.txt`       | Machine mode (REJECT/AUTO)           | Current state  |
| `/start_X.txt`     | Rejects per boot session             | Last 100 boots |
| `/version.txt`     | Firmware version tracking            | Current version|

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
| `WiFi.h`     | WiFi AP mode for web server access               |
| `WebServer.h`| HTTP web server for remote monitoring            |

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
| LED (Machine ON)       | 1        | GREEN LED - indicates when machine is running    |
| LED + Buzzer (Alert)   | 1        | RED LED/Buzzer - active during reject mode       |

---

## 📂 Project Files Reference

### **Header Files (`.h`)**

| File                  | Purpose                                                     |
|-----------------------|-------------------------------------------------------------|
| `config.h`            | Pin definitions, I2C addresses, global variables, constants |
| `file_operations.h`   | SPIFFS init, boot tracking, session files, state management |
| `io_operations.h`     | PCF8574 read/write, output control, beep timing             |
| `process.h`           | Button event handlers (AUTO, REJECT)                        |
| `reject.h`            | Dual-sensor slot detection, rejection sequence handling     |
| `serial_cmd.h`        | Serial command interface (LIST, READ, HELP)                 |
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
|------------------------|---------|-------|
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
⚠ RED LED/Buzzer: BLINKING (500ms on/off cycle)
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
   ↓ RED LED/Buzzer begins blinking (500ms interval)
   
2. Operator places rejected part in bin
   ↓ Part enters slot (Sensor A triggers)
   ↓ BEEP notification (500ms)
   
3. Part fully seats (Sensor B triggers)
   ↓ BEEP notification (500ms)
   
4. A→B sequence confirmed
   ↓ Monitoring mode: 5 seconds of anti-cheat surveillance
   
5. Monitoring complete (no removal detected)
   ↓ Session reject count incremented
   ↓ Lifetime reject count incremented
   ↓ Machine returns to AUTO mode
   ↓ Ready for operator to press AUTO button
```

### **Cheat Detection (Anti-Removal)**
```
During 5-second monitoring window after A→B sequence:
  - If Sensor A triggers again = CHEATING DETECTED
  - Count NOT incremented
  - Part must be re-confirmed in bin before restart
  - System remains in REJECT mode
  - Alert message sent to serial monitor
```

---

## 💾 Data Persistence (File System)

### **Stored Files in SPIFFS**

| File Name          | Purpose                        | Retention      | Contents        |
|--------------------|--------------------------------|----------------|-----------------|
| `/boot_number.txt` | Total power cycles             | Forever        | Single integer  |
| `/total_count.txt` | Lifetime reject count          | Forever        | Single integer  |
| `/state.txt`       | Current machine mode           | Current state  | Single byte (0=AUTO, 1=REJECT) |
| `/start_X.txt`     | Reject count per boot session  | Last 100 boots | Single integer  |
| `/version.txt`     | Firmware version tracking      | Current version| Single integer  |

### **Automatic File Management**

**Session File Organization:**
- Each ESP32 restart creates new `/start_X.txt` file
- Tracks rejects separately for each boot session
- Batch deletion triggers every 100 boots (at boot 101, 201, 301...)
- Always maintains last 100 session files

**Example Timeline:**
```
Boot 1:   /start_1.txt (5 rejects)     Lifetime Total: 5
Boot 2:   /start_2.txt (3 rejects)     Lifetime Total: 8
Boot 3:   /start_3.txt (7 rejects)     Lifetime Total: 15
...
Boot 100: /start_100.txt (2 rejects)   Lifetime Total: 5,243
Boot 101: ⚡ BATCH DELETE: /start_1.txt through /start_100.txt removed
          /start_101.txt (0 rejects)   Lifetime Total: 5,243
Boot 102: /start_102.txt (4 rejects)   Lifetime Total: 5,247
```

### **Storage Capacity**

| Parameter            | Value                              |
|----------------------|------------------------------------|
| Session file size    | ~10 bytes per file                 |
| Max session files    | 100 (configurable in `config.h`)   |
| Total space used     | ~1 KB for all session data         |
| SPIFFS capacity      | 1.3 MB                             |
| Space utilization    | 0.08% (extremely efficient)        |
| Partition strategy   | Last 100 boots always retained     |

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

Pages:
├── Dashboard (/)        - Current system status, core files
├── All Sessions (/sessions) - View all boot session files
└── Download All (/downloadall) - Export all SPIFFS data
```

### **Web Dashboard Display**
- Current boot number
- Current session reject count
- Lifetime total rejects
- Machine mode (REJECT/AUTO)
- Core file viewer (boot_number, total_count, state)
- Session file browser (start_1 through start_100)
- File download functionality

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
| `LIST` | None | Display all SPIFFS files | `LIST` |
| `READ` | `<file_path>` | Display specific file contents | `READ /total_count.txt` |
| `HELP` | None | Show command help menu | `HELP` |

### **Serial Output During Operation**

**Boot Sequence:**
```
WELCOME ESP32 : REJECTION BIN INTERLOCKING SYSTEM
FILE INIT
SPIFFS Mounted successfully
Total: 1.30 MB | Used: 0.05 MB | Free: 1.25 MB
[BOOT] Boot Number: 25
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
[MONITORING] Started for Slot 1 - checking for removal (Duration: 5000 ms)
[MONITORING] Complete - Part confirmed safe
Part Confirmed in Slot 1
Session Count (Boot #25): 1
Lifetime Total Count: 1848
Ready for next cycle
```

**Reject Mode:**
```
REJECT MODE - MACHINE STOPPED
Waiting for part in rejection bin...
[MONITORING] Cheating detected! Sensor A triggered during monitoring
Part being removed - Count NOT incremented!
Please place part properly in the bin
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
   - Verify boot sequence messages
   - Test AUTO button (should start machine)
   - Test REJECT button (should stop machine)
   - Test sensor sequences (A then B)

4. **Web Dashboard Access**
   - Connect to WiFi: `RejectionBin_AP`
   - Navigate to: `http://192.168.1.21`
   - View system status and data files

---

## 🔧 Configuration

### **Modifiable Parameters in `config.h`**

```cpp
#define MAX_START_FILES 100      // Keep last N boot sessions
#define POLL_INTERVAL   20       // Input polling frequency (ms)
#define BEEP_DURATION   500      // Beep length (ms)
#define MONITORING_DURATION 5000 // Anti-cheat monitoring time (ms)
#define SPIFFS_VERSION  1        // Increment for file structure changes
```

---

## 📝 System Features Summary

| Feature | Details |
|---------|---------|
| **Multi-Slot Detection** | 3 slots, each with dual sensors (A & B) |
| **Anti-Cheat Monitoring** | 5-second surveillance after A→B sequence |
| **Boot Tracking** | Counts power cycles and stores per-boot data |
| **Lifetime Tracking** | Never-reset total reject counter |
| **Automatic Cleanup** | Batch deletes old session files every 100 boots |
| **Data Persistence** | SPIFFS-based file storage survives power loss |
| **Web Interface** | WiFi AP mode with file browser and downloader |
| **Serial Debugging** | Full command interface for monitoring |
| **LED/Buzzer Alerts** | Visual and audio feedback for reject mode |
| **Relay Control** | Machine start/stop via digital relay output |
| **I2C Expansion** | 8 inputs + 8 outputs via PCF8574 modules |

---

## 🐛 Troubleshooting

| Issue                       | Solution                                  |
|-----------------------------|-------------------------------------------|
| "SPIFFS Mount failed"       | Format SPIFFS from Arduino IDE Tools menu |
| Files not persisting        | Check SPIFFS partition in board settings  |
| I2C errors                  | Verify PCF8574 addresses (0x25, 0x26)     |
| Counters reset unexpectedly | Check file system integrity               |
| Sensors not detecting       | Verify PCF1 wiring and sensor power       |
| Web dashboard not accessible| Check WiFi AP settings (SSID/Password)   |

---

## 📈 Performance Metrics

| Metric             | Value                 |
|--------------------|-----------------------|
| Sensor poll rate   | 50 Hz (20ms interval) |
| Edge detection     | Rising edge only      |
| Beep duration      | 500ms                 |
| Buzzer blink rate  | 1 Hz (500ms on/off)   |
| File write time    | < 10ms                |
| Boot time          | ~2 seconds            |
| I2C Speed          | 100 kHz               |

---

## 🔒 Safety Features

✅ **Machine interlock** - Cannot start until rejected part confirmed  
✅ **State persistence** - System remembers reject mode after power loss  
✅ **Dual-sensor confirmation** - Prevents false positives  
✅ **Anti-cheat monitoring** - Detects part removal attempts  
✅ **Visual/audio alerts** - Operator awareness  
✅ **Data redundancy** - Multiple file backups  
✅ **Automatic cleanup** - Prevents filesystem overflow  

---

## 📄 License

This project is proprietary software. All rights reserved.

---

## 👨‍💻 Development Notes

- **Code Organization:** Modular header-based design for easy maintenance
- **I2C Clock Speed:** 100 kHz for stable communication
- **Edge Detection:** Rising edge detection for button/sensor inputs
- **Non-blocking Beep:** Beep timing managed in `update_outputs()` loop
- **SPIFFS Reliability:** Auto-format on first boot, version checking for updates

---

## ⚠️ Disclaimer

This system is designed as a safety interlock and should not be the sole safety mechanism. Always follow proper industrial safety protocols and implement redundant safety systems for critical applications.
