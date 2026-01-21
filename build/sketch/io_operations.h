#line 1 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\io_operations.h"
#ifndef IO_OPERATIONS_H
#define IO_OPERATIONS_H
#include "config.h"

// Function to read input states from PCF8574 #1 (input module)
uint8_t read_inputs() 
{
    // Request 1 byte of data from PCF1 
    Wire.requestFrom(PCF1_ADDR, 1);
    
    if (Wire.available()) 
    {
        uint8_t raw = Wire.read();  // Read raw input byte
        return ~raw;  // Invert bits (PCF8574 inputs are active LOW, we want active HIGH)
    }
    return 0x00;  // Return all LOW if read fails
}

// Function to write output states to PCF8574 #2 (output module)
void write_outputs(uint8_t data) 
{
    Wire.beginTransmission(PCF2_ADDR);  // Start I2C transmission to PCF2
    Wire.write(data);                    // Send output byte (each bit controls one relay/LED)
    Wire.endTransmission();              // End transmission and apply changes
}

// Function to trigger a 500ms beep (non-blocking)
void trigger_beep()
{
    beep_active = true;
    beep_start_time = millis();
}

// Function to update all outputs based on current system state
void update_outputs()
{
    uint8_t out = 0x00;  // Start with all outputs OFF
    
    // RELAY: Machine Control ============
    if (machine_status) 
    {
        out |= (1 << RELAY_AUTO);  // Turn on machine relay
    }
    
    //  GREEN LED 
    if (machine_status) 
    {
        out |= (1 << LED_MACHINE_ON);  // Turn on GREEN LED when machine running
    }
    
    //  BUZZER  
    if (beep_active)
    {
        unsigned long elapsed = millis() - beep_start_time;
        
        if (elapsed < BEEP_DURATION)
        {
            out |= (1 << BUZZER_LED);  // Beep ON
        }
        else
        {
            beep_active = false;  // Beep complete
        }
    }
    
    // Write the constructed output byte to PCF2
    write_outputs(out);
}

// Function to control machine relay (turn machine ON or OFF)
void relay_output(bool on) 
{
    machine_status = on;  // Update global machine status flag
    update_outputs();     // Apply changes to all outputs
}

#endif