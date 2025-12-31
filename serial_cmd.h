// serial_cmd.h - Read-only serial command interface for viewing SPIFFS data
#ifndef SERIAL_CMD_H
#define SERIAL_CMD_H

#include "config.h"
#include "FS.h"
#include "SPIFFS.h"

void printHelp()
{
    Serial.println("---------------- HELP CENTER -------------");
    Serial.println("Serial Commands :");
    Serial.println(" LIST");
    Serial.println("   - Print all stored file data");
    Serial.println();
    Serial.println(" READ <path>");
    Serial.println("   - Print data of a specific file");
    Serial.println("   - Example: READ /total_count.txt");
    Serial.println();
    Serial.println(" HELP");
    Serial.println("   - Show this help information");
    Serial.println();
    Serial.println(" Web Server : ");
    Serial.println(" - Connect to the Wifi ");
    Serial.println(" - SSID - RejectionBin_AP PASS - rejection ");
    Serial.println(" - Open 192.168.1.21 on Browser");
    Serial.println("Files:");
    Serial.println(" /boot_number.txt   - total power cycles");
    Serial.println(" /total_count.txt   - lifetime reject count");
    Serial.println(" /state.txt         - machine mode");
    Serial.println(" /start_X.txt       - session data (X = 1 to 100)");
    Serial.println("------------------------------------------");
}

void readFile(const String &filepath)
{
    if (!SPIFFS.exists(filepath)) 
    {
        Serial.println("ERR : FILE NOT FOUND - " + filepath);
        return;
    }

    File file = SPIFFS.open(filepath, "r");
    if (!file) 
    {
        Serial.println("ERR: OPEN FAILED - " + filepath);
        return;
    }

    Serial.print("FILE - ");
    Serial.print(filepath + " : ");

    while (file.available()) 
    {
        Serial.write(file.read());
    }

    Serial.println();  
    file.close();
}

void listAllFiles()
{
    Serial.println();
    Serial.println("------------ Listing All Files --------------");
    
    // Core files
    readFile("/boot_number.txt");
    readFile("/total_count.txt");
    readFile("/state.txt");

    // Session files
    char path[20];
    for (int i = 1; i <= 100; i++) 
    {
        snprintf(path, sizeof(path), "/start_%d.txt", i);
        if (SPIFFS.exists(path)) 
        {
            readFile(path);
        }
    }
    
    Serial.println("---------------------------------------------");
}

void handleSerialCommands()
{
    if (!Serial.available())
        return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "HELP" ) 
    {
        printHelp();
    }
    else if (cmd == "LIST") 
    {
        listAllFiles();
    }
    else if (cmd.startsWith("READ ")) 
    {
        String path = cmd.substring(5);
        path.trim();
        path.toLowerCase();
        readFile(path);
    }
    else if (cmd.length() > 0)
    {
        Serial.println("ERR: UNKNOWN CMD - Type HELP for commands");
    }
}

#endif