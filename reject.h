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
        state.reject_count++;
        state.machine_mode = false;
        uint8_t slot = active_slot;
        active_slot = 0;
        write_state();
        
        // Confirmation pulse
        pulse_output(OUT_PULSE, 200);
        
        Serial.printf("Rejection slot no : %d\n", slot);
        Serial.printf("Total Reject Count: %lu\n", state.reject_count);
        Serial.println("Ready for next cycle");
    }
}

/*
void handle_part_removal(uint8_t edge) 
{
    // Only detect removal when NOT in reject mode
    if (state.machine_mode || active_slot != 0) return;
    
    // Detect Sensor B triggered first (part exiting bin backwards)
    if (active_slot == 0) 
    {
        if (edge & (1 << SLOT1_B)) 
        {
            active_slot = -1;  // Negative flag for removal
            Serial.println("[SENSOR] Slot 1B detected - checking for removal...");
        }
        else if (edge & (1 << SLOT2_B)) 
        {
            active_slot = -2;
            Serial.println("[SENSOR] Slot 2B detected - checking for removal...");
        }
        else if (edge & (1 << SLOT3_B)) 
        {
            active_slot = -3;
            Serial.println("[SENSOR] Slot 3B detected - checking for removal...");
        }
        return;
    }
    
    // Confirm with Sensor A (part fully removed)
    bool valid_removal = false;
    
    if (active_slot == -1 && (edge & (1 << SLOT1_A))) 
    {
        valid_removal = true;
    }
    else if (active_slot == -2 && (edge & (1 << SLOT2_A))) 
    {
        valid_removal = true;
    }
    else if (active_slot == -3 && (edge & (1 << SLOT3_A))) 
    {
        valid_removal = true;
    }
    
    if (valid_removal) 
    {
        if (state.reject_count > 0) 
        {
            state.reject_count--;
            int8_t slot = -active_slot;
            active_slot = 0;
            write_state();
            Serial.printf("↑ PART REMOVED - Slot %d\n", slot);
            Serial.printf("↑ Total Reject Count: %lu\n", state.reject_count);
        }
    }
}
*/



#endif 