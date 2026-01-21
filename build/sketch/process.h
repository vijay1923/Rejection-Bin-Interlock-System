#line 1 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\process.h"
#ifndef PROCESS_H
#define PROCESS_H

// Forward declarations of handler functions
void auto_button_handler();
void reject_button_handler();

// Main function to process detected input events (button presses or sensor triggers)
void process_inputs(uint8_t edge) 
{
    // Check if AUTO button was pressed (rising edge detected)
    if (edge & (1 << BTN_AUTO)) 
    {
        auto_button_handler();  // Handle AUTO button press
    }
    
    // Check if REJECT button was pressed (rising edge detected)
    if (edge & (1 << BTN_REJECT)) 
    {
        reject_button_handler();  // Handle REJECT button press
    }
    
    // Check for slot sensor detection (handles A and B sensor sequences)
    reject_handler(edge);
}

// Handler for AUTO button press - starts/resumes machine operation
void auto_button_handler() 
{
    // Prevent AUTO mode activation if system is still in REJECT mode
    // Operator must confirm part in bin before resuming
    if (state.machine_mode) 
    {
        Serial.println("AUTO ignored - still in REJECT mode");
        return;
    }
    
    Serial.println("AUTO MODE - MACHINE RUNNING");
    
    // Clear monitoring from previous cycle - start fresh
    monitoring_active = false;
    part_count_incremented = false;
    monitoring_last_sensor = 0;
    monitored_slot = 0;
    
    // Set machine to AUTO mode (normal operation)
    state.machine_mode = false;
    active_slot = 0;  // Clear any active slot tracking
    
    // Save machine mode to persistent storage
    write_state();
    
    // Turn on machine relay (start production)
    relay_output(true);
}

// Handler for REJECT button press - stops machine and enters rejection mode
void reject_button_handler() 
{
    // Only allow REJECT mode if machine is currently running
    // Prevents rejecting parts when machine is already stopped
    if (!machine_status) 
    {
        Serial.println("[BUTTON] REJECT ignored - machine not running");
        return;
    }
    
    Serial.println("REJECT MODE - MACHINE STOPPED");
    Serial.println("Waiting for part in rejection bin...");
    
    // Clear monitoring from previous cycle - reset for new part
    monitoring_active = false;
    part_count_incremented = false;
    monitoring_last_sensor = 0;
    monitored_slot = 0;
    
    // Set machine to REJECT mode (waiting for part confirmation)
    state.machine_mode = true;
    active_slot = 0;  // Reset slot tracking for new sequence
    
    // Save machine mode to persistent storage
    write_state();
    
    // Turn off machine relay (stop production)
    relay_output(false);
    
    // Trigger 500ms beep when REJECT button pressed
    trigger_beep();
}

#endif