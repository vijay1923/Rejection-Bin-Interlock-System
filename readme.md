## Title ##
🏭 Rejection Bin Interlocking System :- ESP32-Based Industrial Part Rejection Control

## 📖 Overview ##

This project implements an industrial rejection bin interlocking system using an ESP32 microcontroller. It ensures safety in manufacturing by stopping the machine when a defective part is detected and preventing restart until the rejected part is confirmed in the bin. The system features three detection slots with dual sensors, state persistence across power cycles, and visual/audio alerts for operator awareness.

## 📚 Libraries Used

| Library Name | Purpose                                          |
|--------------|--------------------------------------------------|
| `Wire.h`     | I2C communication with PCF8574 expanders         |
| `SPIFFS.h`   | File system for state persistence in flash       |
| `FS.h`       | Base file system operations                      |
| `FFat.h`     | FAT file system support                          |
| `WString.h`  | Arduino string operations                        |

## 🧰 Hardware Components

| Component              | Quantity | Description                                      |
|------------------------|----------|--------------------------------------------------|
| ESP32 DevKit           | 1        | Main microcontroller                             |
| PCF8574 I/O Expander   | 2        | I2C input/output expansion (0x25, 0x26)          |
| Push Button (AUTO)     | 1        | Start machine operation                          |
| Push Button (REJECT)   | 1        | Stop machine and enter reject mode               |
| Proximity Sensor       | 6        | Part detection (2 sensors per slot: A & B)       |
| Relay Module           | 1        | Controls machine AUTO signal to PLC              |
| LED (REJECT Indicator) | 1        | Shows when system is in reject mode              |
| LED (Alert)            | 1        | Blinks during reject mode                        |
| Buzzer                 | 1        | Audio alert during reject mode                   |

## 🛠️ Software Requirements ##

| Software/Tool          | Description                                                        |
|------------------------|--------------------------------------------------------------------|
| Arduino Maker Workshop | Development environment for ESP32 programming                      |
| ESP32 Board            | ESP32 board support package in Arduino IDE                         |
| Serial Monitor         | Debug and monitor system events (115200 baud)                      |

## 📁 File Structure & Description ##

| File                  | Purpose                                                    |
|-----------------------|------------------------------------------------------------|
| `main.ino`            | Main program loop, initialization, input polling           |
| `config.h`            | Pin definitions, I2C addresses, global variables           |
| `file_operations.h`   | SPIFFS initialization, state save/restore functions        |
| `io_operations.h`     | PCF8574 read/write operations, output control              |
| `process.h`           | Button event handlers (AUTO, REJECT)                       |
| `reject.h`            | Slot detection logic and rejection sequence handling       |

## 🔌 Pin Mapping ##

**PCF1 (0x25) - Input Expander**

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

**PCF2 (0x26) - Output Expander**

| Pin | Function       | Description                      |
|-----|----------------|----------------------------------|
| 0   | RELAY_AUTO     | AUTO relay output to PLC         |
| 1   | LED_REJECT     | REJECT mode indicator LED        |
| 2   | OUT_PULSE      | Confirmation pulse output        |
| 7   | BUZZER_LED     | Buzzer/Alert LED (blinks)        |

**ESP32 I2C Pins**

| Pin   | Function | Description          |
|-------|----------|----------------------|
| GPIO21| SDA      | I2C data line        |
| GPIO22| SCL      | I2C clock line       |

## ⚙️ System Operation ##

**AUTO Mode (Normal Operation):**
- Machine running, AUTO relay energized
- System monitors for REJECT button press
- All alerts OFF

**REJECT Mode (Waiting for Part):**
- Machine stopped, AUTO relay de-energized
- REJECT LED ON, Buzzer/LED blinking (500ms interval)
- System waiting for part confirmation in any slot
- AUTO button ignored until part confirmed

**Part Detection Sequence:**
1. Sensor A triggers (part entering slot)
2. System locks to that specific slot
3. Sensor B triggers (part fully seated)
4. On valid A→B sequence:
   - Reject count incremented
   - State saved to SPIFFS
   - Confirmation pulse sent (200ms)
   - System exits REJECT mode
   - Ready for AUTO restart

## 🖨️ Serial Output Format ##

Output Includes:
* System initialization and state restoration
* Button press events
* Sensor trigger notifications
* Slot detection progress
* Rejection count updates
* Mode transitions

```
-> WELCOME ESP32 : REJECTION BIN INTERLOCKING SYSTEM
-> Mounted successfully
-> Total: 1.50 MB, Used: 0.01 MB, Free: 1.49 MB
-> State restored
-> STATE Restored from memory:-
-> Reject Count: 42
-> Reject Mode: NO
-> System Ready
-> Press AUTO button to start machine

-> AUTO MODE - MACHINE RUNNING

-> REJECT MODE - MACHINE STOPPED
-> Waiting for part in rejection bin...
-> Buzzer/LED Alert ACTIVE

-> [SENSOR] Slot 2A detected - waiting for 2B...
-> Rejection slot no : 2
-> Total Reject Count: 43
-> State saved
-> Ready for next cycle

-> AUTO MODE - MACHINE RUNNING

```

## 👨‍🔧 Author

**Vijay Magadum**  
Embedsol Technologies LLP