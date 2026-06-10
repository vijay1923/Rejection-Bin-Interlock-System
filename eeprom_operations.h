#ifndef EEPROM_OPERATIONS_H
#define EEPROM_OPERATIONS_H

#include <EEPROM.h>
#include "config.h"

// ─── Firmware version check ───────────────────────────────────────────────
// Call this BEFORE init_filesystem() and init_eeprom().
// Returns true if a version mismatch was detected (EEPROM reset triggered).
// When FIRMWARE_VERSION is bumped in config.h, EEPROM magic is wiped so
// init_eeprom() re-initializes all data, and SPIFFS is reformatted by
// check_spiffs_version() inside init_filesystem().
// This ensures that when you upload new firmware with changes to data structure
// or logic, it starts with a clean slate and avoids compatibility issues.
bool check_firmware_version()
{
    EEPROM.begin(EEPROM_SIZE);    // Safe to call before init_eeprom()

    uint16_t stored = ((uint16_t)EEPROM.read(ADDR_FW_VERSION) << 8)
                     | EEPROM.read(ADDR_FW_VERSION + 1);

    if (stored == FIRMWARE_VERSION)
    {
        Serial.printf("[FW] Version OK: v%u\n", FIRMWARE_VERSION);
        return false;
    }

    // Version changed (or device is brand-new)
    Serial.println("============================================");
    Serial.printf( "[FW] NEW FIRMWARE DETECTED: v%u -> v%u\n", stored, FIRMWARE_VERSION);
    Serial.println("[FW] Wiping EEPROM & SPIFFS - starting fresh");
    Serial.println("============================================");

    // Invalidate the magic number so init_eeprom() re-initialises everything
    EEPROM.write(ADDR_MAGIC,     0xFF);
    EEPROM.write(ADDR_MAGIC + 1, 0xFF);

    // Record the new firmware version
    EEPROM.write(ADDR_FW_VERSION,     (FIRMWARE_VERSION >> 8) & 0xFF);
    EEPROM.write(ADDR_FW_VERSION + 1,  FIRMWARE_VERSION        & 0xFF);

    EEPROM.commit();
    return true;
}

// Initialize EEPROM and check if it's first boot
void init_eeprom() 
{
    Serial.println("--------------EEPROM INIT-------------");
    
    // Initialize EEPROM with defined size
    EEPROM.begin(EEPROM_SIZE);
    
    // Read magic number to check if EEPROM has been initialized before
    uint16_t magic = (EEPROM.read(ADDR_MAGIC) << 8) | EEPROM.read(ADDR_MAGIC + 1);   // Read 2 bytes (magic number for first boot detection)
    
    if (magic != EEPROM_MAGIC)   // if the eeprom is uninitialized
    {
        // First boot ever - initialize EEPROM with defaults
        Serial.println("First boot detected - initializing EEPROM...");
        
        // Write magic number
        EEPROM.write(ADDR_MAGIC, (EEPROM_MAGIC >> 8) & 0xFF);  // High byte
        EEPROM.write(ADDR_MAGIC + 1, EEPROM_MAGIC & 0xFF);   // Low byte
        
        // Initialize machine mode to AUTO (false = 0)
        EEPROM.write(ADDR_MACHINE_MODE, 0);
        
        // Initialize lifetime count to 0
        EEPROM.write(ADDR_LIFETIME_COUNT, 0);   // Most byte (MSB) for lifetime rejection count
        EEPROM.write(ADDR_LIFETIME_COUNT + 1, 0);  // 2nd byte
        EEPROM.write(ADDR_LIFETIME_COUNT + 2, 0); // 3rd byte
        EEPROM.write(ADDR_LIFETIME_COUNT + 3, 0);  // Least byte (LSB)
        
        // Initialize boot number to 1
        EEPROM.write(ADDR_BOOT_NUMBER, 0);  // Most byte (MSB)
        EEPROM.write(ADDR_BOOT_NUMBER + 1, 0);  //
        EEPROM.write(ADDR_BOOT_NUMBER + 2, 0);
        EEPROM.write(ADDR_BOOT_NUMBER + 3, 1);
        
        // Write current firmware version
        EEPROM.write(ADDR_FW_VERSION,     (FIRMWARE_VERSION >> 8) & 0xFF);
        EEPROM.write(ADDR_FW_VERSION + 1,  FIRMWARE_VERSION        & 0xFF);

        // Commit changes to EEPROM
        EEPROM.commit();
        
        Serial.println("EEPROM initialized with defaults");
    } 
    else 
    {
        Serial.println("EEPROM already initialized");   
    }
    
    Serial.println("-------------------------------");
}

// Read machine mode from EEPROM
void eeprom_read_machine_mode() 
{
    uint8_t mode = EEPROM.read(ADDR_MACHINE_MODE);   // Read machine mode byte
    state.machine_mode = (mode == 1); // true = REJECT mode, false = AUTO mode
    Serial.printf("[EEPROM] Machine Mode: %s\n", state.machine_mode ? "REJECT" : "AUTO");  // Update state for display
}

// Write machine mode to EEPROM
void eeprom_write_machine_mode() 
{
    uint8_t mode = state.machine_mode ? 1 : 0;   // check machine state (auto/reject)
    EEPROM.write(ADDR_MACHINE_MODE, mode);   // Write machine state
    EEPROM.commit();
    Serial.printf("[EEPROM] Saved Machine Mode: %s\n", state.machine_mode ? "REJECT" : "AUTO");   // Update state for display
}

// Read lifetime count from EEPROM
void eeprom_read_lifetime_count() 
{
    total_lifetime_count = 0;
    total_lifetime_count |= ((uint32_t)EEPROM.read(ADDR_LIFETIME_COUNT) << 24);  // Most byte (MSB)
    total_lifetime_count |= ((uint32_t)EEPROM.read(ADDR_LIFETIME_COUNT + 1) << 16); // 2nd byte
    total_lifetime_count |= ((uint32_t)EEPROM.read(ADDR_LIFETIME_COUNT + 2) << 8); // 3rd byte
    total_lifetime_count |= ((uint32_t)EEPROM.read(ADDR_LIFETIME_COUNT + 3));  // Least byte (LSB) 
    
    Serial.printf("[EEPROM] Lifetime Count: %lu\n", total_lifetime_count);  // Update state for display
}

// Write lifetime count to EEPROM  by 1 byte at a time
void eeprom_write_lifetime_count() 
{
    EEPROM.write(ADDR_LIFETIME_COUNT, (total_lifetime_count >> 24) & 0xFF);  // Most byte (MSB)
    EEPROM.write(ADDR_LIFETIME_COUNT + 1, (total_lifetime_count >> 16) & 0xFF);  // 2nd byte 
    EEPROM.write(ADDR_LIFETIME_COUNT + 2, (total_lifetime_count >> 8) & 0xFF); // 3rd byte
    EEPROM.write(ADDR_LIFETIME_COUNT + 3, total_lifetime_count & 0xFF); // List byte (LSB)
    EEPROM.commit();
    
    // Update state for display
    state.reject_count = total_lifetime_count;
}

// Read boot number from EEPROM and increment it
void eeprom_read_and_increment_boot() 
{
    // Read current boot number
    current_boot_number = 0;
    current_boot_number |= ((uint32_t)EEPROM.read(ADDR_BOOT_NUMBER) << 24);  // Most byte (MSB)
    current_boot_number |= ((uint32_t)EEPROM.read(ADDR_BOOT_NUMBER + 1) << 16);  // 2nd byte
    current_boot_number |= ((uint32_t)EEPROM.read(ADDR_BOOT_NUMBER + 2) << 8); // 3rd byte
    current_boot_number |= ((uint32_t)EEPROM.read(ADDR_BOOT_NUMBER + 3));  // Least byte (LSB)
    
    // Increment for this boot
    current_boot_number++; 
    
    // Write back incremented value
    EEPROM.write(ADDR_BOOT_NUMBER, (current_boot_number >> 24) & 0xFF);  // Most byte (MSB)
    EEPROM.write(ADDR_BOOT_NUMBER + 1, (current_boot_number >> 16) & 0xFF); // 2nd byte
    EEPROM.write(ADDR_BOOT_NUMBER + 2, (current_boot_number >> 8) & 0xFF);  // 3rd byte
    EEPROM.write(ADDR_BOOT_NUMBER + 3, current_boot_number & 0xFF); // Least byte (LSB)
    EEPROM.commit();    // commit changes to EEPROM
    
    Serial.printf("[EEPROM] Boot Number: %lu\n", current_boot_number);
}

// Increment lifetime count (called when part is confirmed in bin)
void eeprom_increment_lifetime_count() 
{
    total_lifetime_count++;
    eeprom_write_lifetime_count();
}

#endif