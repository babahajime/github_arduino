// sample02 - �u�U�[

#include <ESP8266WiFi.h>

#define PIN_BZR 5                         // GPIO 5(16�ԃs��)

void beep() {
  analogWrite(PIN_BZR, 500);
  analogWriteFreq(440); // ���̉�
  delay(500);
  analogWrite(PIN_BZR, 0);
  delay(500);
}

void setup() {
  beep();
}

void loop() {
}

