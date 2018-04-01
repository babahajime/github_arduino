// soil_moiture_blynk.java

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>

// Blynkで生成されるAuth Token
char auth[] = "eba203f264104d43a5b4916c3f5184ef";
// 無線ルーターのSSIDとパスワード
char ssid[] = "aterm-893793-g";
char pass[] = "35b2c26f4d6b8";

int soil_moisture;
int t1 = 300; // threshold #1
int t2 = 700; // threshold #2

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  Wire.begin(); 
}

BLYNK_READ(V0) {
  soil_moisture = analogRead(0);
  Serial.println(soil_moisture);
  if (soil_moisture > 0 && soil_moisture <= t1) {
    Blynk.virtualWrite(V0, "dry soil");
    Serial.println("dry soil");
  } else if (soil_moisture > t1 && soil_moisture <= t2) {
    Blynk.virtualWrite(V0, "humid soil");
    Serial.println("humid soil");
  } else if (soil_moisture > t2 && soil_moisture <= 1024) {
    Blynk.virtualWrite(V0, "in water");
    Serial.println("in water");
  } else {
    Blynk.virtualWrite(V0, "unknown");
    Serial.println("unknown");
  }
}

BLYNK_READ(V1) {
  soil_moisture = analogRead(0);
//  Serial.println(soil_moisture);
  Blynk.virtualWrite(V1, soil_moisture);
}

BLYNK_WRITE(V2) {
  //スマホ側 Blynk アプリで設定したスライダー値の受信
  t1 = param[0].asInt();
  Serial.printf("Threshold#1 changed: %d\r\n", t1);
}

BLYNK_WRITE(V3) {
  //スマホ側 Blynk アプリで設定したスライダー値の受信
  t2 = param[0].asInt();
  Serial.printf("Threshold#2 changed: %d\r\n", t2);
}

void loop() {
  // put your main code here, to run repeatedly:
  Blynk.run();
  delay(5000);  // every 5 seconds
}


