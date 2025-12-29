#include <Arduino.h>
#line 1 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
#include <Wire.h>
#include "config.h"
#include "file_operations.h"
#include "io_operations.h"
#include "process.h"
#include "reject.h"


#line 9 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void setup();
#line 59 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void loop();
#line 9 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
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
    
    // Initialize filesystem
    init_filesystem();
    read_state();   // get stored data from memory 
    
    // Print last machine data 
    Serial.println("STATE Restored from memory:-");
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
