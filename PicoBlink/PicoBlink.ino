#include <Arduino.h>
#include <I2S.h>
#include "hardware/structs/watchdog.h"

// I2S on Core 1
I2S i2s(OUTPUT);

// Shared Global Variables (Thread-safe-ish for simple types)
volatile bool toneRunning = false;
volatile float currentFreq = 440.0;
volatile float phase = 0;
volatile float phaseIncrement = 0;
int sampleRate = 44100;

// ==========================================
// CORE 0: Serial Watch & Command Interface
// ==========================================
void setup() {
    // Check for saved clock speed from reset
    uint32_t saved_freq = watchdog_hw->scratch[0];
    if (saved_freq >= 10 && saved_freq <= 250) {
        set_sys_clock_khz(saved_freq * 1000, true);
    }

    Serial.begin(115200);
    // Wait for serial to be ready so we don't miss the first messages
    while(!Serial && millis() < 3000); 
    
    Serial.println("\n[CORE 0] Serial Monitor Active");
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input == "help") {
            Serial.println("Commands: i2s start, i2s stop, i2s tone <f>, reset to <mhz>");
        } 
        else if (input == "i2s start") {
            toneRunning = true;
            Serial.println("Audio Engine Triggered on Core 1");
        }
        else if (input == "i2s stop") {
            toneRunning = false;
        }
        else if (input.startsWith("i2s tone ")) {
            currentFreq = input.substring(9).toFloat();
            phaseIncrement = (2.0 * PI * currentFreq) / (float)sampleRate;
            Serial.printf("Frequency set to %.1f Hz\n", currentFreq);
        }
        else if (input == "i2s sniff") {
          Serial.println("\n--- Virtual Logic Analyzer ---");

          gpio_set_input_enabled(26, true);
          gpio_set_input_enabled(27, true);

          const uint32_t sample_ms = 200; // Longer sample for better precision
          uint32_t bclk_count = 0, lrclk_count = 0;
          bool last_bclk = digitalRead(26), last_lrclk = digitalRead(27);

          unsigned long start = micros(); // Use micros for timing accuracy
          unsigned long end_time = start + (sample_ms * 1000);

          while (micros() < end_time) {
            bool b = digitalRead(26);
            bool l = digitalRead(27);
            if (b != last_bclk) { bclk_count++; last_bclk = b; }
            if (l != last_lrclk) { lrclk_count++; last_lrclk = l; }
          }
          unsigned long actual_duration_us = micros() - start;

          // Math: (Transitions / 2) / (seconds)
          float bclk_hz = (bclk_count / 2.0) / (actual_duration_us / 1000000.0);
          float lrclk_hz = (lrclk_count / 2.0) / (actual_duration_us / 1000000.0);

          Serial.printf("BCLK (Bit Clock):   %.2f kHz\n", bclk_hz / 1000.0);
          Serial.printf("LRCLK (Sample Rate): %.2f Hz\n", lrclk_hz);
          Serial.printf("Bits per Frame:     %.1f\n", bclk_hz / lrclk_hz);
          Serial.println("------------------------------");
        }
        else if (input.startsWith("reset to ")) {
            uint32_t mhz = input.substring(9).toInt();
            watchdog_hw->scratch[0] = mhz;
            watchdog_reboot(0,0,0);
        }
    }
}

// ==========================================
// CORE 1: High-Priority Audio & Blink
// ==========================================
void setup1() {
    // Core 1 setup runs slightly after Core 0
    i2s.setDATA(28);
    i2s.setBCLK(26);
    i2s.begin(sampleRate);
    
    pinMode(LED_BUILTIN, OUTPUT);
    
    // Initial calculation
    phaseIncrement = (2.0 * PI * currentFreq) / (float)sampleRate;
}

void loop1() {
    // 1. Audio Generation (Hard Real-Time)
    if (toneRunning) {
        while (i2s.availableForWrite()) {
            int16_t sample = (int16_t)(sin(phase) * 32767.0f);
            i2s.write(sample);
            i2s.write(sample);
            
            phase += phaseIncrement;
            if (phase >= 2.0 * PI) phase -= 2.0 * PI;
        }
    }

    // 2. Heartbeat LED (Moved here to ensure it doesn't block Serial)
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink >= 500) {
        lastBlink = millis();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
}
