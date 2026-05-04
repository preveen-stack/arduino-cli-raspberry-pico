#include <Arduino.h>
#include <I2S.h>

#ifdef __cplusplus
extern "C" {
#endif
  #include <hardware/watchdog.h>
  #include <hardware/clocks.h>
  #include <hardware/adc.h>
  #include <hardware/gpio.h>
#ifdef __cplusplus
}
#endif

I2S i2s(OUTPUT);

// Global Variables
unsigned long lastBlink = 0;
int blinkFreq = 500; 
bool blinkEnabled = true;
bool ledState = LOW;

// Audio Variables
bool toneRunning = false;
float currentFreq = 440.0;
float phase = 0;
float phaseIncrement = 0;
int sampleRate = 44100;
int bitDepth = 16;

void updatePhaseIncrement() {
  phaseIncrement = (2.0 * PI * currentFreq) / (float)sampleRate;
}

void printHelp() {
  Serial.println("\n========= PICO CONTROL MENU =========");
  Serial.println("help             - Show this menu");
  Serial.println("clock            - Show system clock speeds");
  Serial.println("temp             - Read internal CPU temp");
  Serial.println("blink on/off     - Toggle onboard LED");
  Serial.println("blink freq <ms>  - Set LED rate (1-10000)");
  Serial.println("led measure      - Measure LED pulse frequency");
  Serial.println("reset            - Reboot the Pico");
  Serial.println("\n--- I2S Commands ---");
  Serial.println("i2s init <sr> <bw> <ch>");
  Serial.println("i2s start/stop | i2s measure | i2s detail");
  Serial.println("=====================================\n");
}

// New Function: Measure LED Frequency
void measureLED() {
  Serial.println("\n--- LED Frequency Measurement (GP25) ---");
  
  if (!blinkEnabled) {
    Serial.println("Error: LED blinking is OFF. Cannot measure frequency.");
    return;
  }

  // Calculate theoretical frequency based on the blinkFreq (interval)
  // Frequency = 1 / (Period in seconds). Period = blinkFreq * 2 (on + off)
  float theoreticalFreq = 1000.0 / (blinkFreq * 2.0);

  // Hardware measurement attempt
  // Note: frequency_count_khz is for high speeds. 
  // For low speeds, we count pulses over 1 second.
  unsigned long start = millis();
  int pulses = 0;
  bool lastState = digitalRead(LED_BUILTIN);

  Serial.println("Measuring for 2 seconds...");
  while (millis() - start < 2000) {
    bool currentState = digitalRead(LED_BUILTIN);
    if (currentState != lastState) {
      if (currentState == HIGH) pulses++;
      lastState = currentState;
    }
  }

  float measuredFreq = pulses / 2.0;

  Serial.printf("Theoretical Freq: %.3f Hz\n", theoreticalFreq);
  Serial.printf("Measured Freq:    %.3f Hz\n", measuredFreq);
  Serial.println("----------------------------------------\n");
}

void measureI2S() {
  Serial.println("\n--- I2S Hardware Clock Measurement ---");
  uint32_t bclk_khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLKSRC_GPIN0 + 26);
  uint32_t lrclk_khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLKSRC_GPIN0 + 27);
  Serial.printf("BCLK (GP26):  %.3f MHz\n", bclk_khz / 1000.0);
  Serial.printf("LRCLK (GP27): %u Hz\n", lrclk_khz * 1000);
}

void handleI2STone() {
  if (!toneRunning) return;
  while (i2s.availableForWrite()) {
    int16_t sample = (int16_t)(sin(phase) * 32767.0f);
    i2s.write(sample);
    i2s.write(sample);
    phase += phaseIncrement;
    if (phase >= 2.0 * PI) phase -= 2.0 * PI;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  adc_init();
  adc_set_temp_sensor_enabled(true);
  delay(2000);
  Serial.println("Pico Online. Type 'help' to begin.");
}

void loop() {
  if (blinkEnabled && (millis() - lastBlink >= (unsigned long)blinkFreq)) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }

  handleI2STone();

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    String cmd = input;
    cmd.toLowerCase();

    if (cmd == "help") printHelp();
    else if (cmd == "clock") {
       Serial.printf("\nSystem: %.2f MHz\n", rp2040.f_cpu() / 1000000.0);
    }
    else if (cmd == "temp") {
      adc_select_input(4);
      uint16_t raw = adc_read();
      float temp = 27.0f - ((raw * (3.3f / (1 << 12))) - 0.706f) / 0.001721f;
      Serial.printf("CPU Temp: %.2f °C\n", temp);
    }
    else if (cmd == "led measure") measureLED();
    else if (cmd == "blink on") blinkEnabled = true;
    else if (cmd == "blink off") blinkEnabled = false;
    else if (cmd.startsWith("blink freq ")) {
      blinkFreq = cmd.substring(11).toInt();
      Serial.printf("Blink interval: %d ms\n", blinkFreq);
    }
    else if (cmd.startsWith("i2s init ")) {
      int s1 = cmd.indexOf(' ', 9);
      sampleRate = cmd.substring(9, s1).toInt();
      i2s.setDATA(28); i2s.setBCLK(26);
      if (i2s.begin(sampleRate)) {
        updatePhaseIncrement();
        Serial.printf("I2S Ready: %dHz\n", sampleRate);
      }
    }
    else if (cmd == "i2s start") toneRunning = true;
    else if (cmd == "i2s stop") toneRunning = false;
    else if (cmd == "i2s measure") measureI2S();
    else if (cmd == "reset") watchdog_reboot(0,0,0);
  }
}