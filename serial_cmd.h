// serial_cmd.h - Read-only serial command interface for viewing SPIFFS data
#ifndef SERIAL_CMD_H
#define SERIAL_CMD_H

#include "config.h"       // System configuration and definitions
#include "FS.h"           // File system library
#include "SPIFFS.h"       // SPI Flash File System library

// Function to display available serial commands and system information
void printHelp()
{
    Serial.println("---------------- HELP CENTER -------------");
    Serial.println("Serial Commands :");
    
    // LIST command - displays all files stored in SPIFFS
    Serial.println(" LIST");
    Serial.println("   - Print all stored files with data");
    Serial.println();
    
    // READ command - displays content of a specific file
    Serial.println(" READ <path>");
    Serial.println("   - Print data of a specific file");
    Serial.println("   - Example: READ /start_1.txt");
    Serial.println();
    
    // RST command - restart ESP32
    Serial.println(" RST");
    Serial.println("   - Restart ESP32");
    Serial.println();
    
    // HELP command - shows this help menu
    Serial.println(" HELP");
    Serial.println("   - Show this help information");
    Serial.println();
    
    // Web server connection information
    Serial.println(" Web Server : ");
    Serial.println(" - Connect to the Wifi ");
    Serial.println(" - SSID - RejectionBin_AP PASS - rejectionbin");
    Serial.println(" - Open 192.168.1.21 on Browser");
    
    // Description of stored data and their locations
    Serial.println();
    Serial.println("Data Storage:");
    Serial.println(" EEPROM (Fast, Power-Safe):");
    Serial.println("   - machine_mode      - AUTO/REJECT state");
    Serial.println("   - lifetime_count    - total rejects (never resets)");
    Serial.println("   - boot_number       - power cycles");
    Serial.println();
    Serial.println(" SPIFFS (Session History):");
    Serial.println("   - /version.txt      - firmware version");
    Serial.println("   - /start_X.txt      - session logs (X = 1 to 100)");
    Serial.println("------------------------------------------");
}

// Function to read and display the contents of a specific file
void readFile(const String &filepath)
{
    // Check if the requested file exists in SPIFFS
    if (!SPIFFS.exists(filepath)) 
    {
        Serial.println("ERR : FILE NOT FOUND - " + filepath);
        return;
    }

    // Attempt to open the file in read mode
    File file = SPIFFS.open(filepath, "r");
    if (!file) 
    {
        Serial.println("ERR: OPEN FAILED - " + filepath);
        return;
    }

    // Print file path as header
    Serial.print("FILE - ");
    Serial.print(filepath + " : ");

    // Read and print file contents byte by byte
    while (file.available()) 
    {
        Serial.write(file.read());
    }

    Serial.println();  // New line after file content
    file.close();      // Close file to free resources
}

// Function to list and display all files stored in SPIFFS
void listAllFiles()
{
    Serial.println();
    Serial.println("------------ Listing All Files --------------");
    
    // Display EEPROM data first
    Serial.println("[EEPROM Data]");
    Serial.printf("  Boot Number:      %lu\n", current_boot_number);
    Serial.printf("  Lifetime Count:   %lu\n", total_lifetime_count);
    Serial.printf("  Machine Mode:     %s\n", state.machine_mode ? "REJECT" : "AUTO");
    Serial.println();
    
    // Display SPIFFS files
    Serial.println("[SPIFFS Files]");
    readFile("/version.txt");   // Firmware version

    // Loop through all possible session files (1 to 100)
    char path[20];  // Buffer to hold file path string
    for (int i = 1; i <= 100; i++) 
    {
        // Construct file path for each session file
        snprintf(path, sizeof(path), "/start_%d.txt", i);
        
        // Only read and display if the file exists
        if (SPIFFS.exists(path)) 
        {
            readFile(path);
        }
    }
    
    Serial.println("---------------------------------------------");
}

// Main function to handle incoming serial commands
void handleSerialCommands()
{
    // Exit if no serial data is available
    if (!Serial.available())
        return;

    // Read command until newline character
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();           // Remove leading/trailing whitespace
    cmd.toUpperCase();    // Convert to uppercase for case-insensitive comparison

    // Process HELP command - display help information
    if (cmd == "HELP" ) 
    {
        printHelp();
    }
    // Process LIST command - display all files
    else if (cmd == "LIST") 
    {
        listAllFiles();
    }
    // Process READ command - display specific file
    else if (cmd.startsWith("READ ")) 
    {
        // Extract file path from command (everything after "READ ")
        String path = cmd.substring(5);
        path.trim();           // Remove extra spaces
        path.toLowerCase();    // File paths are case-sensitive, use lowercase
        readFile(path);        // Read and display the file
    }
    // Process RST command - restart ESP32
    else if(cmd == "RST")   // implement restart command
    {
        Serial.println("Restarting ESP in 3 seconds....");
        delay(3000); 
        ESP.restart();   
    }
    // Handle unknown commands
    else if (cmd.length() > 0)
    {
        Serial.println("ERR: UNKNOWN CMD - Type HELP for commands");
    }
}

#endif