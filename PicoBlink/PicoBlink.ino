#include <Arduino.h>
#include <I2S.h>
#include "hardware/structs/watchdog.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

// --- I2S Instance ---
I2S i2s(OUTPUT);

// --- Shared Volatile Variables (Cross-Core Communication) ---
volatile bool toneRunning = false;
volatile bool requestRestart = false;
volatile float currentFreq = 440.0;
volatile float phase = 0;
volatile float phaseIncrement = 0;
volatile int sampleRate = 44100;

volatile int numVoices = 1;      // Start with 1 voice
volatile float cpuLoad = 0.0;    // Percentage 0-100
uint32_t workTime = 0;           // Total micros spent calculating
uint32_t totalTime = 0;          // Total micros elapsed
                                 //
// Breathing LED variables
volatile bool breathingEnabled = true;
int fadeValue = 0;
int fadeDirection = 5;
unsigned long lastFadeUpdate = 0;

void updatePhaseIncrement() {
    // Formula: (2 * PI * Frequency) / SampleRate
    phaseIncrement = (2.0 * PI * currentFreq) / (float)sampleRate;
}

#include "hardware/adc.h"

float get_internal_temp() {
    // 1. Initialize the ADC hardware
    adc_init();
    
    // 2. Enable the internal temperature sensor
    adc_set_temp_sensor_enabled(true);
    
    // 3. Select ADC channel 4 (the internal temp sensor)
    adc_select_input(4);
    
    // 4. Read the raw value (12-bit: 0-4095)
    uint16_t raw = adc_read();
    
    // 5. Convert to Voltage
    const float conversion_factor = 3.3f / (1 << 12);
    float voltage = raw * conversion_factor;
    
    // 6. Convert Voltage to Degrees Celsius
    // Formula from RP2040 Datasheet: T = 27 - (Voltage - 0.706)/0.001721
    float tempC = 27.0f - (voltage - 0.706f) / 0.001721f;
    
    return tempC;
}

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
        else if (input.startsWith("i2s config ")) {
          // Expected format: i2s config <sr> <bits> <ch>
          // Example: i2s config 48000 16 2
          int s1 = input.indexOf(' ', 11);
          int s2 = input.indexOf(' ', s1 + 1);

          sampleRate = input.substring(11, s1).toInt();
          int bitWidth = input.substring(s1 + 1, s2).toInt();
          int channels = input.substring(s2 + 1).toInt();

          i2s.end();
          if (i2s.begin(sampleRate)) {
            // Manually calculate and force the hardware divisor for the new config
            // Total Bits per Frame = bitWidth * channels
            float target_div = (float)rp2040.f_cpu() / (sampleRate * bitWidth * channels * 2.0f);

            uint16_t div_int = (uint16_t)target_div;
            uint8_t div_frac = (uint8_t)((target_div - div_int) * 256.0f);
            pio0->sm[0].clkdiv = (div_int << 16) | (div_frac << 8);

            updatePhaseIncrement(); // Ensure the sine wave sounds correct at new SR

            Serial.printf("Configured: %dHz, %d-bit, %d-ch\n", sampleRate, bitWidth, channels);
            Serial.printf("New Hardware Divisor: %.4f\n", target_div);
          }
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
          uint32_t sys_clk = rp2040.f_cpu();
          uint32_t div_reg = pio0->sm[0].clkdiv;
          float pio_divider = (float)(div_reg >> 16) + (float)((div_reg & 0xFFFF) >> 8) / 256.0f;

          // Measure actual hardware speed
          float calc_bclk = (float)sys_clk / (pio_divider * 2.0f);

          // We can't know the bitwidth from the register, 
          // so we compare BCLK to LRCLK to "discover" it.
          float calc_lrclk = (float)sys_clk / (pio_divider * 2.0f * 32.0f); // Default 32-bit frame

          Serial.println("\n--- Real-Time Hardware Verification ---");
          Serial.printf("BCLK (Bit Clock):   %.3f MHz\n", calc_bclk / 1000000.0);
          Serial.printf("LRCLK (Sample Rate): %.2f Hz\n", calc_lrclk);

          // Discovery Logic: How many bits are in one sample period?
          float discovered_bits = calc_bclk / calc_lrclk;
          Serial.printf("Bits Per Frame:     %.0f\n", discovered_bits);

          if (abs(calc_lrclk - sampleRate) < 100) {
            Serial.println("Status: TIMING MATCHED");
          } else {
            Serial.println("Status: TIMING MISMATCH");
          }
        }
        else if (input.startsWith("i2s stress ")) {
          numVoices = input.substring(11).toInt();
          Serial.printf("Complexity increased to %d voices.\n", numVoices);
        }
        else if (input == "i2s status") {
          Serial.printf("Sample Rate: %d Hz\n", sampleRate);
          Serial.printf("CPU Load (Core 1): %.1f%%\n", cpuLoad * 100.0);
          if (cpuLoad > 0.95) Serial.println("WARNING: Core 1 is near SATURATION!");
        }
        else if (input.startsWith("reset to ")) {
            uint32_t mhz = input.substring(9).toInt();
            if (mhz >= 10 && mhz <= 250) {
                Serial.printf("Rebooting to %d MHz...\n", mhz);
                watchdog_hw->scratch[0] = mhz;
                delay(100);
                watchdog_reboot(0,0,0);
            }
        } else if (input == "temp") { 
            Serial.printf("Temp: %.0f\n", get_internal_temp());
        } else {
          Serial.print("Unknown Command\n");
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
    if (requestRestart) {
        i2s.end();
        i2s.begin(sampleRate);
        updatePhaseIncrement();
        requestRestart = false;
    }

    if (toneRunning) {
        uint32_t start_u = micros();

        while (i2s.availableForWrite()) {
            float mixedSample = 0;
            
            // --- The Stress Factor ---
            // Each voice adds a sin() calculation and a float addition
            for (int i = 0; i < numVoices; i++) {
                mixedSample += sin(phase * (i + 1)) * (1.0f / numVoices);
            }

            int16_t out = (int16_t)(mixedSample * 32767.0f);
            i2s.write(out); // Left
            i2s.write(out); // Right

            phase += phaseIncrement;
            if (phase >= 2.0 * PI) phase -= 2.0 * PI;
        }

        // Calculate CPU Load every 100ms
        uint32_t end_u = micros();
        workTime += (end_u - start_u);
        
        static uint32_t lastReport = 0;
        if (millis() - lastReport >= 100) {
            cpuLoad = (float)workTime / (1000.0f * 100); // work / 100ms
            workTime = 0;
            lastReport = millis();
        }
    }
}
void loop1_orig() {
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
