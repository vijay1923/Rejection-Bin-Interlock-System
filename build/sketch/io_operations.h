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
                     // Example: Button pressed = 0 on PCF → inverted to 1 for easier logic
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

// Function to update all outputs based on current system state
void update_outputs()
{
    uint8_t out = 0x00;  // Start with all outputs OFF (0x00 = all bits LOW)
    
    // Set AUTO relay bit if machine is running
    if (machine_status) 
    {
        out |= (1 << RELAY_AUTO);  // Turn on machine relay
    }
    
    // Set REJECT LED bit if system is in reject mode
    if (state.machine_mode) 
    {
        out |= (1 << LED_REJECT);  // Turn on reject indicator LED
    }
    
    // Blink buzzer/LED in reject mode to alert operator
    if (state.machine_mode) 
    {
        // Toggle buzzer state at regular intervals (defined by BUZZER_INTERVAL)
        if (millis() - last_buzzer_toggle >= BUZZER_INTERVAL)
        {
            last_buzzer_toggle = millis();   // Update last toggle time
            buzzer_state = !buzzer_state;    // Flip buzzer state (ON/OFF)
        }
        
        // Set buzzer/LED bit if currently in ON state
        if (buzzer_state)
        {
            out |= (1 << BUZZER_LED);  // Turn on buzzer/LED
        }
    }
    else
    {
        buzzer_state = false;  // Ensure buzzer is off when not in reject mode
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

// Function to send a timed pulse on a specific output pin
// Used for sending confirmation signals to PLC/machine
void pulse_output(uint8_t pin, uint16_t duration_ms) 
{
    uint8_t out = 0x00;  // Start with all outputs OFF
    
    // Preserve current AUTO relay state
    if (machine_status) out |= (1 << RELAY_AUTO);
    
    // Preserve current REJECT mode indicators
    if (state.machine_mode) 
    {
        out |= (1 << LED_REJECT);              // Keep reject LED on
        if (buzzer_state) out |= (1 << BUZZER_LED);  // Keep buzzer state
    }
    
    // Turn ON the specified pin for the pulse
    out |= (1 << pin);
    write_outputs(out);     // Apply pulse start
    delay(duration_ms);     // Hold pulse for specified duration
    
    // Turn OFF the specified pin to end the pulse
    out &= ~(1 << pin);     // Clear the pin bit (bitwise AND with inverted mask)
    write_outputs(out);     // Apply pulse end
}

#endif