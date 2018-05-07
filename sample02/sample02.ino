// sample02 - ブザー

#include <ESP8266WiFi.h>

#define PIN_BZR 5                         // GPIO 5(16番ピン)

void beep() {
  analogWrite(PIN_BZR, 500);
  analogWriteFreq(440); // ラの音
  delay(500);
  analogWrite(PIN_BZR, 0);
  delay(500);
}

void setup() {
  beep();
}

void loop() {
}

