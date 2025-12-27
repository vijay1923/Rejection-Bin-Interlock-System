#include <Arduino.h>
#line 1 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
#include <Wire.h>
#include <EEPROM.h>

#define PCF1_ADDR       0x25    // Input PCF8574 buttons & sensors
#define PCF2_ADDR       0x26    // Output PCF8574 relays & leds & buzers
#define POLL_INTERVAL   20      // Polling interval in milliseconds
#define BUZZER_INTERVAL 500     // Buzzer blink/beep interval in milliseconds

// Input Pin Mapping PCF1
#define BTN_AUTO        0       // AUTO button
#define BTN_REJECT      1       // REJECT/E-STOP button
#define SLOT1_A         2       // Slot 1 Sensor A (front)
#define SLOT1_B         3       // Slot 1 Sensor B (back)
#define SLOT2_A         4       // Slot 2 Sensor A (front)
#define SLOT2_B         5       // Slot 2 Sensor B (back)
#define SLOT3_A         6       // Slot 3 Sensor A (front)
#define SLOT3_B         7       // Slot 3 Sensor B (back)

// Output Pin Mapping PCF2
#define RELAY_AUTO      0       // AUTO relay to PLC
#define LED_REJECT      1       // REJECT mode indicator
#define OUT_PULSE       2       // Confirmation pulse output
#define BUZZER_LED      7       // Buzzer/LED for reject alert (blinks & beeps)

#define EEPROM_SIZE     512     // allocate memory in flash 
#define EEPROM_ADDR     0       // starting memory address

struct MachineState    /// machine structer 
{
    uint32_t reject_count;      // Total rejected parts
    bool machine_mode;           // true if waiting for part in bin

};

uint8_t prev_inputs = 0x00;     // Previous normalized input state
uint8_t active_slot = 0;        // Local slot tracking not saved
MachineState state;             // Current machine state
bool machine_status = false;   // true when AUTO relay is ON
uint32_t last_poll_time = 0;    // For polling timing
uint32_t last_buzzer_toggle = 0; // For buzzer blink timing
bool buzzer_state = false;      // Current buzzer on/off state


#line 44 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void save_state();
#line 51 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void load_state();
#line 70 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
uint8_t read_inputs();
#line 81 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void write_outputs(uint8_t data);
#line 88 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void update_outputs();
#line 126 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void relay_output(bool on);
#line 132 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void pulse_output(uint8_t pin, uint16_t duration_ms);
#line 151 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void auto_button_handler();
#line 166 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void reject_button_handler();
#line 183 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void reject_handler(uint8_t edge);
#line 299 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void process_inputs(uint8_t edge);
#line 319 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void setup();
#line 369 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void loop();
#line 44 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void save_state() 
{
    EEPROM.put(EEPROM_ADDR, state);
    EEPROM.commit();
    Serial.println("[EEPROM] State saved");
}

void load_state() 
{
    EEPROM.get(EEPROM_ADDR, state);  
    
    // Validate rejection count 
    if (state.reject_count > 1000000) 
    {
        // Invalid data - use defaults
        state.reject_count = 0;
        state.machine_mode = false;
        Serial.println("[EEPROM] Invalid data - using defaults");
        save_state();
    } 
    else 
    {
        Serial.println("[EEPROM] State restored");
    }
}

uint8_t read_inputs() 
{
    Wire.requestFrom(PCF1_ADDR, 1);
    if (Wire.available()) 
    {
        uint8_t raw = Wire.read();
        return ~raw;  // Invert (active LOW → active HIGH)
    }
    return 0x00;
}

void write_outputs(uint8_t data) 
{
    Wire.beginTransmission(PCF2_ADDR);
    Wire.write(data);
    Wire.endTransmission();
}

void update_outputs()
{
    uint8_t out = 0x00;
    
    // Set AUTO relay
    if (machine_status) 
    {
        out |= (1 << RELAY_AUTO);
    }
    
    // Set REJECT LED
    if (state.machine_mode) 
    {
        out |= (1 << LED_REJECT);
    }
    
    // Blink buzzer/LED in reject mode
    if (state.machine_mode) 
    {
        if (millis() - last_buzzer_toggle >= BUZZER_INTERVAL)
        {
            last_buzzer_toggle = millis();
            buzzer_state = !buzzer_state;
        }
        
        if (buzzer_state)
        {
            out |= (1 << BUZZER_LED);
        }
    }
    else
    {
        buzzer_state = false;  // Turn off when not in reject mode
    }
    
    write_outputs(out);
}

void relay_output(bool on) 
{
    machine_status = on;
    update_outputs();
}

void pulse_output(uint8_t pin, uint16_t duration_ms) 
{
    uint8_t out = 0x00;
    
    if (machine_status) out |= (1 << RELAY_AUTO);
    if (state.machine_mode) 
    {
        out |= (1 << LED_REJECT);
        if (buzzer_state) out |= (1 << BUZZER_LED);
    }
    
    out |= (1 << pin);
    write_outputs(out);
    delay(duration_ms);
    
    out &= ~(1 << pin);
    write_outputs(out);
}

void auto_button_handler() 
{
    if (state.machine_mode) 
    {
        Serial.println("[BUTTON] AUTO ignored - still in REJECT mode");
        return;
    }
    
    Serial.println("AUTO MODE - MACHINE RUNNING");    
    state.machine_mode = false;
    active_slot = 0;
    save_state();
    relay_output(true);
}

void reject_button_handler() 
{
    if (!machine_status) 
    {
        Serial.println("[BUTTON] REJECT ignored - machine not running");
        return;
    }
    
    Serial.println("REJECT MODE - MACHINE STOPPED");
    Serial.println("Waiting for part in rejection bin...");
    
    state.machine_mode = true;
    active_slot = 0;
    save_state();
    relay_output(false);
}

void reject_handler(uint8_t edge) 
{
    // Only process sensor inputs in reject mode
    if (!state.machine_mode) return;
    
    if (active_slot == 0) 
    {
        if (edge & (1 << SLOT1_A)) 
        {
            active_slot = 1;
            Serial.println("[SENSOR] Slot 1A detected - waiting for 1B...");
        }
        else if (edge & (1 << SLOT2_A)) 
        {
            active_slot = 2;
            Serial.println("[SENSOR] Slot 2A detected - waiting for 2B...");
        }
        else if (edge & (1 << SLOT3_A)) 
        {
            active_slot = 3;
            Serial.println("[SENSOR] Slot 3A detected - waiting for 3B...");
        }
        return;
    }
    
    bool valid_sequence = false;
    
    if (active_slot == 1 && (edge & (1 << SLOT1_B))) 
    {
        valid_sequence = true;
    }
    else if (active_slot == 2 && (edge & (1 << SLOT2_B))) 
    {
        valid_sequence = true;
    }
    else if (active_slot == 3 && (edge & (1 << SLOT3_B))) 
    {
        valid_sequence = true;
    }
    
    if (valid_sequence) 
    {
        state.reject_count++;
        state.machine_mode = false;
        uint8_t slot = active_slot;
        active_slot = 0;
        save_state();
        
        // Confirmation pulse
        pulse_output(OUT_PULSE, 200);
        
        Serial.printf("Rejection slot no : %d\n", slot);
        Serial.printf("Total Reject Count: %lu\n", state.reject_count);
        Serial.println("Ready for next cycle");
    }
}

/*
void handle_part_removal(uint8_t edge) 
{
    // Only detect removal when NOT in reject mode
    if (state.machine_mode || active_slot != 0) return;
    
    // Detect Sensor B triggered first (part exiting bin backwards)
    if (active_slot == 0) 
    {
        if (edge & (1 << SLOT1_B)) 
        {
            active_slot = -1;  // Negative flag for removal
            Serial.println("[SENSOR] Slot 1B detected - checking for removal...");
        }
        else if (edge & (1 << SLOT2_B)) 
        {
            active_slot = -2;
            Serial.println("[SENSOR] Slot 2B detected - checking for removal...");
        }
        else if (edge & (1 << SLOT3_B)) 
        {
            active_slot = -3;
            Serial.println("[SENSOR] Slot 3B detected - checking for removal...");
        }
        return;
    }
    
    // Confirm with Sensor A (part fully removed)
    bool valid_removal = false;
    
    if (active_slot == -1 && (edge & (1 << SLOT1_A))) 
    {
        valid_removal = true;
    }
    else if (active_slot == -2 && (edge & (1 << SLOT2_A))) 
    {
        valid_removal = true;
    }
    else if (active_slot == -3 && (edge & (1 << SLOT3_A))) 
    {
        valid_removal = true;
    }
    
    if (valid_removal) 
    {
        if (state.reject_count > 0) 
        {
            state.reject_count--;
            int8_t slot = -active_slot;
            active_slot = 0;
            save_state();
            Serial.printf("↑ PART REMOVED - Slot %d\n", slot);
            Serial.printf("↑ Total Reject Count: %lu\n", state.reject_count);
        }
    }
}
*/


void process_inputs(uint8_t edge) 
{
    // Priority 1: Button handling
    if (edge & (1 << BTN_AUTO)) 
    {
        auto_button_handler();
    }
    
    if (edge & (1 << BTN_REJECT)) 
    {
        reject_button_handler();
    }
    
    // Priority 2: Slot detection (only in reject mode)
    reject_handler(edge);
    
    // Priority 3: Part removal 
    // handle_part_removal(edge);
}

void setup() 
{
    Serial.begin(115200);
       
    // Initialize I2C
    Wire.begin(21, 22);
    Wire.setClock(100000);
    
    // Configure PCF1 (inputs with pullups)
    Wire.beginTransmission(PCF1_ADDR);
    Wire.write(0xFF);
    Wire.endTransmission();
    
    // Configure PCF2 (outputs LOW - all relays OFF)
    Wire.beginTransmission(PCF2_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();
    
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    load_state();   // get stored data from memory 
    
    // Print last machine data 
    Serial.println("[STATE] Restored from memory:");
    Serial.printf("Reject Count: %lu\n", state.reject_count);
    Serial.printf("Reject Mode: %s\n", state.machine_mode ? "YES - waiting for part in Bin " : "NO");
    
    // Safety: Always start with relay OFF
    machine_status = false;   // default machine off 
    relay_output(false);     // off relay output 
     
    Serial.println("WELCOME ESP32 : REJECTION BIN INTERLOCKING SYSTEM  ");
    
    if (state.machine_mode) // if machine was in reject mode before restart / power off 
    {
        Serial.println("SYSTEM IN REJECT MODE");
        Serial.println("Waiting for part confirmation...");
        Serial.println("Buzzer/LED Alert ACTIVE");
    } 
    else 
    {
        Serial.println("System Ready");
        Serial.println("Press AUTO button to start machine");
    }
    
    // Read initial input state
    prev_inputs = read_inputs();
}


void loop() 
{
    // poll pcf  
    if (millis() - last_poll_time >= POLL_INTERVAL) 
    {
        last_poll_time = millis();
        
        // Read current inputs
        uint8_t current_inputs = read_inputs();
        
        // Detect rising edges (button press / sensor trigger)
        uint8_t edge = current_inputs & ~prev_inputs;
        
        if (edge) 
        {
            process_inputs(edge);
        }
        
        // Update previous state
        prev_inputs = current_inputs;
    }
    
    // Continuously update outputs (for buzzer blinking)
    update_outputs();
}
