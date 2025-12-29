#line 1 "C:\\Users\\Shree\\Documents\\Arduino\\Embedsol\\r_bin_interlock_sys\\io_operations.h"
#ifndef IO_OPERATIONS_H
#define IO_OPERATIONS_H
#include "config.h"


uint8_t read_inputs() 
{
    Wire.requestFrom(PCF1_ADDR, 1);
    if (Wire.available()) 
    {
        uint8_t raw = Wire.read();
        return ~raw;  // Invert (active LOW → active HIGH)
    }
    return 0x00;
}

void write_outputs(uint8_t data) 
{
    Wire.beginTransmission(PCF2_ADDR);
    Wire.write(data);
    Wire.endTransmission();
}

void update_outputs()
{
    uint8_t out = 0x00;
    
    // Set AUTO relay
    if (machine_status) 
    {
        out |= (1 << RELAY_AUTO);
    }
    
    // Set REJECT LED
    if (state.machine_mode) 
    {
        out |= (1 << LED_REJECT);
    }
    
    // Blink buzzer/LED in reject mode
    if (state.machine_mode) 
    {
        if (millis() - last_buzzer_toggle >= BUZZER_INTERVAL)
        {
            last_buzzer_toggle = millis();
            buzzer_state = !buzzer_state;
        }
        
        if (buzzer_state)
        {
            out |= (1 << BUZZER_LED);
        }
    }
    else
    {
        buzzer_state = false;  // Turn off when not in reject mode
    }
    
    write_outputs(out);
}

void relay_output(bool on) 
{
    machine_status = on;
    update_outputs();
}

void pulse_output(uint8_t pin, uint16_t duration_ms) 
{
    uint8_t out = 0x00;
    
    if (machine_status) out |= (1 << RELAY_AUTO);
    if (state.machine_mode) 
    {
        out |= (1 << LED_REJECT);
        if (buzzer_state) out |= (1 << BUZZER_LED);
    }
    
    out |= (1 << pin);
    write_outputs(out);
    delay(duration_ms);
    
    out &= ~(1 << pin);
    write_outputs(out);
}


#endif 