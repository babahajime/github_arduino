// sample01 - L�`�J

#include <ESP8266WiFi.h>

#define PIN_LED 5                         // GPIO 5(16�ԃs��)�Ƀu�U�[��ڑ����� 

void setup() {
  pinMode(PIN_LED, OUTPUT);               // GPIO#16
  digitalWrite(PIN_LED,HIGH);             // LED�̓_�� 
  delay(msec);
  digitalWrite(PIN_LED,LOW);              // LED�̏��� 
}

void loop() {
}
