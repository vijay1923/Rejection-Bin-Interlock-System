#ifndef IO_OPERATIONS_H
#define IO_OPERATIONS_H
#include "config.h"

void i2c_enter_failsafe(const char* source)
{
    if (i2c_failsafe_latched)
        return;

    i2c_failsafe_latched = true;

    // Stop all active monitoring/state machine progression
    stop_monitoring();
    active_slot = 0;

    // Force machine to safe locked state
    machine_status = false;
    state.machine_mode = true; // REJECT lock mode
    eeprom_write_machine_mode();

    Serial.println("================================================");
    Serial.printf("[I2C FAIL-SAFE] Triggered by: %s\n", source);
    Serial.printf("[I2C FAIL-SAFE] Consecutive failures: %u\n", i2c_consecutive_failures);
    Serial.println("[I2C FAIL-SAFE] Machine locked in REJECT mode");
    Serial.println("================================================");

    // Emergency operator indication
    trigger_beep();
    trigger_beep();
    trigger_beep();
}

void i2c_record_failure(const char* source)
{
    if (i2c_consecutive_failures < 65535)
    {
        i2c_consecutive_failures++;
    }

    if (i2c_consecutive_failures >= I2C_FAILSAFE_THRESHOLD)
    {
        i2c_enter_failsafe(source);
    }
}

void i2c_record_success()
{
    if (i2c_consecutive_failures > 0)
    {
        Serial.printf("[I2C] Recovered after %u failed operation(s)\n", i2c_consecutive_failures);
    }
    i2c_consecutive_failures = 0;
}

// Function to read input states from PCF8574 #1 (input module)
uint8_t read_inputs() 
{
    // Request 1 byte of data from PCF1 with retry
    for (uint8_t attempt = 0; attempt < I2C_RETRY_COUNT; attempt++)
    {
        size_t bytes = Wire.requestFrom((int)PCF1_ADDR, 1);

        if (bytes == 1 && Wire.available())
        {
            uint8_t raw = Wire.read();  // Read raw input byte
            i2c_record_success();
            return ~raw;  // Invert bits (PCF8574 inputs are active LOW, we want active HIGH)
        }
    }

    i2c_record_failure("PCF1 input read");
    return 0x00;  // Return all LOW if read fails
}

// Function to write output states to PCF8574 #2 (output module)
bool write_outputs(uint8_t data) 
{
    // Retry output writes on bus errors
    for (uint8_t attempt = 0; attempt < I2C_RETRY_COUNT; attempt++)
    {
        Wire.beginTransmission(PCF2_ADDR);  // Start I2C transmission to PCF2
        Wire.write(data);                    // Send output byte (each bit controls one relay/LED)
        uint8_t err = Wire.endTransmission();

        if (err == 0)
        {
            i2c_record_success();
            return true;
        }
    }

    i2c_record_failure("PCF2 output write");
    return false;
}

// Function to trigger a 500ms beep for alerts
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
        unsigned long elapsed = millis() - beep_start_time;  // Calculate elapsed time since beep started
        
        if (elapsed < BEEP_DURATION)  // Beep duration not yet completed
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