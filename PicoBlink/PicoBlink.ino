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