#line 1 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\file_operations.h"
#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H
#include "config.h"
#include "WString.h"
#include "FS.h"
#include "SPIFFS.h"
#include "FFat.h"

// Initialize filesystem
void init_filesystem() 
{
    if (!SPIFFS.begin(true)) // It activates the flash area reserved for SPIFFS so it behaves like a tiny file storage.
    {
        Serial.println("Mount failed");
        return;
    }
    Serial.println("Mounted successfully");
    
    // Print filesystem info in bytes 
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    size_t free = total - used;
   // Serial.printf("Total: %u bytes, Used: %u bytes, Free: %u bytes\n", total, used, free);

    // Print filesystem info in mb 
    double totalMB = total / (1024.0 * 1024.0);
    double usedMB  = used  / (1024.0 * 1024.0);
    double freeMB  = free  / (1024.0 * 1024.0);
    Serial.printf("Total: %.2f MB, Used: %.2f MB, Free: %.2f MB\n",totalMB, usedMB, freeMB);
}

void write_state() 
{
    File file = SPIFFS.open("/state.txt", "w");
    if (!file) 
    {
        Serial.println("Failed to open file for writing");
        return;
    }
    file.printf("%lu\n%d\n", state.reject_count, state.machine_mode ? 1 : 0);
    file.close();
    Serial.println("State saved");
}

void read_state() 
{
    File file = SPIFFS.open("/state.txt", "r");
    if (!file) 
    {
        // File doesn't exist - use defaults
        state.reject_count = 0;
        state.machine_mode = false;
        Serial.println("State file not found - using defaults");
        write_state();
        return;
    }
    
    String line1 = file.readStringUntil('\n');
    String line2 = file.readStringUntil('\n');
    file.close();
    
    state.reject_count = line1.toInt();
    state.machine_mode = (line2.toInt() == 1);
    
    // Validate rejection count 
    if (state.reject_count > 1000000) 
    {
        // Invalid data - use defaults
        state.reject_count = 0;
        state.machine_mode = false;
        Serial.println("Invalid data - using defaults");
        write_state();
    } 
    else 
    {
        Serial.println("State restored");
    }
}


#endif 