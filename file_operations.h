#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "config.h"
#include "FS.h"
#include "SPIFFS.h"

// Function to check if SPIFFS version matches firmware version
// Auto-reformats when firmware is updated with new file structure
bool check_spiffs_version() 
{
    File file = SPIFFS.open("/version.txt", "r");
    
    if (!file) 
    {
        // No version file exists - create it (first boot with version tracking)
        file = SPIFFS.open("/version.txt", "w");
        if (file) 
        {
            file.println(SPIFFS_VERSION);  // Write current version
            file.close();
        }
        return true;
    }
    
    // Read stored version number
    int stored_version = file.readStringUntil('\n').toInt();  // Read version as integer
    file.close();  
    
    // Check if stored version matches firmware's expected version
    if (stored_version != SPIFFS_VERSION) 
    {
        Serial.printf("Version mismatch: stored=%d, expected=%d\n", stored_version, SPIFFS_VERSION);
        Serial.println("Reformatting SPIFFS for new firmware version...");
        
        // Reformat for new firmware version
        SPIFFS.end();
        if (SPIFFS.format()) 
        {
            SPIFFS.begin(false);
            
            // Create new version file with current version
            file = SPIFFS.open("/version.txt", "w");
            if (file) 
            {
                file.println(SPIFFS_VERSION);
                file.close();
            }
            Serial.println("SPIFFS updated to firmware version");
            return true;
        }
        return false;
    }
    return true;
}

// Function to initialize and mount the SPIFFS file system
void init_filesystem() 
{
    Serial.println("----------------------SPIFFS INIT-----------------");
    
    // Try to mount SPIFFS with auto-format enabled
    if (!SPIFFS.begin(true)) 
    {
        Serial.println("SPIFFS Mount failed!");
        return;
    }
    
    Serial.println("SPIFFS Mounted successfully");
    
    // Check version compatibility
    check_spiffs_version();
    
    // Print filesystem info
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    size_t free = total - used;
    
    double totalMB = total / (1024.0 * 1024.0);
    double usedMB  = used  / (1024.0 * 1024.0);
    double freeMB  = free  / (1024.0 * 1024.0);
    
    Serial.printf("Total: %.2f MB | Used: %.2f MB | Free: %.2f MB\n", totalMB, usedMB, freeMB);
    Serial.println("------------------------------------------");
}

// Create new session file for current boot
void create_session_file() 
{
    // Batch manager - delete old files if we exceed MAX_START_FILES
    if (current_boot_number > MAX_START_FILES && (current_boot_number - 1) % MAX_START_FILES == 0) 
    {
        Serial.println("[SPIFFS] BATCH DELETE: Removing old session files");
        
        uint32_t start_delete = current_boot_number - MAX_START_FILES;
        uint32_t end_delete = current_boot_number - 1;
        
        for (uint32_t i = start_delete; i <= end_delete; i++) 
        {
            String filename = "/start_" + String(i) + ".txt";
            if (SPIFFS.exists(filename)) 
            {
                SPIFFS.remove(filename);
                Serial.printf("  Deleted: %s\n", filename.c_str());
            }
        }
        
        Serial.printf("Deleted files from start_%lu to start_%lu\n", start_delete, end_delete);
    }
    
    // Create new session file for this boot
    String session_file = "/start_" + String(current_boot_number) + ".txt";
    File file = SPIFFS.open(session_file, "w");
    if (file) 
    {
        file.println(0);  // Initialize with 0 rejects
        file.close();
        Serial.printf("[SPIFFS] Created session file: %s\n", session_file.c_str());
    }
    
    // Reset session counter
    current_session_count = 0;
}

// Save reject count to current session file
void save_session_reject_count() 
{
    // Increment session count
    current_session_count++;
    
    // Save to current session file
    String session_file = "/start_" + String(current_boot_number) + ".txt";
    File file = SPIFFS.open(session_file, "w");
    if (file) 
    {
        file.println(current_session_count);
        file.close();
    }
    
    Serial.printf("[SPIFFS] Session count updated: %lu\n", current_session_count);
}

#endif