#ifndef CONFIG_H
#define CONFIG_H

#define PCF1_ADDR       0x25    // Input PCF8574 buttons & sensors
#define PCF2_ADDR       0x26    // Output PCF8574 relays & leds & buzers

#define POLL_INTERVAL   20      // Polling interval in milliseconds

#define MAX_START_FILES 100     // Keep last 100 boot session files

#define BTN_AUTO        7       // AUTO button
#define BTN_REJECT      6       // REJECT/E-STOP button
#define SLOT1_A         5       // Slot 1 Sensor A (front)
#define SLOT1_B         4       // Slot 1 Sensor B (back)
#define SLOT2_A         3       // Slot 2 Sensor A (front)
#define SLOT2_B         2       // Slot 2 Sensor B (back)
#define SLOT3_A         1       // Slot 3 Sensor A (front)
#define SLOT3_B         0       // Slot 3 Sensor B (back)

#define RELAY_AUTO      0       // AUTO relay to PLC
#define LED_MACHINE_ON  1       // GREEN LED - Machine running indicator (renamed from LED_REJECT)
#define BUZZER_LED      2       // RED LED + Buzzer for reject alert and sensor beeps

// Beep duration constant
#define BEEP_DURATION   500     // All beeps are 500ms


// struct to hold machine state and counts
struct MachineState
{
    uint32_t reject_count;      // Total rejected parts (current session only)
    bool machine_mode;          // true if waiting for part in bin
};

uint8_t prev_inputs = 0x00;         // Previous normalized input state
uint8_t active_slot = 0;            // Local slot tracking not saved
MachineState state;                 // Current machine state
bool machine_status = false;        // true when AUTO relay is ON
uint32_t last_poll_time = 0;        // For polling timing

// Beep timing variables (replaces old buzzer blink variables)
bool beep_active = false;           // Is a beep currently playing?
unsigned long beep_start_time = 0;  // When did the beep start?

// Boot tracking variables
uint32_t current_boot_number = 0;   // Current boot/restart number
uint32_t current_session_count = 0; // Reject count for current boot session
uint32_t total_lifetime_count = 0;  // Total rejects across all boots

// Monitoring state variables
bool monitoring_active = false;
unsigned long monitoring_start_time = 0;
uint8_t monitored_slot = 0;
bool part_count_incremented = false;  // Track if count was already incremented for this part
uint8_t monitoring_last_sensor = 0;   // Track last sensor triggered: 1=SensorA, 2=SensorB (for B→A detection)

#define MONITORING_DURATION 5000  // 5 seconds monitoring time after A→B

// SPIFFS version tracking - increment when file structure changes in firmware updates
#define SPIFFS_VERSION 1

void auto_button_handler();
void reject_button_handler();
void reject_handler(uint8_t edge);
void process_inputs(uint8_t edge);

#endif