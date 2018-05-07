// sample01 - Lチカ

#include <ESP8266WiFi.h>

#define PIN_LED 5                         // GPIO 5(16番ピン)にブザーを接続する 

void setup() {
  pinMode(PIN_LED, OUTPUT);               // GPIO#16
  digitalWrite(PIN_LED,HIGH);             // LEDの点灯 
  delay(msec);
  digitalWrite(PIN_LED,LOW);              // LEDの消灯 
}

void loop() {
}
