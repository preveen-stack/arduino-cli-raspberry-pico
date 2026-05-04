void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // LED_BUILTIN is pin 25 on the Pico
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
