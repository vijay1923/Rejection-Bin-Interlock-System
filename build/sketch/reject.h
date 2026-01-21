#line 1 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\reject.h"
#ifndef REJECT_H
#define REJECT_H
#include "config.h"

// Function to start monitoring after A→B sequence
void start_monitoring(uint8_t slot)
{
    monitoring_active = true;
    monitoring_start_time = millis();
    monitored_slot = slot;
    Serial.printf("[MONITORING] Started for Slot %d - checking for removal (Duration: %d ms)\n", slot, MONITORING_DURATION);
}

// Unified continuous monitoring function - detects B→A cheating sequence
// Continuously monitors until AUTO or REJECT button is pressed
void check_monitoring(uint8_t current_inputs)
{
    if (!monitoring_active) return;  // Exit if monitoring is not active
    
    // Get the sensor pins for the monitored slot
    uint8_t sensor_a_pin = 0;
    uint8_t sensor_b_pin = 0;
    
    if (monitored_slot == 1) 
    {
        sensor_a_pin = SLOT1_A;
        sensor_b_pin = SLOT1_B;
    }
    else if (monitored_slot == 2) 
    {
        sensor_a_pin = SLOT2_A;
        sensor_b_pin = SLOT2_B;
    }
    else if (monitored_slot == 3) 
    {
        sensor_a_pin = SLOT3_A;
        sensor_b_pin = SLOT3_B;
    }
    
    // Track which sensor is currently triggered
    uint8_t current_sensor = 0;
    if (current_inputs & (1 << sensor_b_pin))
        current_sensor = 2;  // Sensor B triggered
    else if (current_inputs & (1 << sensor_a_pin))
        current_sensor = 1;  // Sensor A triggered
    
    // ===== B→A CHEATING DETECTION =====
    // If Sensor B was last triggered, and now Sensor A triggers = REMOVAL DETECTED
    if (current_sensor == 1 && monitoring_last_sensor == 2)
    {
        // B→A sequence detected = Part being pulled out = CHEATING!
        unsigned long elapsed = millis() - monitoring_start_time;
        bool part_already_counted = (elapsed >= MONITORING_DURATION) && part_count_incremented;
        
        if (part_already_counted)
        {
            Serial.printf("[CHEATING DETECTED!] B→A removal sequence - Illegal removal of counted part (Slot %d)\n", monitored_slot);
            Serial.println("SYSTEM STOPPING - Part already counted, cannot be removed!");
        }
        else
        {
            Serial.printf("[CHEATING DETECTED!] B→A removal sequence - Part being removed before confirmation (Slot %d)\n", monitored_slot);
            Serial.println("Part being removed - Count NOT incremented!");
        }
        
        // RESET AND STOP MACHINE IMMEDIATELY
        monitoring_active = false;
        part_count_incremented = false;
        monitoring_last_sensor = 0;
        active_slot = 0;
        monitored_slot = 0;
        
        // Stop machine immediately
        state.machine_mode = true;  // Enter REJECT mode to stop
        relay_output(false);  // Turn off machine relay
        write_state();
        
        // Alert operator with emergency beeps
        Serial.println("[SYSTEM STOPPED] Machine halted due to cheating detection!");
        Serial.printf("Session Count: %lu\n", current_session_count);
        Serial.printf("Lifetime Count: %lu\n", total_lifetime_count);
        trigger_beep();  // Alert beep
        trigger_beep();  // Double beep for alarm
        trigger_beep();  // Triple beep = EMERGENCY
        return;
    }
    
    // ===== UPDATE LAST SENSOR TRACKER =====
    if (current_sensor > 0)
    {
        monitoring_last_sensor = current_sensor;  // Track which sensor is active
    }
    
    // ===== CHECK IF 5-SECOND MONITORING PERIOD HAS COMPLETED =====
    unsigned long elapsed = millis() - monitoring_start_time;
    
    if (elapsed >= MONITORING_DURATION && !part_count_incremented)
    {
        // Monitoring period passed without B→A cheating - SAFE TO COUNT
        part_count_incremented = true;  // Mark as counted
        
        // Increment count NOW (after 5-second safety period passed)
        save_reject_count();
        
        Serial.println("\n[MONITORING] 5-second safety period complete - Part confirmed safe!");
        Serial.printf("Part Confirmed in Slot %d (no B→A removal detected)\n", monitored_slot);
        Serial.printf("Session Count (Boot #%lu): %lu\n", current_boot_number, current_session_count);
        Serial.printf("Lifetime Total Count: %lu\n", total_lifetime_count);
        Serial.println("[CONTINUOUS CHECK] Monitoring for post-count removal attempts...");
        Serial.println("Part is now locked - B→A sequence will trigger alert!\n");
    }
}

// Function to handle rejection bin sensor sequence detection and confirmation
void reject_handler(uint8_t edge) 
{
    if (!state.machine_mode)  // Only process sensor inputs when system is in reject mode
        return;
    
    // Check if no slot is currently active (waiting for first sensor)
    if (active_slot == 0) 
    {
        // Check if Slot 1's first sensor (1A) was triggered
        if (edge & (1 << SLOT1_A)) 
        {
            active_slot = 1;
            Serial.println("[SENSOR] Slot 1A detected - waiting for 1B...");
            trigger_beep();  //  Beep when Sensor A triggers
        }
        // Check if Slot 2's first sensor (2A) was triggered
        else if (edge & (1 << SLOT2_A)) 
        {
            active_slot = 2;
            Serial.println("[SENSOR] Slot 2A detected - waiting for 2B...");
            trigger_beep();  //  Beep when Sensor A triggers
        }
        // Check if Slot 3's first sensor (3A) was triggered
        else if (edge & (1 << SLOT3_A)) 
        {
            active_slot = 3;
            Serial.println("[SENSOR] Slot 3A detected - waiting for 3B...");
            trigger_beep();  // Beep when Sensor A triggers
        }
        return;
    }
    
    // Flag to track if the sensor sequence is valid (A followed by B)
    bool valid_sequence = false;
    
    // Check if Slot 1's second sensor (1B) was triggered after 1A
    if (active_slot == 1 && (edge & (1 << SLOT1_B))) 
    {
        valid_sequence = true;
        Serial.println("Slot-1: B detected, Sequence complete");
        trigger_beep();  // Beep when Sensor B triggers
    }
    // Check if Slot 2's second sensor (2B) was triggered after 2A
    else if (active_slot == 2 && (edge & (1 << SLOT2_B))) 
    {
        valid_sequence = true;   
        Serial.println("Slot-2: B detected, Sequence complete");
        trigger_beep();  //Beep when Sensor B triggers
    }
    // Check if Slot 3's second sensor (3B) was triggered after 3A
    else if (active_slot == 3 && (edge & (1 << SLOT3_B))) 
    {
        valid_sequence = true;   
        Serial.println("Slot-3: B detected, Sequence complete");
        trigger_beep();  //  Beep when Sensor B triggers
    }
    
    // If both sensors in sequence were triggered
    if (valid_sequence) 
    {
        Serial.printf("[SENSOR] Slot %d B detected - A→B sequence complete\n", active_slot);
        
        // Switch system back to AUTO mode
        state.machine_mode = false;
        
        uint8_t slot = active_slot;
        active_slot = 0;
        
        // Save state
        write_state();
        
        // Start monitoring for cheating attempts
        start_monitoring(slot);
        
        // Start relay immediately after part is in bin
        relay_output(true);
        
        Serial.println("[RELAY] Machine restarted - part confirmed in rejection bin");
    }
}

#endif