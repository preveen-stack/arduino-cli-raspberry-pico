#include <Arduino.h>
#include <I2S.h>
#include "hardware/structs/watchdog.h"
#include "hardware/pwm.h"

// --- I2S Instance ---
I2S i2s(OUTPUT);

// --- Shared Volatile Variables (Cross-Core Communication) ---
volatile bool toneRunning = false;
volatile bool requestRestart = false;
volatile float currentFreq = 440.0;
volatile float phase = 0;
volatile float phaseIncrement = 0;
volatile int sampleRate = 44100;

// Breathing LED variables
volatile bool breathingEnabled = true;
int fadeValue = 0;
int fadeDirection = 5;
unsigned long lastFadeUpdate = 0;

// ==========================================
// CORE 0: Serial Watch & Command Interface
// ==========================================
void setup() {
    // 1. Restore saved clock speed from watchdog scratch register
    uint32_t saved_freq = watchdog_hw->scratch[0];
    if (saved_freq >= 10 && saved_freq <= 250) {
        set_sys_clock_khz(saved_freq * 1000, true);
    }

    Serial.begin(115200);
    while(!Serial && millis() < 3000); 
    
    Serial.println("\n====================================");
    Serial.printf(" PICO DUAL-CORE ONLINE (%lu MHz)\n", rp2040.f_cpu()/1000000);
    Serial.println("====================================");
    Serial.println("Type 'help' for commands.");
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input == "help") {
            Serial.println("\n--- Commands ---");
            Serial.println("i2s start/stop  - Toggle Audio");
            Serial.println("i2s sniff       - Read internal PIO registers");
            Serial.println("i2s tone <f>    - Change sine frequency");
            Serial.println("reset to <mhz>  - Reboot at specific speed");
            Serial.println("clock           - Show current SysClk");
        } 
        else if (input == "clock") {
            Serial.printf("Current Clock: %.2f MHz\n", rp2040.f_cpu()/1000000.0);
        }
        else if (input == "i2s start") {
          i2s.begin(sampleRate); 

          // --- MANUAL HARDWARE OVERRIDE ---
          // Calculate divisor: SysClk / (SampleRate * Bits * Channels * 2)
          // For 125MHz, 44.1kHz, 16-bit, Stereo: 125,000,000 / (44100 * 32 * 2) = 44.2885
          float target_div = (float)rp2040.f_cpu() / (sampleRate * 32.0f * 2.0f);

          // Convert float to PIO hardware format (16-bit integer, 8-bit fractional)
          uint16_t div_int = (uint16_t)target_div;
          uint8_t div_frac = (uint8_t)((target_div - div_int) * 256.0f);
          uint32_t final_reg_val = (div_int << 16) | (div_frac << 8);

          // Write directly to PIO0 State Machine 0 clock divisor register
          pio0->sm[0].clkdiv = final_reg_val;
          // --------------------------------

          toneRunning = true;
          Serial.printf("Audio Started. Forced Divisor: %.4f (Reg: 0x%08X)\n", target_div, final_reg_val);
        }
        else if (input == "i2s stop") {
            toneRunning = false;
            Serial.println("Audio Stopped");
        }
        else if (input.startsWith("i2s tone ")) {
            currentFreq = input.substring(9).toFloat();
            phaseIncrement = (2.0 * PI * currentFreq) / (float)sampleRate;
            Serial.printf("Tone: %.1f Hz\n", currentFreq);
        }
        else if (input == "i2s sniff") {
            Serial.println("\n--- PIO Internal Register Sniff ---");
            uint32_t sys_clk = rp2040.f_cpu();
            uint32_t div_reg = pio0->sm[0].clkdiv;
            float pio_divider = (float)(div_reg >> 16) + (float)((div_reg & 0xFFFF) >> 8) / 256.0f;
            
            if (pio_divider < 1.0f) pio_divider = 1.0f;

            float calc_bclk = (float)sys_clk / (pio_divider * 2.0f);
            float calc_lrclk = calc_bclk / 32.0f; 

            Serial.printf("Hardware Divisor: %.4f\n", pio_divider);
            Serial.printf("Measured BCLK:    %.3f MHz\n", calc_bclk / 1000000.0);
            Serial.printf("Measured LRCLK:   %.2f Hz\n", calc_lrclk);
            Serial.println("------------------------------------");
        }
        else if (input.startsWith("reset to ")) {
            uint32_t mhz = input.substring(9).toInt();
            if (mhz >= 10 && mhz <= 250) {
                Serial.printf("Rebooting to %d MHz...\n", mhz);
                watchdog_hw->scratch[0] = mhz;
                delay(100);
                watchdog_reboot(0,0,0);
            }
        }
    }
}

// ==========================================
// CORE 1: Real-Time Audio & Breathing LED
// ==========================================
void setup1() {
    i2s.setDATA(28);
    i2s.setBCLK(26);
    i2s.begin(sampleRate);
    
    pinMode(LED_BUILTIN, OUTPUT);
    phaseIncrement = (2.0 * PI * currentFreq) / (float)sampleRate;
}

void loop1() {
    // 1. Check for clock change requests from Core 0
    if (requestRestart) {
        i2s.end();
        i2s.begin(sampleRate);
        phaseIncrement = (2.0 * PI * currentFreq) / (float)sampleRate;
        requestRestart = false;
    }

    // 2. High-Priority Audio Filling
    if (toneRunning) {
        while (i2s.availableForWrite()) {
            int16_t sample = (int16_t)(sin(phase) * 32767.0f);
            i2s.write(sample); // Left
            i2s.write(sample); // Right
            
            phase += phaseIncrement;
            if (phase >= 2.0 * PI) phase -= 2.0 * PI;
        }
    }

    // 3. Breathing LED Logic (Runs every 20ms)
    if (breathingEnabled && (millis() - lastFadeUpdate >= 20)) {
        lastFadeUpdate = millis();
        analogWrite(LED_BUILTIN, fadeValue);
        
        fadeValue += fadeDirection;
        if (fadeValue <= 0 || fadeValue >= 255) {
            fadeDirection = -fadeDirection;
        }
    }
}
