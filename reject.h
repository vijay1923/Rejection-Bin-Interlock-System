#ifndef REJECT_H
#define REJECT_H
#include "config.h"

// Function to start continuous monitoring after A→B sequence
void start_monitoring(uint8_t slot)
{
    monitoring_active = true;
    monitored_slot = slot;
    part_counted = false;  // Part not yet counted
    monitoring_last_sensor = 0;  // Reset sensor tracking
    Serial.printf("[MONITORING] Started continuous monitoring for Slot %d\n", slot);
    Serial.println("[MONITORING] Part will be counted when AUTO or REJECT button is pressed");
    Serial.println("[MONITORING] B→A removal will trigger immediate alert!");
}

// Stop monitoring and clear all monitoring states
void stop_monitoring()
{
    if (monitoring_active)
    {
        Serial.printf("[MONITORING] Stopped monitoring for Slot %d\n", monitored_slot);
    }
    monitoring_active = false;
    monitored_slot = 0;
    monitoring_last_sensor = 0;
    part_counted = false;
}

// Continuous monitoring function - detects B→A cheating sequence at any time
// Runs continuously until AUTO or REJECT button is pressed
void check_monitoring(uint8_t current_inputs)
{
    if (!monitoring_active) 
    return;  // Exit if monitoring is not active
    
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
        
        if (part_counted)
        {
            // Part was already counted - SERIOUS VIOLATION
            Serial.println(" ############### CRITICAL CHEATING DETECTED! ################");
            Serial.printf("[ALERT] B→A removal in Slot %d - COUNTED part removed!\n", monitored_slot);
            Serial.println("[ALERT] This is a SERIOUS violation - part was already counted!");
        }
        else
        {
            // Part not yet counted - removal before confirmation
            Serial.println("################ CHEATING DETECTED! ################");
            Serial.printf("[ALERT] B→A removal in Slot %d - Part removed before counting!\n", monitored_slot);
            Serial.println("[ALERT] Part must be re-confirmed in bin!");
        }
        
        // RESET AND STOP MACHINE IMMEDIATELY
        stop_monitoring();  // Clear all monitoring states
        active_slot = 0;
        
        // Stop machine immediately
        state.machine_mode = true;  // Enter REJECT mode
        relay_output(false);  // Turn off machine relay
        eeprom_write_machine_mode();  // Save to EEPROM
        
        // Alert operator with emergency beeps
        Serial.println("\n[SYSTEM STOPPED] Machine halted - Cheating detected!");
        Serial.printf("Session Count: %lu\n", current_session_count);
        Serial.printf("Lifetime Count: %lu\n", total_lifetime_count);
        Serial.println("System locked - Re-confirm part in bin to continue\n");
        
        trigger_beep();  // Triple beep = EMERGENCY ALERT for cheating detection
        trigger_beep();
        trigger_beep();
        return;
    }
    
    // ===== UPDATE LAST SENSOR TRACKER =====
    if (current_sensor > 0)
    {
        monitoring_last_sensor = current_sensor;  // Track which sensor is active
    }
}

// Function to increment count when AUTO or REJECT button is pressed
void increment_part_count()
{
    if (!monitoring_active || part_counted)
        return;  // Only count if monitoring is active and part not yet counted
    
    // Mark part as counted
    part_counted = true;
    
    // Increment counts
    eeprom_increment_lifetime_count();     // Increment and save lifetime count to EEPROM
    save_session_reject_count();          // Increment and save session count to SPIFFS
    
    Serial.println("-----------------------------------------");
    Serial.printf("Part confirmed in Slot %d - Count incremented\n", monitored_slot);
    Serial.printf("Session Count (Boot #%lu): %lu\n", current_boot_number, current_session_count);
    Serial.printf("Lifetime Total Count: %lu\n", total_lifetime_count);
    Serial.println("\n[MONITORING] Continuous monitoring still active");
    Serial.println("[WARNING] Part is now LOCKED - B→A removal will trigger alert!\n");
    Serial.println("-----------------------------------------");
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
            Serial.println("\n[SENSOR] Slot 1A detected - waiting for 1B...");
            trigger_beep();  // Beep when Sensor A triggers
        }
        // Check if Slot 2's first sensor (2A) was triggered
        else if (edge & (1 << SLOT2_A)) 
        {
            active_slot = 2;
            Serial.println("\n[SENSOR] Slot 2A detected - waiting for 2B...");
            trigger_beep();  // Beep when Sensor A triggers
        }
        // Check if Slot 3's first sensor (3A) was triggered
        else if (edge & (1 << SLOT3_A)) 
        {
            active_slot = 3;
            Serial.println("\n[SENSOR] Slot 3A detected - waiting for 3B...");
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
        Serial.println("[SENSOR] Slot 1B detected - A→B sequence complete!");
        trigger_beep();  // Beep when Sensor B triggers
    }
    // Check if Slot 2's second sensor (2B) was triggered after 2A
    else if (active_slot == 2 && (edge & (1 << SLOT2_B))) 
    {
        valid_sequence = true;     // Valid A→B sequence
        Serial.println("[SENSOR] Slot 2B detected - A→B sequence complete!");
        trigger_beep();  // Beep when Sensor B triggers
    }
    // Check if Slot 3's second sensor (3B) was triggered after 3A
    else if (active_slot == 3 && (edge & (1 << SLOT3_B))) 
    {
        valid_sequence = true;   
        Serial.println("[SENSOR] Slot 3B detected - A→B sequence complete!");
        trigger_beep();  // Beep when Sensor B triggers
    }
    
    // If both sensors in sequence were triggered
    if (valid_sequence) 
    {
        Serial.printf("\n[CONFIRMED] Part detected in Slot %d - Sequence validated\n", active_slot);
        
        // If there was a previous part being monitored, count it now
        if (monitoring_active && !part_counted)
        {
            Serial.println("[AUTO-COUNT] Previous part being counted before new part...");
            increment_part_count();
        }
        
        // Switch system back to AUTO mode
        state.machine_mode = false;
        
        uint8_t slot = active_slot;
        active_slot = 0;
        
        // Save state to EEPROM
        eeprom_write_machine_mode();
        
        // Start continuous monitoring for this part
        start_monitoring(slot);
        
        // Start relay immediately - machine can run while monitoring
        relay_output(true);
        
        Serial.println("[RELAY] Machine restarted - part confirmed in rejection bin");
        Serial.println("[READY] Press AUTO to count part and continue, or REJECT for another part\n");
    }
}

#endif