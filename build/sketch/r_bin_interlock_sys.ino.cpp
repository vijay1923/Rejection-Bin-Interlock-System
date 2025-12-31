#include <Arduino.h>
#line 1 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
#include <Wire.h>
#include "config.h"
#include "file_operations.h"
#include "io_operations.h"
#include "process.h"
#include "reject.h"
#include "serial_cmd.h"
#include "web_server.h"

#line 10 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void setup();
#line 57 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void loop();
#line 10 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\r_bin_interlock_sys.ino"
void setup() 
{
    Serial.begin(115200);
    delay(2000);   
    Serial.println("WELCOME ESP32 : REJECTION BIN INTERLOCKING SYSTEM");
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

    initWebServer();   
    init_filesystem();
    init_boot_number();
    init_total_count();
    
    read_state();
    Serial.println("-------------SYSTEM STATUS -------------");
    Serial.printf("Boot Number:          %lu\n", current_boot_number);
    Serial.printf("Session Count:        %lu\n", current_session_count);
    Serial.printf("Lifetime rejection:   %lu\n", total_lifetime_count);
    Serial.printf("Machine Mode:         %s\n", state.machine_mode ? "REJECT" : "AUTO");
     Serial.println("----------------------------------------");
    machine_status = false;
    relay_output(false);
    
    if (state.machine_mode) 
    {
        Serial.println("SYSTEM IN REJECT MODE");
        Serial.println("Waiting for part confirmation in bin...");
        Serial.println("Buzzer/LED Alert ACTIVE");
    } 
    else 
    {
        Serial.println("SYSTEM READY");
        Serial.println("Press AUTO button to start machine");
    }
    prev_inputs = read_inputs();
}

void loop() 
{
   handleSerialCommands();   // serial command handler  
   server.handleClient();     // web server handler 
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
    update_outputs();
}
