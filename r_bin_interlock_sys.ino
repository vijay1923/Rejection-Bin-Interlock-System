// Include necessary libraries and custom header files
#include <Wire.h>                    // I2C communication library for PCF8574 modules
#include "config.h"                  // Configuration settings and constants
#include "eeprom_operations.h"       // EEPROM operations for critical data
#include "file_operations.h"         // SPIFFS operations for session logs
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
    Serial.println("WELCOME ESP32 : REJECTION BIN INTERLOCKING SYSTEM ");
    
    // Initialize I2C communication with SDA on GPIO21 and SCL on GPIO22
    Wire.begin(21, 22);
    Wire.setClock(100000);  // Set I2C clock to 100kHz for stable communication
    
    // Configure PCF1 (first PCF8574 module) as inputs with internal pullups enabled
    Wire.beginTransmission(PCF1_ADDR);
    Wire.write(0xFF);  // set all pins HIGH for input with pullups
    Wire.endTransmission();
    
    // Configure PCF2 (second PCF8574 module) as outputs, all set LOW initially
    Wire.beginTransmission(PCF2_ADDR);
    Wire.write(0x00);   // set all outputs low 
    Wire.endTransmission();

    // Initialize system components
    initWebServer();          // Start the web server for remote access
    init_filesystem();        // Mount SPIFFS for session logs
    
    // Initialize EEPROM and load critical data
    init_eeprom();                      // Initialize EEPROM
    eeprom_read_and_increment_boot();   // Read and increment boot number
    eeprom_read_lifetime_count();       // Load lifetime reject count
    eeprom_read_machine_mode();         // Load machine mode (AUTO/REJECT)
    
    // Create new session file in SPIFFS
    create_session_file();
    
    // Display current system status on serial monitor
    Serial.println("---------------------STATUS----------------------");
    Serial.printf("Boot Number:        %-24lu\n", current_boot_number);
    Serial.printf("Session Count:      %-24lu\n", current_session_count);
    Serial.printf("Lifetime Rejects:   %-24lu\n", total_lifetime_count);
    Serial.printf("Machine Mode:       %-24s\n", state.machine_mode ? "REJECT" : "AUTO");
    Serial.println("--------------------------------------------------");
    
    // Initialize machine status and outputs
    machine_status = true;   // Start with machine relay ON
    relay_output(machine_status); 
    
    // Check machine mode and display appropriate startup message
    if (state.machine_mode) 
    {
        // System is in REJECT mode - waiting for operator intervention
        Serial.println(" SYSTEM IN REJECT MODE");
        Serial.println("Waiting for part confirmation in bin...");
        Serial.println("Place part to continue\n");
    } 
    else 
    {
        // System is in AUTO mode - ready for normal operation
        Serial.println(" SYSTEM READY");
        Serial.println("Press AUTO button to start production");
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
        
        // CONTINUOUS monitoring - runs every cycle, checks for B→A at all times
        check_monitoring(current_inputs);
        
        // Detect rising edge transitions (button press / sensor trigger)
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