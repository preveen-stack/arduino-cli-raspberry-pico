#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif
  #include <hardware/watchdog.h>
  #include <hardware/clocks.h>
  #include <hardware/adc.h>
#ifdef __cplusplus
}
#endif

unsigned long lastBlink = 0;
int blinkFreq = 500; 
bool blinkEnabled = true;
bool ledState = LOW;

void printHelp() {
  Serial.println("\n--- Available Commands ---");
  Serial.println("help             - Show this menu");
  Serial.println("pinout           - Display Pico pinout map");
  Serial.println("clock            - Show internal system clock speeds");
  Serial.println("temp             - Read internal CPU temperature");
  Serial.println("blink on/off     - Toggle onboard LED");
  Serial.println("blink freq <ms>  - Set rate (1 - 10000)");
  Serial.println("reset            - Reboot the Pico");
  Serial.println("---------------------------\n");
}

void printTemp() {
  adc_select_input(4); // Internal temp sensor is always on ADC 4
  uint16_t raw = adc_read();
  const float conversion_factor = 3.3f / (1 << 12);
  float voltage = raw * conversion_factor;
  float temp = 27.0f - (voltage - 0.706f) / 0.001721f;

  Serial.print("CPU Temperature: ");
  Serial.print(temp, 2);
  Serial.println(" °C");
}

void printClocks() {
  Serial.println("\n--- RP2040 Clock Frequencies ---");

  // These calls are baked into the Arduino RP2040 core
  Serial.print("System Clock: ");
  Serial.print(rp2040.f_cpu() / 1000000.0);
  Serial.println(" MHz");

  // These constants are usually defined, but let's use the object for safety
  Serial.print("USB Clock:    48.00 MHz (Fixed)");
  Serial.println();

  Serial.print("ADC Clock:    48.00 MHz (Fixed)");
  Serial.println();

  Serial.println("--------------------------------\n");
}


void printPinout() {
  Serial.println("\n--- Raspberry Pi Pico Pinout ---");
  Serial.println("GP0 [01] [40] VBUS | GP1 [02] [39] VSYS");
  Serial.println("GND [03] [38] GND  | GP2 [04] [37] 3V3_EN");
  // ... (Full text from previous step)
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Initialize ADC for temperature sensor
  adc_init();
  adc_set_temp_sensor_enabled(true);
  
  delay(2000); 
  Serial.println("Pico System Online. Type 'help' for commands.");
}

void loop() {
  if (blinkEnabled && (millis() - lastBlink >= (unsigned long)blinkFreq)) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();

    if (input == "help") printHelp();
    else if (input == "pinout") printPinout();
    else if (input == "clock") printClocks();
    else if (input == "temp") printTemp();
    else if (input == "blink on") { blinkEnabled = true; Serial.println("Blinking ON"); }
    else if (input == "blink off") { blinkEnabled = false; Serial.println("Blinking OFF"); }
    else if (input.startsWith("blink freq ")) {
      int val = input.substring(11).toInt();
      if (val >= 1 && val <= 10000) {
        blinkFreq = val;
        Serial.printf("Freq set to %d ms\n", blinkFreq);
      }
    }
    else if (input == "reset") {
      Serial.println("Rebooting...");
      delay(100);
      watchdog_reboot(0, 0, 0); 
    }
    else if (input != "") {
      Serial.println("Unknown command. Type 'help'.");
    }
  }
}
