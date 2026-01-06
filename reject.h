#ifndef REJECT_H
#define REJECT_H

// Function to handle rejection bin sensor sequence detection and confirmation
void reject_handler(uint8_t edge) 
{
    // Only process sensor inputs when system is in reject mode
    // In AUTO mode, sensors are ignored
    if (!state.machine_mode) return;
    
    // Check if no slot is currently active (waiting for first sensor)
    if (active_slot == 0) 
    {
        // Check if Slot 1's first sensor (1A) was triggered
        if (edge & (1 << SLOT1_A)) 
        {
            active_slot = 1;  // Mark Slot 1 as active
            Serial.println("[SENSOR] Slot 1A detected - waiting for 1B...");
        }
        // Check if Slot 2's first sensor (2A) was triggered
        else if (edge & (1 << SLOT2_A)) 
        {
            active_slot = 2;  // Mark Slot 2 as active
            Serial.println("[SENSOR] Slot 2A detected - waiting for 2B...");
        }
        // Check if Slot 3's first sensor (3A) was triggered
        else if (edge & (1 << SLOT3_A)) 
        {
            active_slot = 3;  // Mark Slot 3 as active
            Serial.println("[SENSOR] Slot 3A detected - waiting for 3B...");
        }
        return;  // Exit and wait for second sensor in sequence
    }
    
    // Flag to track if the sensor sequence is valid (A followed by B)
    bool valid_sequence = false;
    
    // Check if Slot 1's second sensor (1B) was triggered after 1A
    if (active_slot == 1 && (edge & (1 << SLOT1_B))) 
    {
        valid_sequence = true;  // Complete sequence: 1A -> 1B
    }
    // Check if Slot 2's second sensor (2B) was triggered after 2A
    else if (active_slot == 2 && (edge & (1 << SLOT2_B))) 
    {
        valid_sequence = true;  // Complete sequence: 2A -> 2B
    }
    // Check if Slot 3's second sensor (3B) was triggered after 3A
    else if (active_slot == 3 && (edge & (1 << SLOT3_B))) 
    {
        valid_sequence = true;  // Complete sequence: 3A -> 3B
    }
    
    // If both sensors in sequence were triggered, part is confirmed in bin
    if (valid_sequence) 
    {
        // Switch system back to AUTO mode (exit reject mode)
        state.machine_mode = false;
        
        // Store which slot confirmed the part before resetting
        uint8_t slot = active_slot;
        active_slot = 0;  // Reset active slot tracker
        
        // Save updated machine mode to persistent storage
        write_state();
        
        // IMPORTANT: Save reject count to BOTH files
        // Updates both the current session file and the lifetime total count file
        save_reject_count();
        
        // // Send confirmation pulse to PLC/machine (200ms pulse on OUT_PULSE pin)
        // pulse_output(OUT_PULSE, 200);
        
        // Display confirmation results on serial monitor
        Serial.printf("Part Confirmed in Slot %d\n", slot);
        Serial.printf("Session Count (Boot #%lu): %lu\n", current_boot_number, current_session_count);
        Serial.printf("Lifetime Total Count: %lu\n", total_lifetime_count);
        Serial.println("Ready for next cycle");
    }
}

#endif