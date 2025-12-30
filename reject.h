#ifndef REJECT_H
#define REJECT_H

void reject_handler(uint8_t edge) 
{
    // Only process sensor inputs in reject mode
    if (!state.machine_mode) return;
    
    if (active_slot == 0) 
    {
        if (edge & (1 << SLOT1_A)) 
        {
            active_slot = 1;
            Serial.println("[SENSOR] Slot 1A detected - waiting for 1B...");
        }
        else if (edge & (1 << SLOT2_A)) 
        {
            active_slot = 2;
            Serial.println("[SENSOR] Slot 2A detected - waiting for 2B...");
        }
        else if (edge & (1 << SLOT3_A)) 
        {
            active_slot = 3;
            Serial.println("[SENSOR] Slot 3A detected - waiting for 3B...");
        }
        return;
    }
    
    bool valid_sequence = false;
    
    if (active_slot == 1 && (edge & (1 << SLOT1_B))) 
    {
        valid_sequence = true;
    }
    else if (active_slot == 2 && (edge & (1 << SLOT2_B))) 
    {
        valid_sequence = true;
    }
    else if (active_slot == 3 && (edge & (1 << SLOT3_B))) 
    {
        valid_sequence = true;
    }
    
    if (valid_sequence) 
    {
        // Exit reject mode
        state.machine_mode = false;
        uint8_t slot = active_slot;
        active_slot = 0;
        
        // Save machine mode
        write_state();
        
        // IMPORTANT: Save reject count to BOTH files
        save_reject_count();  // Updates session file + total count file
        
        // Send confirmation pulse
        pulse_output(OUT_PULSE, 200);
        
        // Display results
        Serial.printf("Part Confirmed in Slot %d\n", slot);
        Serial.printf("Session Count (Boot #%lu): %lu\n", current_boot_number, current_session_count);
        Serial.printf("Lifetime Total Count: %lu\n", total_lifetime_count);
        Serial.println("Ready for next cycle");
    }
}

#endif