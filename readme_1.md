

# 🚀 PHASE 1: SYSTEM STARTUP (setup()) 

POWER ON
    ↓
1. Serial communication starts (115200 baud)
    ↓
2. I2C initialization (GPIO 21=SDA, 22=SCL, 100kHz)
    ↓
3. PCF1 configured as INPUT (0xFF = all pullups enabled)
    ↓
4. PCF2 configured as OUTPUT (0x00 = all relays OFF)
    ↓
5. Web server starts (WiFi AP: "RejectionBin_AP")
    ↓
6. SPIFFS file system mounts
    ├─ Check integrity (detect corruption)
    ├─ Check version (auto-format if firmware updated)
    └─ Display storage info
    ↓
7. Load boot number from /boot_number.txt
    ├─ If file doesn't exist → boot_number = 1
    ├─ If file exists → boot_number = previous + 1
    ├─ Save new boot number
    └─ Create new session file: /start_X.txt (X = boot number)
    ↓
8. Load total lifetime count from /total_count.txt
    ↓
9. Load machine state from /state.txt
    ├─ If machine_mode = true → System was in REJECT mode
    └─ If machine_mode = false → System was in AUTO mode
    ↓
10. Display system status on Serial Monitor
    ├─ Boot Number
    ├─ Session Count
    ├─ Lifetime Count
    └─ Machine Mode
    ↓
11. Turn OFF all relays (safety)
    ↓
12. Read initial input state (for edge detection)
    ↓
READY!

---
# 🔁 PHASE 2: MAIN LOOP (loop())

┌─────────────────────────────────────────┐
│ START OF LOOP                           │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│ handleSerialCommands()                  │
│ - Check if serial data available        │
│ - Process: LIST, READ, HELP commands    │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│ server.handleClient()                   │
│ - Process web server HTTP requests      │
│ - Routes: /, /sessions, /view, etc.     │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│ Check if 20ms has passed?               │
│ (POLL_INTERVAL timer)                   │
└───────────────┬─────────────────────────┘
                │
           YES  │  NO → Skip to update_outputs()
                ▼
┌─────────────────────────────────────────┐
│ read_inputs()                           │
│ - Read all 8 inputs from PCF1           │
│ - Invert bits (active LOW → HIGH)       │
│ - current_inputs = sensor/button states │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│ check_monitoring(current_inputs)        │
│ - If monitoring active:                 │
│   ├─ Check if 5 seconds passed → Save count
│   └─ Check if Sensor A triggered → Cheating!
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│ Edge Detection                          │
│ edge = current_inputs & ~prev_inputs    │
│ - Finds which inputs went LOW → HIGH    │
└───────────────┬─────────────────────────┘
                │
           edge │ detected?
                ▼
┌─────────────────────────────────────────┐
│ process_inputs(edge)                    │
│ - Check if AUTO button pressed          │
│ - Check if REJECT button pressed        │
│ - Call reject_handler(edge)             │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│ prev_inputs = current_inputs            │
│ - Store for next edge detection         │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│ update_outputs()                        │
│ - Control Relay (machine ON/OFF)        │
│ - Control Green LED (machine indicator) │
│ - Handle 500ms beeps (non-blocking)     │
└───────────────┬─────────────────────────┘
                │
                ▼
           LOOP REPEATS

---

# 🎮 PHASE 3: OPERATOR ACTIONS

Operator presses AUTO button
    ↓
Edge detected: BTN_AUTO bit is HIGH
    ↓
process_inputs(edge) called
    ├─ Detects AUTO button
    └─ Calls auto_button_handler()
        ↓
    Check: Is machine in REJECT mode?
        ├─ YES → Print "AUTO ignored" → EXIT ❌
        └─ NO → Continue ✅
            ↓
        state.machine_mode = false
        active_slot = 0
            ↓
        write_state() → Save to /state.txt
            ↓
        relay_output(true)
            ├─ machine_status = true
            └─ update_outputs()
                ├─ RELAY_AUTO = ON
                └─ LED_MACHINE_ON (Green) = ON
            ↓
        Serial: "AUTO MODE - MACHINE RUNNING"
            ↓
    ✅ MACHINE IS NOW RUNNING

---

# ACTION 2: REJECT BUTTON PRESSED

Operator presses REJECT button
    ↓
Edge detected: BTN_REJECT bit is HIGH
    ↓
process_inputs(edge) called
    ├─ Detects REJECT button
    └─ Calls reject_button_handler()
        ↓
    Check: Is machine currently running?
        ├─ NO → Print "REJECT ignored" → EXIT ❌
        └─ YES → Continue ✅
            ↓
        state.machine_mode = true
        active_slot = 0
            ↓
        write_state() → Save to /state.txt
            ↓
        relay_output(false)
            ├─ machine_status = false
            └─ update_outputs()
                ├─ RELAY_AUTO = OFF
                └─ LED_MACHINE_ON (Green) = OFF
            ↓
        trigger_beep() 
            ├─ beep_active = true
            ├─ beep_start_time = millis()
            └─ Next update_outputs() will turn buzzer ON for 500ms
            ↓
        Serial: "REJECT MODE - MACHINE STOPPED"
            ↓
    ✅ SYSTEM IN REJECT MODE, WAITING FOR PART

---

#  ACTION 3: PART PLACEMENT - SENSOR A TRIGGERS

Operator places part in Slot 1
    ↓
Sensor 1A triggers (part enters)
    ↓
Edge detected: SLOT1_A bit is HIGH
    ↓
process_inputs(edge) called
    └─ Calls reject_handler(edge)
        ↓
    Check: Is machine in REJECT mode?
        ├─ NO → EXIT (ignore sensors) ❌
        └─ YES → Continue ✅
            ↓
    Check: Is active_slot == 0?
        ├─ NO → Check for Sensor B (different flow)
        └─ YES → Continue ✅
            ↓
        Detect which sensor A triggered:
            ├─ SLOT1_A? → active_slot = 1
            ├─ SLOT2_A? → active_slot = 2
            └─ SLOT3_A? → active_slot = 3
            ↓
        trigger_beep()
            └─ BEEP for 500ms (Red LED + Buzzer)
            ↓
        Serial: "[SENSOR] Slot 1A detected - waiting for 1B..."
            ↓
        return → Wait for Sensor B
            ↓
    ✅ SYSTEM WAITING FOR SENSOR B

---

# ACTION 4: PART PLACEMENT - SENSOR B TRIGGERS

Part continues through bin
    ↓
Sensor 1B triggers (part exits)
    ↓
Edge detected: SLOT1_B bit is HIGH
    ↓
process_inputs(edge) called
    └─ Calls reject_handler(edge)
        ↓
    Check: Is machine in REJECT mode?
        └─ YES → Continue ✅
            ↓
    Check: Is active_slot == 0?
        └─ NO (active_slot = 1) → Continue to validation ✅
            ↓
    Validate Sensor B:
        ├─ Is active_slot == 1 AND SLOT1_B triggered? → valid_sequence = true ✅
        ├─ Is active_slot == 2 AND SLOT2_B triggered? → valid_sequence = true ✅
        └─ Is active_slot == 3 AND SLOT3_B triggered? → valid_sequence = true ✅
            ↓
    If valid_sequence == true:
        ↓
        trigger_beep()
            └─ BEEP for 500ms
            ↓
        Serial: "Slot-1: B detected, Sequence complete"
            ↓
        state.machine_mode = false (exit REJECT mode)
        slot = active_slot (remember which slot)
        active_slot = 0 (reset)
            ↓
        write_state() → Save to /state.txt
            ↓
        start_monitoring(slot)
            ├─ monitoring_active = true
            ├─ monitoring_start_time = millis()
            ├─ monitored_slot = 1
            └─ Serial: "[MONITORING] Started for Slot 1..."
            ↓
    ✅ MONITORING ACTIVE FOR 5 SECONDS

---

#  ACTION 4: PART PLACEMENT - SENSOR B TRIGGERS

Part continues through bin
    ↓
Sensor 1B triggers (part exits)
    ↓
Edge detected: SLOT1_B bit is HIGH
    ↓
process_inputs(edge) called
    └─ Calls reject_handler(edge)
        ↓
    Check: Is machine in REJECT mode?
        └─ YES → Continue ✅
            ↓
    Check: Is active_slot == 0?
        └─ NO (active_slot = 1) → Continue to validation ✅
            ↓
    Validate Sensor B:
        ├─ Is active_slot == 1 AND SLOT1_B triggered? → valid_sequence = true ✅
        ├─ Is active_slot == 2 AND SLOT2_B triggered? → valid_sequence = true ✅
        └─ Is active_slot == 3 AND SLOT3_B triggered? → valid_sequence = true ✅
            ↓
    If valid_sequence == true:
        ↓
        trigger_beep()
            └─ BEEP for 500ms
            ↓
        Serial: "Slot-1: B detected, Sequence complete"
            ↓
        state.machine_mode = false (exit REJECT mode)
        slot = active_slot (remember which slot)
        active_slot = 0 (reset)
            ↓
        write_state() → Save to /state.txt
            ↓
        start_monitoring(slot)
            ├─ monitoring_active = true
            ├─ monitoring_start_time = millis()
            ├─ monitored_slot = 1
            └─ Serial: "[MONITORING] Started for Slot 1..."
            ↓
    ✅ MONITORING ACTIVE FOR 5 SECONDS


---

# ⏱️ PHASE 4: MONITORING PERIOD (5 seconds)


┌────────────────────────────────────────┐
│ check_monitoring(current_inputs)      │
└───────────────┬────────────────────────┘
                │
    Is monitoring_active?
                │
        NO → return (exit)
        YES ↓
                │
    Calculate elapsed time:
    elapsed = millis() - monitoring_start_time
                │
                ▼
        ┌───────┴───────┐
        │               │
    elapsed ≥ 5000?   elapsed < 5000?
        │               │
        YES             NO
        │               │
        ▼               ▼
    ┌────────────┐  ┌──────────────┐
    │ MONITORING │  │ CHECK FOR    │
    │ COMPLETE   │  │ SENSOR A     │
    └────────────┘  └──────────────┘
        │               │
        │               ▼
        │           Is Sensor A HIGH?
        │               │
        │           ┌───┴───┐
        │           │       │
        │          YES      NO
        │           │       │
        │           ▼       return
        │       ┌─────────────┐
        │       │ CHEATING!   │
        │       │ - Reset     │
        │       │ - No count  │
        │       │ - Stay in   │
        │       │   REJECT    │
        │       └─────────────┘
        │
        ▼
┌─────────────────────────────────────┐
│ PART CONFIRMED                      │
├─────────────────────────────────────┤
│ save_reject_count()                 │
│ ├─ current_session_count++          │
│ ├─ total_lifetime_count++           │
│ ├─ Save to /start_X.txt             │
│ └─ Save to /total_count.txt         │
├─────────────────────────────────────┤
│ monitoring_active = false           │
│ monitored_slot = 0                  │
├─────────────────────────────────────┤
│ Serial: "Part Confirmed in Slot 1"  │
│ Serial: "Session Count: 6"          │
│ Serial: "Lifetime Count: 1234"      │
└─────────────────────────────────────┘
        │
        ▼
    ✅ COUNT SAVED, READY FOR NEXT CYCLE

---

# 🚨 SCENARIO: CHEATING DETECTED

During monitoring (0-5 seconds after A→B)
    ↓
Operator pulls part back out
    ↓
Sensor A triggers (part moving backward)
    ↓
check_monitoring(current_inputs) detects it
    ↓
    current_inputs & (1 << SLOT1_A) is TRUE
    ↓
Serial: "[CHEATING DETECTED!] Sensor A triggered..."
Serial: "Part being removed - Count NOT incremented!"
    ↓
monitoring_active = false
active_slot = 0
monitored_slot = 0
    ↓
state.machine_mode remains TRUE (stay in REJECT)
    ↓
Serial: "Please place part properly in the bin"
Serial: "Session Count: 5 (unchanged)"
Serial: "Lifetime Count: 1233 (unchanged)"
    ↓
❌ NO COUNT INCREMENT, SYSTEM RESETS TO WAIT FOR PROPER PLACEMENT

---

# 📊 STATE MACHINE DIAGRAM

┌──────────────┐
│   IDLE       │ ◄──────────────┐
│ (Power ON)   │                │
└──────┬───────┘                │
       │                        │
   AUTO button                  │
       │                        │
       ▼                        │
┌──────────────┐                │
│   RUNNING    │                │
│ (AUTO mode)  │                │
│ Green LED ON │                │
└──────┬───────┘                │
       │                        │
  REJECT button                 │
  + BEEP 500ms                  │
       │                        │
       ▼                        │
┌──────────────┐                │
│ REJECT MODE  │                │
│ Waiting for  │                │
│ part in bin  │                │
└──────┬───────┘                │
       │                        │
   Sensor A                     │
   + BEEP 500ms                 │
       │                        │
       ▼                        │
┌──────────────┐                │
│ WAITING B    │                │
│ active_slot  │                │
│ = 1/2/3      │                │
└──────┬───────┘                │
       │                        │
   Sensor B                     │
   + BEEP 500ms                 │
       │                        │
       ▼                        │
┌──────────────┐                │
│ MONITORING   │                │
│ (5 seconds)  │                │
│ Watch for A  │                │
└──────┬───────┘                │
       │                        │
   ┌───┴────┐                   │
   │        │                   │
Sensor A   Timer                │
triggers   expires              │
   │        │                   │
   ▼        ▼                   │
CHEAT   CONFIRMED               │
   │        │                   │
   │        ├─ Save count       │
   │        └─ Back to IDLE ────┘
   │
   └─ Reset, stay in REJECT

---