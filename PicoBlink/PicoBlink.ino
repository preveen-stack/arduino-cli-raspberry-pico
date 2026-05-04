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

// --- I2S Instance ---
I2S i2s(OUTPUT);

// --- Global Variables ---
unsigned long lastBlink = 0;
int blinkFreq = 500;
bool blinkEnabled = true;
bool ledState = LOW;

// --- Tone Generator ---
bool toneRunning = false;
float currentFreq = 440.0;
float phase = 0;
float phaseIncrement = 0;
int sampleRate = 44100;
int bitDepth = 16;

void updatePhaseIncrement() {
  phaseIncrement = (2.0 * PI * currentFreq) / (float)sampleRate;
}

// --- Menu Functions ---
void printHelp() {
  Serial.println("\n========= PICO CONTROL MENU =========");
  Serial.println("help             - Show this menu");
  Serial.println("pinout           - Display Pico pinout map");
  Serial.println("clock            - Show system clock speeds");
  Serial.println("temp             - Read internal CPU temp");
  Serial.println("blink on/off     - Toggle onboard LED");
  Serial.println("blink freq <ms>  - Set LED rate (1-10000)");
  Serial.println("reset            - Reboot the Pico");
  Serial.println("\n--- I2S Commands ---");
  Serial.println("i2s init <sr> <bw> <ch> - e.g., i2s init 44100 16 2");
  Serial.println("i2s start/stop          - Toggle audio engine");
  Serial.println("i2s status              - Current I2S config");
  Serial.println("i2s detail              - PIO logical clock values");
  Serial.println("i2s tone <freq>         - Set sine wave (100-18000)");
  Serial.println("i2s measure             - Hardware clock verify");
  Serial.println("=====================================\n");
}

void printPinout() {
  Serial.println("\n--- Raspberry Pi Pico Pinout ---");
  Serial.println("      [USB CONNECTOR]      ");
  Serial.println("GP0  [01] [40] VBUS (5V)");
  Serial.println("GP1  [02] [39] VSYS");
  Serial.println("GND  [03] [38] GND");
  Serial.println("GP28 [34] DOUT (Data)");
  Serial.println("GP27 [32] LRCLK (Word)");
  Serial.println("GP26 [31] BCLK (Bit)");
  Serial.println("--------------------------------\n");
}

void printClocks() {
  Serial.println("\n--- System Clocks ---");
  Serial.printf("System: %.2f MHz\n", rp2040.f_cpu() / 1000000.0);
  Serial.printf("USB:    48.00 MHz\n");
  Serial.printf("ADC:    48.00 MHz\n");
}

void printTemp() {
  adc_select_input(4);
  uint16_t raw = adc_read();
  float voltage = raw * (3.3f / (1 << 12));
  float temp = 27.0f - (voltage - 0.706f) / 0.001721f;
  Serial.printf("CPU Temp: %.2f °C\n", temp);
}

void measureI2S() {
  Serial.println("\n--- Hardware Frequency Counter (Live) ---");

  // Force input buffers ON for these pins so the FC0 can 'see' the PIO output
  gpio_set_input_enabled(26, true);
  gpio_set_input_enabled(27, true);
  
  // Give the hardware a microsecond to settle
  delayMicroseconds(10);

  // Re-attempt measurement using the suggested SDK macro
  uint32_t bclk_khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLKSRC_GPIN0 + 26);
  uint32_t lrclk_khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLKSRC_GPIN0 + 27);

  // If it still shows SysClk, we try the secondary mapping (46 is the GPIO offset)
  if (lrclk_khz > 100000) { // If it's reporting ~125MHz
      bclk_khz = frequency_count_khz(46 + 26);
      lrclk_khz = frequency_count_khz(46 + 27);
  }

  Serial.printf("BCLK (GP26):  %.3f MHz\n", bclk_khz / 1000.0);
  Serial.printf("LRCLK (GP27): %u Hz\n", lrclk_khz * 1000);

  if (lrclk_khz > 0 && lrclk_khz < 200000) { // Reasonable audio range
    Serial.printf("Bits/Frame:    %.1f\n", (float)bclk_khz / (float)lrclk_khz);
  } else {
    Serial.println("Note: Hardware counter is still defaulting to SysClk.");
    Serial.println("Use 'i2s detail' for theoretical verification.");
  }
}

void handleI2STone() {
  if (!toneRunning) return;

  while (i2s.availableForWrite()) {
    int16_t sample = (int16_t)(sin(phase) * 32767.0f);
    i2s.write(sample); // Left
    i2s.write(sample); // Right
    
    phase += phaseIncrement;
    if (phase >= 2.0 * PI) phase -= 2.0 * PI;
  }
}

// --- Main Setup & Loop ---
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
    else if (cmd == "pinout") printPinout();
    else if (cmd == "clock") printClocks();
    else if (cmd == "temp") printTemp();
    else if (cmd == "reset") {
      Serial.println("Resetting...");
      delay(100);
      watchdog_reboot(0,0,0);
    }
    else if (cmd == "blink on") blinkEnabled = true;
    else if (cmd == "blink off") blinkEnabled = false;
    else if (cmd.startsWith("blink freq ")) {
      blinkFreq = cmd.substring(11).toInt();
      Serial.printf("Blink set to %d ms\n", blinkFreq);
    }
    
    // I2S Submenu
    else if (cmd.startsWith("i2s init ")) {
      int s1 = cmd.indexOf(' ', 9);
      int s2 = cmd.indexOf(' ', s1 + 1);
      sampleRate = cmd.substring(9, s1).toInt();
      bitDepth = cmd.substring(s1 + 1, s2).toInt();
      
      i2s.setDATA(28);
      i2s.setBCLK(26);
      i2s.setBitsPerSample(bitDepth);
      
      if (i2s.begin(sampleRate)) {
        // Force inputs to stay enabled so the frequency counter can read them back
        gpio_set_input_enabled(26, true);
        gpio_set_input_enabled(27, true);
        
        updatePhaseIncrement();
        Serial.printf("I2S Ready: %dHz/%dbit\n", sampleRate, bitDepth);
      }
    }
    else if (cmd == "i2s start") { toneRunning = true; Serial.println("Audio Started"); }
    else if (cmd == "i2s stop")  { toneRunning = false; Serial.println("Audio Stopped"); }
    else if (cmd == "i2s status") {
      Serial.printf("Status: %s | Freq: %.1fHz | SR: %d\n", toneRunning?"RUN":"STOP", currentFreq, sampleRate);
    }
    else if (cmd == "i2s detail") {
      float sysClk = rp2040.f_cpu();
      float targetBclk = (float)sampleRate * bitDepth * 2.0f;
      float pioDiv = sysClk / (targetBclk * 2.0f);
      Serial.printf("SysClk: %.2f MHz | Target BCLK: %.3f MHz | PIO Div: %.4f\n", sysClk/1e6, targetBclk/1e6, pioDiv);
    }
    else if (cmd.startsWith("i2s tone ")) {
      currentFreq = cmd.substring(9).toFloat();
      updatePhaseIncrement();
      Serial.printf("Tone set to %.1f Hz\n", currentFreq);
    }
    else if (cmd == "i2s measure") {
      measureI2S();
    } else if (cmd == "i2s check") {
      // Check if the PIO FIFO is being emptied (meaning data is going to pins)
      bool pioActive = !pio_sm_is_tx_fifo_full(pio0, 0); 
      Serial.printf("PIO Engine Active: %s\n", pioActive ? "YES" : "NO");

      // Logical verification of the pins
      Serial.printf("Pin Mapping: BCLK=%d, LRCLK=%d, DOUT=%d\n", 26, 27, 28);

      if (toneRunning) {
        Serial.println("Result: Engine is pumping data. Hardware is likely pulsing.");
      } else {
        Serial.println("Result: Engine is idle.");
      }
    }
  }
}
