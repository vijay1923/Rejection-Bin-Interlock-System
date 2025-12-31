#line 1 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\file_operations.h"
#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "config.h"
#include "FS.h"
#include "SPIFFS.h"

void init_filesystem() 
{
    Serial.println("FILE INIT ");
    
    if (!SPIFFS.begin(true)) 
    {
        Serial.println("PIFFS Mount failed!");
        return;
    }
    
    Serial.println("SPIFFS Mounted successfully");
    
    // Print filesystem info
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    size_t free = total - used;
    
    double totalMB = total / (1024.0 * 1024.0);
    double usedMB  = used  / (1024.0 * 1024.0);
    double freeMB  = free  / (1024.0 * 1024.0);
    
    Serial.printf("Total: %.2f MB | Used: %.2f MB | Free: %.2f MB\n", totalMB, usedMB, freeMB);
}

// boot number manager 
void init_boot_number() 
{
    File file = SPIFFS.open("/boot_number.txt", "r");
    
    if (!file) 
    {
        // First boot ever - create file
        current_boot_number = 1;
        Serial.println("First boot - initializing boot counter");
    } 
    else 
    {
        String line = file.readStringUntil('\n');
        file.close();
        current_boot_number = line.toInt() + 1;  // Increment boot number
    }
    
    // Save updated boot number
    file = SPIFFS.open("/boot_number.txt", "w");
    if (file) 
    {
        file.println(current_boot_number);
        file.close();
        Serial.printf("[BOOT] Boot Number: %lu\n", current_boot_number);
    }
    
    //  batch manager   max 100 boot  files 
   if (current_boot_number > MAX_START_FILES && (current_boot_number - 1) % MAX_START_FILES == 0) 
    {
        Serial.println("BATCH DELETE: Removing old session files");
        
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
    
    // new session file 
    String session_file = "/start_" + String(current_boot_number) + ".txt";
    file = SPIFFS.open(session_file, "w");
    if (file) 
    {
        file.println(0);  // Initialize with 0 rejects
        file.close();
        Serial.printf("[SESSION] Created: %s\n", session_file.c_str());
    }
    
    current_session_count = 0;  // Reset session counter
}

// total reject count 
void init_total_count() 
{
    File file = SPIFFS.open("/total_count.txt", "r");
    
    if (!file) 
    {
        // First time - create file
        total_lifetime_count = 0;
        Serial.println("No total count file - starting from 0");
        
        file = SPIFFS.open("/total_count.txt", "w");
        if (file) 
        {
            file.println(0);
            file.close();
        }
    } 
    else 
    {
        String line = file.readStringUntil('\n');
        file.close();
        total_lifetime_count = line.toInt();
        Serial.printf("Lifetime Count: %lu\n", total_lifetime_count);
    }
}

// machine state 
void write_state() 
{
    File file = SPIFFS.open("/state.txt", "w");
    if (!file) 
    {
        Serial.println("Failed to write");
        return;
    }
    
    file.println(state.machine_mode ? 1 : 0);  // machine mode 
    file.close();
}

void read_state() 
{
    File file = SPIFFS.open("/state.txt", "r");
    
    if (!file) 
    {
        // File doesn't exist - use default
        state.machine_mode = false;
        Serial.println("[STATE] No state file - using default (AUTO mode)");
        write_state();
        return;
    }
    
    String line = file.readStringUntil('\n');
    file.close();
    
    state.machine_mode = (line.toInt() == 1);
    Serial.printf("[STATE] Machine Mode: %s\n", state.machine_mode ? "REJECT" : "AUTO");
}


// reject count  Both Session & Total
void save_reject_count() 
{
    // Update session count
    current_session_count++;
    
    // Save to current session file
    String session_file = "/start_" + String(current_boot_number) + ".txt";
    File file = SPIFFS.open(session_file, "w");
    if (file) 
    {
        file.println(current_session_count);
        file.close();
    }
    
    // Update total count
    total_lifetime_count++;
    
    // Save to total count file
    file = SPIFFS.open("/total_count.txt", "w");
    if (file) 
    {
        file.println(total_lifetime_count);
        file.close();
    }
    
    // Update state.reject_count for display purposes
    state.reject_count = total_lifetime_count;
}

#endif