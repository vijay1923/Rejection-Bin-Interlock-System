#ifndef CONFIG_H
#define CONFIG_H

#define PCF1_ADDR       0x25    // Input PCF8574 buttons & sensors
#define PCF2_ADDR       0x26    // Output PCF8574 relays & leds & buzers

#define POLL_INTERVAL   20      // Polling interval in milliseconds
#define I2C_RETRY_COUNT 3       // Retry count per I2C read/write transaction
#define I2C_FAILSAFE_THRESHOLD 5 // Consecutive I2C failures before fail-safe lock
#define BUTTON_DEBOUNCE_MS 120   // Minimum interval between valid button presses

#define MAX_START_FILES 100     // Keep last 100 boot session files


/////////////// Input pin definitions ///////////////
#define BTN_AUTO        7       // AUTO button
#define BTN_REJECT      6       // REJECT/E-STOP button
#define SLOT1_A         5       // Slot 1 Sensor A (front)
#define SLOT1_B         4       // Slot 1 Sensor B (back)
#define SLOT2_A         3       // Slot 2 Sensor A (front)
#define SLOT2_B         2       // Slot 2 Sensor B (back)
#define SLOT3_A         1       // Slot 3 Sensor A (front)
#define SLOT3_B         0       // Slot 3 Sensor B (back)

/////////////// Output pin definitions //////////////
#define RELAY_AUTO      0       // AUTO relay to PLC
#define LED_MACHINE_ON  1       // GREEN LED - Machine running indicator
#define BUZZER_LED      2       // RED LED + Buzzer for reject alert and sensor beeps


// EEPROM memory definitions
#define EEPROM_SIZE           512     // Total EEPROM size to allocate
#define EEPROM_MAGIC          0xABCD  // Magic number to detect first boot
#define ADDR_MAGIC            0       // Address 0-1: Magic number (2 bytes)
#define ADDR_MACHINE_MODE     2       // Address 2: Machine mode (1 byte)
#define ADDR_LIFETIME_COUNT   3       // Address 3-6: Lifetime count (4 bytes)
#define ADDR_BOOT_NUMBER      7       // Address 7-10: Boot number (4 bytes)
#define ADDR_FW_VERSION       11      // Address 11-12: Firmware version (2 bytes)

// ── Firmware version ──────────────────────────────────────────────────────────
// Bump this number when uploading new firmware that should start fresh.
// Both EEPROM (all counters/mode) and SPIFFS (all session files) will be wiped.
#define FIRMWARE_VERSION      4

// Beep duration constant
#define BEEP_DURATION   500     // All beeps are 500ms

// struct to hold machine state and counts
struct MachineState
{
    uint32_t reject_count;      // Total rejected parts (for display)
    bool machine_mode;          // true = REJECT mode, false = AUTO mode
};

uint8_t prev_inputs = 0x00;         // Previous normalized input state
uint8_t active_slot = 0;            // Currently active slot (1, 2, or 3)
MachineState state;                 // Current machine state
bool machine_status = false;        // true when AUTO relay is ON
uint32_t last_poll_time = 0;        // For polling timing

// Beep timing variables
bool beep_active = false;           // Is a beep currently playing?
unsigned long beep_start_time = 0;  // When did the beep start?

// Boot tracking variables
uint32_t current_boot_number = 0;   // Current boot/restart number
uint32_t current_session_count = 0; // Reject count for current boot session
uint32_t total_lifetime_count = 0;  // Total rejects across all boots

// Continuous monitoring state variables
bool monitoring_active = false;     // Is continuous monitoring active?
uint8_t monitored_slot = 0;         // Which slot is being monitored (1, 2, or 3)
uint8_t monitoring_last_sensor = 0; // Last sensor triggered: 1=SensorA, 2=SensorB
bool part_counted = false;          // Has this part been counted yet?

// I2C health monitoring
uint16_t i2c_consecutive_failures = 0; // Consecutive failed I2C operations
bool i2c_failsafe_latched = false;     // True after I2C fail-safe lock is triggered

// Runtime diagnostics counters
uint32_t i2c_total_failures = 0;          // Total failed I2C operations since boot
uint32_t i2c_failsafe_trigger_count = 0;  // Number of fail-safe triggers since boot
uint32_t debounce_filtered_count = 0;     // Number of filtered button bounce events
uint32_t auto_button_valid_count = 0;     // Valid AUTO button press count
uint32_t reject_button_valid_count = 0;   // Valid REJECT button press count
unsigned long last_auto_press_ms = 0;     // Last accepted AUTO button timestamp
unsigned long last_reject_press_ms = 0;   // Last accepted REJECT button timestamp


// Forward declarations
void auto_button_handler();
void reject_button_handler();
void reject_handler(uint8_t edge);
void process_inputs(uint8_t edge);
void increment_part_count();
void stop_monitoring();

#endif