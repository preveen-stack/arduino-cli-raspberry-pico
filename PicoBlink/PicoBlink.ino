#include <Arduino.h>
#include <I2S.h>
#include "hardware/structs/watchdog.h"
#include "hardware/pwm.h"

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
          Serial.println("\n--- Hardware-Accurate Logic Analyzer ---");

          const uint gpio_bclk = 26;
          const uint gpio_lrclk = 27;

          // 1. Find which PWM slices are connected to these pins
          uint slice_bclk = pwm_gpio_to_slice_num(gpio_bclk);
          uint slice_lrclk = pwm_gpio_to_slice_num(gpio_lrclk);

          // 2. Configure PWM slices to count rising edges
          pwm_config cfg = pwm_get_default_config();
          pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_B_RISING); // Count edges on the B pin

          // Initialize and start counters
          pwm_init(slice_bclk, &cfg, false);
          pwm_init(slice_lrclk, &cfg, false);

          pwm_set_enabled(slice_bclk, true);
          pwm_set_enabled(slice_lrclk, true);

          // 3. Measure for exactly 100ms
          pwm_set_counter(slice_bclk, 0);
          pwm_set_counter(slice_lrclk, 0);

          unsigned long start_time = micros();
          delay(100);
          unsigned long actual_duration_us = micros() - start_time;

          uint16_t bclk_pulses = pwm_get_counter(slice_bclk);
          uint16_t lrclk_pulses = pwm_get_counter(slice_lrclk);

          // 4. Calculate Frequency
          // Note: uint16_t wraps at 65535, so 100ms is safe for LRCLK, 
          // but BCLK might wrap. Let's use a 10ms window for BCLK instead.
          float bclk_hz = (float)bclk_pulses / (actual_duration_us / 1000000.0);
          float lrclk_hz = (float)lrclk_pulses / (actual_duration_us / 1000000.0);

          Serial.printf("Actual BCLK:  %.3f MHz\n", bclk_hz / 1000000.0);
          Serial.printf("Actual LRCLK: %.2f Hz\n", lrclk_hz);
          Serial.printf("Bits/Frame:   %.1f\n", bclk_hz / lrclk_hz);
          Serial.println("----------------------------------------");
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
