// https://qiita.com/hotchpotch/items/eec0260b8b1938dda696
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"

void setup() {
  Serial.begin(76880);
//  Serial.println("start");
  WiFiManager wifiManager;
//  wifiManager.resetSettings();
//  wifiManager.setConnectTimeout(60);
//  Serial.println(wifiManager.getConfigPortalSSID());
  wifiManager.autoConnect("ESP8266_WiFiAP");
  Serial.println("connected");
}

void loop() {
}
