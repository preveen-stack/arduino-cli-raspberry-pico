#include <Arduino.h>
#include <I2S.h>
#include <hardware/watchdog.h>
#include <hardware/clocks.h>
#include <hardware/adc.h>

// I2S Instance
I2S i2s(OUTPUT);

// Global Variables
unsigned long lastBlink = 0;
int blinkFreq = 500;
bool blinkEnabled = true;
bool ledState = LOW;

// Tone Variables
bool toneRunning = false;
float currentFreq = 440.0;
float phase = 0;
float phaseIncrement = 0;
int sampleRate = 44100;

void updatePhaseIncrement() {
    phaseIncrement = (2.0 * PI * currentFreq) / (float)sampleRate;
}

void printHelp() {
    Serial.println("\n--- I2S Commands ---");
    Serial.println("i2s init <sr> <bw> <ch> - Init I2S (e.g., i2s init 44100 16 2)");
    Serial.println("i2s start               - Start I2S engine");
    Serial.println("i2s stop                - Stop I2S engine");
    Serial.println("i2s status              - Show I2S configuration");
    Serial.println("i2s tone <freq>         - Play sine wave (100-18000 Hz)");
    Serial.println("--------------------\n");
}

void handleI2STone() {
    if (!toneRunning) return;

    // Fill I2S buffer if space is available
    while (i2s.availableForWrite()) {
        int16_t sample = (int16_t)(sin(phase) * 32767.0f);
        i2s.write(sample); // Left channel
        i2s.write(sample); // Right channel (assuming 2 channels)
        
        phase += phaseIncrement;
        if (phase >= 2.0 * PI) phase -= 2.0 * PI;
    }
}

void measureI2S() {
    Serial.println("\n--- I2S Hardware Clock Measurement ---");
    
    // The compiler suggested CLOCKS_FC0_SRC_VALUE_CLKSRC_GPIN0
    // This maps the GPIO base to the frequency counter.
    float bclk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLKSRC_GPIN0 + 26); // GP26
    float lrclk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLKSRC_GPIN0 + 27); // GP27

    Serial.printf("BCLK (GP26): %.3f MHz\n", bclk / 1000.0);
    Serial.printf("LRCLK (GP27): %.2f Hz\n", lrclk * 1000.0);
    
    if (lrclk > 0) {
        float bitsPerFrame = (bclk * 1000.0) / (lrclk * 1000.0);
        Serial.printf("Detected Bits Per Frame: %.1f\n", bitsPerFrame);
    }
    Serial.println("---------------------------------------\n");
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    adc_init();
    adc_set_temp_sensor_enabled(true);
    delay(2000);
    Serial.println("Pico System Online. Type 'help' for commands.");
}

void loop() {
    // Blinking Logic
    if (blinkEnabled && (millis() - lastBlink >= (unsigned long)blinkFreq)) {
        lastBlink = millis();
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
    }

    // Handle I2S Audio Generation
    handleI2STone();

    // Command Parser
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        String lowerInput = input;
        lowerInput.toLowerCase();

        if (lowerInput == "help") printHelp();
        
        // I2S INIT: i2s init <sr> <bitwidth> <channels>
        else if (lowerInput.startsWith("i2s init ")) {
            int firstSpace = lowerInput.indexOf(' ', 9);
            int secondSpace = lowerInput.indexOf(' ', firstSpace + 1);
            
            sampleRate = lowerInput.substring(9, firstSpace).toInt();
            int bitWidth = lowerInput.substring(firstSpace + 1, secondSpace).toInt();
            int channels = lowerInput.substring(secondSpace + 1).toInt();

            // 1. Configure the pins
            i2s.setDATA(28); 
            i2s.setBCLK(26);
            // WS/LRCLK is automatically BCLK + 1 (GP27) unless set otherwise
            
            // 2. Set bit depth (The library expects this separately)
            i2s.setBitsPerSample(bitWidth);
            
            // 3. Start I2S with ONLY the sample rate
            if (i2s.begin(sampleRate)) {
                updatePhaseIncrement();
                Serial.printf("OK: I2S Init %dHz, %d-bit, %d ch\n", sampleRate, bitWidth, channels);
            } else {
                Serial.println("ERROR: I2S Init failed.");
            }
        }
        else if (lowerInput == "i2s start") {
            toneRunning = true;
            Serial.println("OK: I2S Tone Started");
        }
        else if (lowerInput == "i2s stop") {
            toneRunning = false;
            Serial.println("OK: I2S Tone Stopped");
        }
        else if (lowerInput == "i2s status") {
            Serial.printf("I2S Status: %s | Freq: %.1f Hz | SR: %d\n", 
                          toneRunning ? "RUNNING" : "STOPPED", currentFreq, sampleRate);
        }
        else if (lowerInput.startsWith("i2s tone ")) {
            float f = lowerInput.substring(9).toFloat();
            if (f >= 100 && f <= 18000) {
                currentFreq = f;
                updatePhaseIncrement();
                Serial.printf("OK: Frequency set to %.1f Hz\n", currentFreq);
            } else {
                Serial.println("ERROR: Range 100-18000 Hz");
            }
        }
        else if (lowerInput == "i2s measure") {
            if (!toneRunning) {
                Serial.println("Warning: I2S is stopped. Clocks may report 0Hz.");
            }
            measureI2S();
        }
        // ... (Include your previous clock/temp/reset/blink commands here)
    }
}
