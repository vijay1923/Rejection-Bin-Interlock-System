#line 1 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\config.h"
#ifndef CONFIG_H
#define CONFIG_H


#define PCF1_ADDR       0x25    // Input PCF8574 buttons & sensors
#define PCF2_ADDR       0x26    // Output PCF8574 relays & leds & buzers
#define POLL_INTERVAL   20      // Polling interval in milliseconds
#define BUZZER_INTERVAL 500     // Buzzer blink/beep interval in milliseconds

// Input Pin Mapping PCF1
#define BTN_AUTO        0       // AUTO button
#define BTN_REJECT      1       // REJECT/E-STOP button
#define SLOT1_A         2       // Slot 1 Sensor A (front)
#define SLOT1_B         3       // Slot 1 Sensor B (back)
#define SLOT2_A         4       // Slot 2 Sensor A (front)
#define SLOT2_B         5       // Slot 2 Sensor B (back)
#define SLOT3_A         6       // Slot 3 Sensor A (front)
#define SLOT3_B         7       // Slot 3 Sensor B (back)

// Output Pin Mapping PCF2
#define RELAY_AUTO      0       // AUTO relay to PLC
#define LED_REJECT      1       // REJECT mode indicator
#define OUT_PULSE       2       // Confirmation pulse output
#define BUZZER_LED      7       // Buzzer/LED for reject alert (blinks & beeps)

// #define EEPROM_SIZE     512     // allocate memory in flash 
// #define EEPROM_ADDR     0       // starting memory address

struct MachineState    /// machine structer 
{
    uint32_t reject_count;      // Total rejected parts
    bool machine_mode;           // true if waiting for part in bin

};

uint8_t prev_inputs = 0x00;     // Previous normalized input state
uint8_t active_slot = 0;        // Local slot tracking not saved
MachineState state;             // Current machine state
bool machine_status = false;   // true when AUTO relay is ON
uint32_t last_poll_time = 0;    // For polling timing
uint32_t last_buzzer_toggle = 0; // For buzzer blink timing
bool buzzer_state = false;      // Current buzzer on/off state


// Function declarations
void auto_button_handler();
void reject_button_handler();
void reject_handler(uint8_t edge);
void process_inputs(uint8_t edge);  
#endif 