#include <Arduino.h>
#include <I2S.h>
#include "hardware/structs/watchdog.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/pio.h"

// --- I2S Instance ---
I2S i2s(OUTPUT);

// --- Shared Volatile Variables ---
volatile bool toneRunning = false;
volatile bool requestRestart = false;
volatile float currentFreq = 440.0;
volatile float phase = 0;
volatile float phaseIncrement = 0;
volatile int sampleRate = 44100;
volatile int numVoices = 1;
volatile float cpuLoad = 0.0;
uint32_t workTime = 0;

// --- DMA Sniffer Globals ---
#define CAPTURE_SAMPLES 1024
uint32_t dma_results[CAPTURE_SAMPLES];
int dma_chan = -1;
uint pio_sm_capture = 0;
PIO capture_pio = pio1; 
bool snifferInitialized = false;

// --- PIO Program: Measures cycles between rising edges ---
const uint16_t capture_program_instructions[] = {
    0xa02b, // 0: mov x, !null     ; Reset X (32-bit max)
    0x2020, // 1: wait 0 pin 0     ; Wait for pin to be LOW
    0x20a0, // 2: wait 1 pin 0     ; Wait for rising edge (Start)
    0x0044, // 3: jmp x-- 4        ; Decrement X (Loop start)
    0x2020, // 4: wait 0 pin 0     ; Wait for pin to be LOW
    0x00c7, // 5: jmp pin 7        ; If pin is HIGH, we found next edge
    0x00c5, // 6: jmp 5            ; Keep waiting for HIGH
    0x20a0, // 7: wait 1 pin 0     ; Confirm rising edge
    0x8020, // 8: push block       ; Push remaining X value to FIFO
};

const struct pio_program capture_program = {
    .instructions = capture_program_instructions,
    .length = 9,
    .origin = -1,
};

void updatePhaseIncrement() {
    phaseIncrement = (2.0 * PI * currentFreq) / (float)sampleRate;
}

float get_internal_temp() {
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
    uint16_t raw = adc_read();
    const float conversion_factor = 3.3f / (1 << 12);
    float voltage = raw * conversion_factor;
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

void setup_dma_sniffer(uint pin) {
    if (snifferInitialized) return;

    uint offset = pio_add_program(capture_pio, &capture_program);
    pio_gpio_init(capture_pio, pin);

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_in_pins(&c, pin);
    sm_config_set_jmp_pin(&c, pin);
    sm_config_set_clkdiv(&c, 1.0f); 

    pio_sm_init(capture_pio, pio_sm_capture, offset, &c);
    pio_sm_set_enabled(capture_pio, pio_sm_capture, true);

    dma_chan = dma_claim_unused_channel(false); // Safety: don't panic if busy
    if (dma_chan >= 0) {
        dma_channel_config dc = dma_channel_get_default_config(dma_chan);
        channel_config_set_transfer_data_size(&dc, DMA_SIZE_32);
        channel_config_set_read_increment(&dc, false);
        channel_config_set_write_increment(&dc, true);
        channel_config_set_dreq(&dc, pio_get_dreq(capture_pio, pio_sm_capture, false));

        dma_channel_configure(dma_chan, &dc, dma_results, &capture_pio->rxf[pio_sm_capture], CAPTURE_SAMPLES, false);
        snifferInitialized = true;
    }
}

// ==========================================
// CORE 0: Commands & Diagnostics
// ==========================================
void setup() {
    uint32_t saved_freq = watchdog_hw->scratch[0];
    if (saved_freq >= 10 && saved_freq <= 250) {
        set_sys_clock_khz(saved_freq * 1000, true);
    }
    Serial.begin(115200);
    while(!Serial && millis() < 2000); 
    
    Serial.println("\n====================================");
    Serial.printf(" PICO DUAL-CORE ONLINE (%lu MHz)\n", rp2040.f_cpu()/1000000);
    Serial.println("====================================");
    Serial.println("Ready. Use 'i2s start' then 'i2s dmasniff'.");
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input == "clock") {
            Serial.printf("Clock: %.2f MHz\n", rp2040.f_cpu()/1000000.0);
        }
        else if (input.startsWith("i2s config ")) {
            int s1 = input.indexOf(' ', 11);
            int s2 = input.indexOf(' ', s1 + 1);
            sampleRate = input.substring(11, s1).toInt();
            int bitWidth = input.substring(s1 + 1, s2).toInt();
            int channels = input.substring(s2 + 1).toInt();
            i2s.end();
            if (i2s.begin(sampleRate)) {
                float target_div = (float)rp2040.f_cpu() / (sampleRate * bitWidth * channels * 2.0f);
                uint16_t div_int = (uint16_t)target_div;
                uint8_t div_frac = (uint8_t)((target_div - div_int) * 256.0f);
                pio0->sm[0].clkdiv = (div_int << 16) | (div_frac << 8);
                updatePhaseIncrement();
                Serial.printf("Configured: %dHz\n", sampleRate);
            }
        }
        else if (input == "i2s start") {
            i2s.begin(sampleRate); 
            float target_div = (float)rp2040.f_cpu() / (sampleRate * 32.0f * 2.0f);
            uint16_t div_int = (uint16_t)target_div;
            uint8_t div_frac = (uint8_t)((target_div - div_int) * 256.0f);
            pio0->sm[0].clkdiv = (div_int << 16) | (div_frac << 8);
            toneRunning = true;
            Serial.println("Audio Started.");
        }
        else if (input == "i2s stop") {
            toneRunning = false;
            Serial.println("Audio Stopped.");
        }
        else if (input == "i2s dmasniff") {
            if (!toneRunning) {
                Serial.println("Error: Start Audio first.");
                return;
            }
            
            // Initialize hardware only on first request to prevent boot hangs
            if (!snifferInitialized) setup_dma_sniffer(26);

            if (dma_chan < 0) {
                Serial.println("DMA Error: Could not claim channel.");
                return;
            }

            dma_channel_set_write_addr(dma_chan, dma_results, true);
            Serial.print("Sniffing BCLK...");
            
            uint32_t start_wait = millis();
            while(dma_channel_is_busy(dma_chan)) {
                if(millis() - start_wait > 500) {
                    dma_channel_abort(dma_chan);
                    Serial.println(" TIMEOUT - No Signal Found.");
                    return;
                }
            }
            Serial.println(" Done.");

            double total_cycles = 0;
            for (int i = 1; i < CAPTURE_SAMPLES; i++) {
                total_cycles += (dma_results[i-1] - dma_results[i]);
            }
            double avg_cycles = total_cycles / (CAPTURE_SAMPLES - 1);
            double measured_bclk = (double)rp2040.f_cpu() / avg_cycles;
            Serial.printf("--- RESULTS ---\nBCLK: %.3f MHz\nLRCLK: %.1f Hz\nCycles/Bit: %.2f\n", 
                          measured_bclk/1000000.0, measured_bclk/32.0, avg_cycles);
        }
        else if (input.startsWith("i2s stress ")) {
            numVoices = input.substring(11).toInt();
            Serial.printf("Voices: %d\n", numVoices);
        }
        else if (input == "i2s status") {
            Serial.printf("SR: %d | Load: %.1f%% | Voices: %d\n", sampleRate, cpuLoad * 100.0, numVoices);
        }
        else if (input.startsWith("reset to ")) {
            uint32_t mhz = input.substring(9).toInt();
            watchdog_hw->scratch[0] = mhz;
            watchdog_reboot(0,0,0);
        } 
        else if (input == "temp") {
            Serial.printf("Temp: %.1f C\n", get_internal_temp());
        }
    }
}

// ==========================================
// CORE 1: High-Speed Audio Engine
// ==========================================
void setup1() {
    i2s.setDATA(28);
    i2s.setBCLK(26);
    // Don't call sniffer here - it causes contention at boot
    updatePhaseIncrement();
}

void loop1() {
    if (toneRunning) {
        uint32_t start_u = micros();
        while (i2s.availableForWrite()) {
            float mixedSample = 0;
            for (int i = 0; i < numVoices; i++) {
                // Optimization: using sinf for 32-bit floats
                mixedSample += sinf(phase * (i + 1)) * (1.0f / numVoices);
            }
            int16_t out = (int16_t)(mixedSample * 32767.0f);
            i2s.write(out); // Left
            i2s.write(out); // Right
            phase += phaseIncrement;
            if (phase >= 2.0 * PI) phase -= 2.0 * PI;
        }
        uint32_t end_u = micros();
        workTime += (end_u - start_u);
        
        static uint32_t lastReport = 0;
        if (millis() - lastReport >= 100) {
            cpuLoad = (float)workTime / 100000.0f;
            workTime = 0;
            lastReport = millis();
        }
    } else {
        delay(1); // Relax when idle
    }
}