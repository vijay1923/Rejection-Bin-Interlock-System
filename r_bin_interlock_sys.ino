// Include necessary libraries and custom header files
#include <Wire.h>                    // I2C communication library for PCF8574 modules
#include "config.h"                  // Configuration settings and constants
#include "file_operations.h"         // File system operations for data persistence
#include "io_operations.h"           // Input/Output operations for PCF8574
#include "process.h"                 // Main processing logic for rejection system
#include "reject.h"                  // Rejection handling logic
#include "serial_cmd.h"              // Serial command processing
#include "web_server.h"              // Web server for remote monitoring

void setup() 
{
    // Initialize serial communication at 115200 baud rate
    Serial.begin(115200);
    delay(2000);   // Wait for serial port to stabilize
    Serial.println("WELCOME ESP32 : REJECTION BIN INTERLOCKING SYSTEM");
    
    // Initialize I2C communication with SDA on GPIO21 and SCL on GPIO22
    Wire.begin(21, 22);
    Wire.setClock(100000);  // Set I2C clock to 100kHz for stable communication
    
    // Configure PCF1 (first PCF8574 module) as inputs with internal pullups enabled
    // Writing 0xFF sets all pins HIGH, enabling pullup resistors for input mode
    Wire.beginTransmission(PCF1_ADDR);
    Wire.write(0xFF);
    Wire.endTransmission();
    
    // Configure PCF2 (second PCF8574 module) as outputs, all set LOW initially
    // Writing 0x00 turns all relays OFF at startup for safety
    Wire.beginTransmission(PCF2_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();

    // Initialize system components
    initWebServer();          // Start the web server for remote access
    init_filesystem();        // Mount file system (SPIFFS/LittleFS)
    init_boot_number();       // Load/increment boot counter
    init_total_count();       // Load lifetime rejection count from storage
    
    // Load saved system state from file system
    read_state();
    
    // Display current system status on serial monitor
    Serial.println("-------------SYSTEM STATUS -------------");
    Serial.printf("Boot Number:          %lu\n", current_boot_number);
    Serial.printf("Session Count:        %lu\n", current_session_count);
    Serial.printf("Lifetime rejection:   %lu\n", total_lifetime_count);
    Serial.printf("Machine Mode:         %s\n", state.machine_mode ? "REJECT" : "AUTO");
    Serial.println("----------------------------------------");
    
    // Initialize machine status and ensure all relays are off
    machine_status = false;
    relay_output(false);  // Safety: Turn off all relay outputs
    
    // Check machine mode and display appropriate startup message
    if (state.machine_mode) 
    {
        // System is in REJECT mode - waiting for operator intervention
        Serial.println("SYSTEM IN REJECT MODE");
        Serial.println("Waiting for part confirmation in bin...");
        Serial.println("Buzzer/LED Alert ACTIVE");
    } 
    else 
    {
        // System is in AUTO mode - ready for normal operation
        Serial.println("SYSTEM READY");
        Serial.println("Press AUTO button to start machine");
    }
    
    // Store initial input state to detect changes (edge detection)
    prev_inputs = read_inputs();
}

void loop() 
{
    // Handle incoming serial commands for debugging/configuration
    handleSerialCommands();
    
    // Process web server requests for remote monitoring/control
    server.handleClient();

    // Poll inputs at regular intervals to avoid excessive I2C traffic
    if (millis() - last_poll_time >= POLL_INTERVAL) 
    {
        last_poll_time = millis();  // Update last poll timestamp
        
        // Read current state of all input pins from PCF1
        uint8_t current_inputs = read_inputs();
        
        // Detect rising edge transitions (button press / sensor trigger)
        // Edge = current HIGH AND previous LOW (bitwise: current & NOT previous)
        uint8_t edge = current_inputs & ~prev_inputs;
        
        // If any input has transitioned from LOW to HIGH, process it
        if (edge) 
        {
            process_inputs(edge);  // Handle the detected input event
        }
        
        // Update previous input state for next iteration's edge detection
        prev_inputs = current_inputs;
    }
    
    // Update output states (relays, buzzer, LEDs) based on current system state
    update_outputs();
}