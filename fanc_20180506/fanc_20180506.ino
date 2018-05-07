// fanc - FAN Control
//
// http://intellectualcuriosity.hatenablog.com/entry/2017/03/16/040952
//


#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h> // HTTPClient
#include "DHT.h"
#include <IRsend.h>

#define SPEED 74880                         // or 115200
// ESP.deepSleepするためには、
// IO16(19番ピン)を RST(リセット,17番ピン) に繋いでおく必要がある。
// → 指定時間経過後にリセットが実行され、再起動がかかる。

// DHT11の設定
#define DHTPIN 0        // 使用するGPIOピン
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

// 抵抗計算
// OSI5FU5111C-40    VF=1.35V, IF=100mA
// http://akizukidenshi.com/download/OSI5FU5111C-40.pdf
// (3.3V - 1.35V) / 0.100 A = 19.5 Ω
// でも光らない、、、以下見つけた
// http://canadie.hatenablog.com/entry/2015/09/05/125025
// ちゃんとトランジスタをかませればok。10Ωと10kΩを入れた
#define IR_LED 4  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(IR_LED);  // Set the GPIO to be used to sending the message.


// WiFiアクセスポイントの設定
const char *WIFI_SSID = "aterm-893793-g";
const char *WIFI_PASSWORD = "35b2c26f4d6b8";

// ITFFFの設定
const char *IFTTT_HOST = "maker.ifttt.com";
const char *IFTTT_URI = "/trigger/ESP8266-fanc/with/key/bBbR_j_WXc27Ufaut-qKik";

// ThingSpeak Settings
const char* THING_SPEAK_HOST = "api.thingspeak.com";
const char* THING_SPEAK_WRITE_API_KEY = "UCGS9BTAC6WGHZU7"; // 各自取得したキーに差し替え

const int channelID = 489810;
String writeAPIKey = "UCGS9BTAC6WGHZU7"; // write API key for your ThingSpeak Channel
const char* server = "api.thingspeak.com";
//const int postingInterval = 1 * 60 * 1000; // post data every 1 minutes

// DeepSleepのインターバル
//const unsigned long DEEP_SLEEP_INTERVAL = 60 * 60 * 1000 * 1000;  // 1時間
const unsigned long DEEP_SLEEP_INTERVAL = 1 * 60 * 1000 * 1000;  // 5分


void setup() {
  Serial.begin(SPEED);
  Serial.println("Booting");
  dht.begin();
  irsend.begin();
  
  // WiFiに接続
  WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Waiting for Wi-Fi connection");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\r\nConnected!");
}

void loop() {
  // DHT11から温度と湿度を読み込む
  delay(2000);
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.println("Temperature: " + String(t) + "C   Humidity: " + String(h) + "%");
  // 不快指数を計算 T-H index
  // https://ja.wikipedia.org/wiki/%E4%B8%8D%E5%BF%AB%E6%8C%87%E6%95%B0
  // -55: 寒い、55-60: 寒くない、60-65: 何も感じない、65-70: 快い、
  // 70-75: 暑くない、75-80: やや暑い、80-85: 暑くて汗が出る、85-: 暑くてたまらない
  float th_index = 0.81 * t + 0.01 * h * (0.99 * t - 14.9) + 46.3;
  Serial.println("T-H index: " + String(th_index));

  // ThingSpeakに温度と湿度を送る
  thingspeak(t, h, th_index);

  // 不快指数が75以上ならファンをON、 75以下ならファンOFF
  if (th_index >= 75) {
    fan_on();
    // IFTTTに温度と湿度を送る
    RequestToIFTTT("?value1=" + String(round(t)) + "&value2=" + String(round(h)) + "&value3=" + String(round(th_index)));
  } else {
    fan_off();
  }


  // DeepSleepに入る
  Serial.println("Go to sleep...");
  ESP.deepSleep(DEEP_SLEEP_INTERVAL, WAKE_RF_DEFAULT);
}

// IFTTTにリクエストを送る
void RequestToIFTTT(String param) {
  WiFiClientSecure client;
  while (!client.connect(IFTTT_HOST, 443)) {
    delay(10);
  }
  client.print(String("GET ") + IFTTT_URI + param +
                       " HTTP/1.1\r\n" +
                       "Host: " + IFTTT_HOST + "\r\n" +
                       "User-Agent: ESP8266\r\n" +
                       "Connection: close\r\n\r\n");
  while (!client.available()) {
    delay(10);
  }
  Serial.println(client.readStringUntil('\r'));
  client.flush();
  client.stop();
}

// HTTPClient版
void thingspeak(float temperature, float humidity, float th_index) {
  // Wi-Fi 接続できてたら ThingSpeak へ送信
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    char url[128];
    sprintf(url, "http://%s/update/", THING_SPEAK_HOST); // not https
    http.begin(url);

    // ThingSpeak API キーはヘッダーに
    http.addHeader("X-THINGSPEAKAPIKEY", THING_SPEAK_WRITE_API_KEY);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // POST パラメータ作る
    char params[128];
    char tempstr[4];
    char humistr[4];
    char thidxstr[4];
    sprintf(params, "field1=%s&field2=%s&field3=%s",
            dtostrf(temperature, 1, 0, tempstr), dtostrf(humidity, 1, 0, humistr),
            dtostrf(th_index,1,0,thidxstr));
    // POST リクエストする
    int httpCode = http.POST(params);

    if (httpCode > 0) {
      // HTTP レスポンスコードが返ってくる
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
    } else {
      // コネクションエラーになるとマイナスが返る
      // see https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/src/ESP8266HTTPClient.h
      // HTTP client errors
      Serial.println("[HTTPS] no connection or no HTTP server.");
    }

    http.end();
  }
}


void fan_on() {
  Serial.println("fan_on()");
  // ファンを「ソフト + 1/fゆらぎon + 切タイマー3時間on」で駆動
  uint16_t soft_rawData[131] = {3476, 1724,  452, 418,  452, 1288,  454, 418,  452, 418,  452, 418,  452, 418,  452, 418,  452, 418,  452, 418,  452, 418,  454, 416,  452, 418,  452, 418,  452, 1288,  454, 416,  454, 418,  452, 418,  452, 418,  452, 418,  452, 418,  452, 1288,  454, 418,  452, 1288,  454, 1286,  454, 418,  452, 416,  454, 1288,  454, 418,  452, 418,  452, 418,  452, 418,  452, 1288,  454, 418,  452, 418,  452, 418,  452, 416,  454, 1288,  454, 1288,  454, 418,  452, 418,  452, 1286,  454, 416,  454, 416,  454, 416,  454, 418,  452, 1286,  454, 418,  452, 416,  454, 1286,  454, 416,  454, 416,  454, 1288,  454, 418,  452, 416,  454, 416,  454, 416,  454, 418,  452, 416,  454, 1288,  454, 1288,  454, 416,  454, 416,  454, 1286,  454, 418,  452};  // UNKNOWN 481E6C3E
  uint16_t yuragi_rawData[131] = {3500, 1698,  480, 390,  480, 1260,  480, 392,  476, 392,  478, 392,  478, 394,  478, 390,  480, 390,  478, 392,  478, 390,  480, 390,  478, 390,  480, 390,  480, 1260,  478, 394,  480, 390,  478, 392,  478, 392,  478, 392,  478, 392,  480, 1262,  480, 392,  476, 1262,  480, 1260,  478, 394,  478, 390,  480, 1262,  480, 390,  480, 390,  478, 392,  480, 390,  478, 1260,  482, 392,  476, 394,  478, 390,  478, 392,  478, 1262,  478, 1262,  480, 392,  480, 392,  478, 392,  476, 1262,  478, 394,  476, 394,  478, 392,  476, 394,  476, 394,  478, 392,  478, 394,  478, 392,  476, 394,  478, 392,  478, 1262,  478, 1262,  476, 394,  476, 394,  474, 396,  478, 1262,  476, 1264,  476, 396,  476, 1264,  478, 392,  476, 1264,  478, 394,  422};  // UNKNOWN C84D16B8
  uint16_t timer_rawData[131] = {3474, 1724,  454, 418,  452, 1288,  452, 418,  452, 418,  452, 418,  452, 418,  452, 418,  454, 418,  452, 418,  452, 418,  452, 416,  452, 418,  454, 416,  452, 1288,  452, 418,  454, 418,  452, 418,  452, 418,  452, 418,  452, 418,  452, 1288,  454, 418,  452, 1288,  454, 1288,  452, 418,  452, 418,  452, 1288,  454, 418,  452, 416,  452, 418,  452, 418,  452, 1288,  454, 418,  452, 418,  452, 418,  452, 418,  452, 1288,  454, 1288,  454, 418,  452, 418,  452, 1288,  454, 1288,  454, 418,  452, 416,  452, 418,  452, 418,  452, 1288,  454, 418,  452, 418,  452, 418,  452, 418,  452, 418,  454, 1286,  454, 1288,  452, 418,  452, 416,  454, 1288,  452, 1288,  454, 1288,  452, 416,  454, 1288,  454, 418,  452, 418,  452, 418,  452};  // UNKNOWN DDD278E8

  Serial.println("a rawData capture from IRrecvDumpV2");
  irsend.sendRaw(soft_rawData, 131, 38);  // Send a raw data capture at 38kHz.
  delay(1000);
  irsend.sendRaw(yuragi_rawData, 131, 38);  // Send a raw data capture at 38kHz.
  delay(1000);
  irsend.sendRaw(timer_rawData, 131, 38);  // Send a raw data capture at 38kHz.
  delay(1000);
}

void fan_off() {
  Serial.println("fan_off()");
  // ファンを「切」で駆動
  uint16_t kiru_rawData[131] = {3476, 1722,  454, 416,  454, 1286,  454, 418,  452, 416,  454, 416,  454, 416,  454, 416,  454, 416,  454, 416,  454, 416,  454, 416,  454, 416,  454, 416,  454, 1286,  454, 416,  454, 418,  452, 416,  454, 416,  454, 416,  454, 416,  454, 1288,  454, 416,  454, 1286,  454, 1288,  454, 416,  454, 416,  454, 1286,  454, 418,  452, 416,  454, 416,  454, 416,  454, 1288,  454, 416,  454, 418,  452, 416,  454, 416,  454, 1288,  454, 1286,  454, 416,  454, 416,  454, 1286,  454, 1286,  456, 416,  454, 418,  452, 1286,  454, 1286,  454, 416,  454, 416,  454, 416,  454, 416,  454, 416,  454, 418,  452, 1286,  454, 1286,  454, 416,  454, 416,  454, 1286,  454, 1286,  456, 1286,  454, 416,  454, 416,  454, 1286,  454, 1286,  454, 416,  454};  // UNKNOWN 258105EC

  Serial.println("a rawData capture from IRrecvDumpV2");
  irsend.sendRaw(kiru_rawData, 131, 38);  // Send a raw data capture at 38kHz.
  delay(1000);
}

