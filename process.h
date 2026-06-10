#ifndef PROCESS_H
#define PROCESS_H

void auto_button_handler();
void reject_button_handler();

// Main function to process detected input events (button presses or sensor triggers)
void process_inputs(uint8_t edge) 
{
    unsigned long now = millis();

    // Check if AUTO button was pressed (rising edge detected)
    if (edge & (1 << BTN_AUTO)) 
    {
        if (now - last_auto_press_ms >= BUTTON_DEBOUNCE_MS)
        {
            last_auto_press_ms = now;
            auto_button_valid_count++;
            auto_button_handler();  // Handle AUTO button press
        }
        else
        {
            debounce_filtered_count++;
            Serial.println("[DEBOUNCE] Ignored AUTO bounce");
        }
    }
    
    // Check if REJECT button was pressed (rising edge detected)
    if (edge & (1 << BTN_REJECT)) 
    {
        if (now - last_reject_press_ms >= BUTTON_DEBOUNCE_MS)
        {
            last_reject_press_ms = now;
            reject_button_valid_count++;
            reject_button_handler();  // Handle REJECT button press
        }
        else
        {
            debounce_filtered_count++;
            Serial.println("[DEBOUNCE] Ignored REJECT bounce");
        }
    }
    
    // Check for slot sensor detection (handles A and B sensor sequences)
    reject_handler(edge);
}

// Handler for AUTO button press - starts/resumes machine operation
void auto_button_handler() 
{
    // If monitoring is active, count the part before switching to AUTO
    if (monitoring_active && !part_counted)
    {
        Serial.println("\n[AUTO] Button pressed - Counting monitored part...");
        increment_part_count();
    }
    
    // If still in REJECT mode, ignore AUTO button
    if (state.machine_mode) 
    {
        Serial.println("[AUTO] Ignored - still in REJECT mode (waiting for part)");
        return;
    }
    
    Serial.println("\n[AUTO] AUTO MODE - Starting new production cycle");
    
    // Clear monitoring - new cycle begins
    stop_monitoring();
    
    // Set machine to AUTO mode (normal operation)
    state.machine_mode = false;
    active_slot = 0;  // Clear any active slot tracking
    
    // Save machine mode to EEPROM
    eeprom_write_machine_mode();
    
    // Turn on machine relay (start production)
    relay_output(true);
    
    Serial.println("[READY] Machine running - Ready for next reject\n");
}

// Handler for REJECT button press - stops machine and enters rejection mode
// In REJECT mode, waits for part confirmation via sensors before resuming
void reject_button_handler() 
{
    // If monitoring is active, count the part before new reject
    if (monitoring_active && !part_counted)
    {
        Serial.println("\n[REJECT] Button pressed - Counting previous part...");
        increment_part_count();
    }
    
    // Only allow REJECT mode if machine is currently running
    if (!machine_status) 
    {
        Serial.println("[REJECT] Ignored - machine not running");
        return;
    }
    
    Serial.println("\n[REJECT] REJECT MODE - Machine stopped");
    Serial.println("[WAITING] Place rejected part in bin...\n");
    
    // Clear monitoring for new part
    stop_monitoring();
    
    // Set machine to REJECT mode (waiting for part confirmation)
    state.machine_mode = true;
    active_slot = 0;  // Reset slot tracking for new sequence
    
    // Save machine mode to EEPROM
    eeprom_write_machine_mode();
    
    // Turn off machine relay (stop production)
    relay_output(false);
    
    // Trigger 500ms beep when REJECT button pressed
    trigger_beep();
}

#endif