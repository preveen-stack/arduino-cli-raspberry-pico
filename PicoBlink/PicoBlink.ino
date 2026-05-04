#include <Arduino.h>

unsigned long lastBlink = 0;
int blinkFreq = 500; // Default 500ms
bool blinkEnabled = true;
bool ledState = LOW;

void printPinout() {
  Serial.println("\n--- Raspberry Pi Pico Pinout ---");
  Serial.println("      [USB CONNECTOR]      ");
  Serial.println("GP0  [01] [40] VBUS (5V)");
  Serial.println("GP1  [02] [39] VSYS");
  Serial.println("GND  [03] [38] GND");
  Serial.println("GP2  [04] [37] 3V3_EN");
  Serial.println("GP3  [05] [36] 3V3_OUT");
  Serial.println("GP4  [06] [35] ADC_VREF");
  Serial.println("GP5  [07] [34] GP28 (ADC2)");
  Serial.println("GND  [08] [33] GND");
  Serial.println("GP6  [09] [32] GP27 (ADC1)");
  Serial.println("GP7  [10] [31] GP26 (ADC0)");
  Serial.println("GP8  [11] [30] RUN");
  Serial.println("GP9  [12] [29] GP22");
  Serial.println("GND  [13] [28] GND");
  Serial.println("GP10 [14] [27] GP21");
  Serial.println("GP11 [15] [26] GP20");
  Serial.println("GP12 [16] [25] GP19");
  Serial.println("GP13 [17] [24] GP18");
  Serial.println("GND  [18] [23] GND");
  Serial.println("GP14 [19] [22] GP17");
  Serial.println("GP15 [20] [21] GP16");
  Serial.println("--------------------------------\n");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  while (!Serial); // Wait for Serial Monitor to open
  Serial.println("Pico Control Ready. Type 'pinout', 'blink on/off', or 'blink freq <ms>'");
}

void loop() {
  // Handle Blinking logic
  if (blinkEnabled) {
    if (millis() - lastBlink >= (unsigned long)blinkFreq) {
      lastBlink = millis();
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState);
    }
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Handle Serial Commands
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();

    if (input == "pinout") {
      printPinout();
    } 
    else if (input == "blink on") {
      blinkEnabled = true;
      Serial.println("Blinking enabled.");
    } 
    else if (input == "blink off") {
      blinkEnabled = false;
      Serial.println("Blinking disabled.");
    } 
    else if (input.startsWith("blink freq ")) {
      String valStr = input.substring(11);
      int val = valStr.toInt();
      if (val >= 1 && val <= 10000) {
        blinkFreq = val;
        Serial.print("Frequency set to: ");
        Serial.print(blinkFreq);
        Serial.println(" ms");
      } else {
        Serial.println("Error: Frequency must be between 1 and 10000 ms.");
      }
    }
  }
}
