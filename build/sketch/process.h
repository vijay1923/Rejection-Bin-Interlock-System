#line 1 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\process.h"
#ifndef PROCESS_H
#define PROCESS_H

void auto_button_handler();
void reject_button_handler();

void process_inputs(uint8_t edge) 
{
    // Priority 1: Button handling
    if (edge & (1 << BTN_AUTO)) 
    {
        auto_button_handler();
    }
    
    if (edge & (1 << BTN_REJECT)) 
    {
        reject_button_handler();
    }
    
    // Priority 2: Slot detection (only in reject mode)
    reject_handler(edge);
    
    // Priority 3: Part removal 
    // handle_part_removal(edge);
}


void auto_button_handler() 
{
    if (state.machine_mode) 
    {
        Serial.println("[BUTTON] AUTO ignored - still in REJECT mode");
        return;
    }
    
    Serial.println("AUTO MODE - MACHINE RUNNING");    
    state.machine_mode = false;
    active_slot = 0;
    write_state();
    relay_output(true);
}

void reject_button_handler() 
{
    if (!machine_status) 
    {
        Serial.println("[BUTTON] REJECT ignored - machine not running");
        return;
    }
    
    Serial.println("REJECT MODE - MACHINE STOPPED");
    Serial.println("Waiting for part in rejection bin...");
    
    state.machine_mode = true;
    active_slot = 0;
    write_state();
    relay_output(false);
}



#endif 