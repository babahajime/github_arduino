// omts - OMuTsu Sensor

#define USE_WIFI_MODE // or 0

#ifdef USE_WIFI_MODE
#include <ESP8266WiFi.h>
// https://github.com/tzapu/WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

// Blynk用
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <BlynkSimpleEsp8266.h>
//#include <Wire.h>
BlynkTimer timer;
// 個別設定（SSID,パスワード,AuthToken等）
#include "config.h"
#endif

#define PIN_LED 4                           // GPIO 4(12番ピン)にLEDを接続する 
#define PIN_BZR 5                           // GPIO 5(16番ピン)にブザーを接続する 
#define PIN_TOUT 18                         // 18番ピンがTOUT（センサ）を接続する 
// ESP.deepSleepするためには、
// IO16(19番ピン)を RST(リセット,17番ピン) に繋いでおく必要がある。
// → 指定時間経過後にリセットが実行され、再起動がかかる。

#define SPEED 74880                         // or 115200
#define SLEEP_P 5*1000000                   // スリープ時間 5秒(uint32_t) 
#define SLEEP_N 6                           // 最長スリープ時間 SLEEP_P×SLEEP_N 
#define DEADZONE 10                         // 前回値との相違に対する閾値(生値）

static char* status[] = {"乾いています", "ちょうど良い", "濡れています", "不明"};
static char message[128];
static int soil_moisture = 0; // 初期値0
static int t1 = 300; // 初期値threshold #1
static int t2 = 700; // 初期値threshold #2
static int mem = 0;                         // RTCメモリからの数値データ保存用 
extern int WAKE_COUNT; 
unsigned long start_ms;                     // 初期化開始時のタイマー値を保存 

void sleep(); 
void led_flash();
void beep();
void beep3();
void ifttt_webhook();


// http://docs.blynk.cc/#blynk-firmware-blynktimer
void myTimerEvent() {
  // １回だと分からんから、複数回読むように要改良
//  soil_moisture = analogRead(0); // AO==TOUTから読む
  Serial.print("soil_moisture=");
  Serial.println(soil_moisture);
#ifdef USE_WIFI_MODE
  Blynk.virtualWrite(V0, soil_moisture);
#endif

  if (soil_moisture > 0 && soil_moisture <= t1) {
    Serial.println(status[0]);
#ifdef USE_WIFI_MODE
    Blynk.virtualWrite(V1, status[0]);
#endif
  } else if (soil_moisture > t1 && soil_moisture <= t2) {
    Serial.println(status[1]);
#ifdef USE_WIFI_MODE
    Blynk.virtualWrite(V1, status[1]);
#endif
  } else if (soil_moisture > t2 && soil_moisture <= 1024) {  // 濡れています
    Serial.println(message);
    led_flash(500); // LED lighting
    beep3(); // and beep
#ifdef USE_WIFI_MODE
    Blynk.virtualWrite(V1, status[2]);
    sprintf(message, "OMTSセンサー通知: %s", status[2]);
//    Blynk.tweet(message);   // 自分がつぶやく
    Blynk.email(email_addr, "OMTSセンサー通知", message);  // メール送信
    Blynk.notify(message);    // Blynkアプリの通知機能
//    ifttt_webhook(message); // Blynkマニュアルのwebhookの項目
#endif
  } else {
    Serial.println(status[3]);
#ifdef USE_WIFI_MODE
    Blynk.virtualWrite(V1, status[3]);
#endif
  }
  delay(200);                             // 送信待ち時間 
}

#ifdef USE_WIFI_MODE
//スマホ側 Blynk アプリで設定したスライダー値の受信
BLYNK_WRITE(V2) {
  t1 = param[0].asInt();
  Serial.printf("Threshold#1 changed to: %d\r\n", t1);
}

//スマホ側 Blynk アプリで設定したスライダー値の受信
BLYNK_WRITE(V3) {
  t2 = param[0].asInt();
  Serial.printf("Threshold#2 changed to: %d\r\n", t2);
}
#endif


// https://github.com/bokunimowakaru/esp/blob/master/2_example/example09m_hum/example09m_hum.ino
// センサ値を読取、Wifi接続送信、
void setup(){                             // 起動時に一度だけ実行する関数 
  int waiting=0;                          // アクセスポイント接続待ち用 
  pinMode(PIN_LED,OUTPUT);
  pinMode(PIN_BZR,OUTPUT);
  Serial.begin(SPEED);
  Serial.println("Booting");
  led_flash(200); // msec
  
  // １回だと分からんから、複数回読むように要改良
  soil_moisture = analogRead(0);          // AO==TOUTから読む
  Serial.print("soil_moisture=");
  Serial.println(soil_moisture);
  mem = fabs(readRtcInt()-soil_moisture); // RTCメモリの温度値と比較する 
  Serial.print("mem="); Serial.println(mem);
  if( WAKE_COUNT % SLEEP_N &&             // SLEEP_Nが0以外 かつ 
      mem <= DEADZONE )                   // 閾値以下 のときに 
    sleep();                              // スリープを実行

#ifdef USE_WIFI_MODE
  Serial.println("Wifi Booting...");
  WiFiManager wifiManager;
  wifiManager.autoConnect("ESP8266_WiFiAP");
//  WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定 
//  WiFi.begin(ssid,password);              // 無線LANアクセスポイントへ接続 
//  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
  while(WiFi.status() != WL_CONNECTED){   // 接続に成功するまで待つ
    Serial.println("Connection Failed! Rebooting...");
    delay(100);                           // 待ち時間処理 
    waiting++;                            // 待ち時間カウンタを1加算する 
    if(waiting%10==0) Serial.print('.');  // 進捗表示 
    if(waiting > 300) sleep();            // 300回(30秒)を過ぎたらスリープ 
  } 
  Serial.println(WiFi.localIP());         // 本機のIPアドレスをシリアル出力   

  Blynk.config(blynk_auth);               // or Blynk.config(auth,server,port);
  bool result = Blynk.connect();          // 接続
//  Wire.begin(); // Wireライブラリを初期化し、I2Cバスにマスタとして接続

  // Blynk Setup a function to be called every second
  //  timer.setInterval(5*1000L, myTimerEvent); // これを使わなくてok
  myTimerEvent();                         // every 5 seconds
#endif
}

void loop() {
  WAKE_COUNT=1;                           // 起動回数をリセット 
  writeRtcInt(soil_moisture);             // 湿度値をRTCメモリへ保存 
  delay(200);                             // 送信待ち時間 
  sleep();
}

void led_flash(int msec) {
  digitalWrite(PIN_LED,HIGH);             // LEDの点灯 
  delay(msec);
  digitalWrite(PIN_LED,LOW);              // LEDの消灯 
}

void sleep(){ 
  Serial.print("Sleeping...");
  digitalWrite(PIN_LED,LOW);              // LEDの消灯 
  digitalWrite(PIN_BZR,LOW);              // BZRの消灯
//WAKE_RF_DEFAULT = 0, // RF_CAL or not after deep-sleep wake up, depends on init data byte 108.
//WAKE_RFCAL = 1,      // RF_CAL after deep-sleep wake up, there will be large current.
//WAKE_NO_RFCAL = 2,   // no RF_CAL after deep-sleep wake up, there will only be small current.
//WAKE_RF_DISABLED = 4 // disable RF after deep-sleep wake up, just like modem sleep, there will be the smallest current.
  ESP.deepSleep(SLEEP_P,WAKE_RF_DEFAULT); // スリープモードへ移行する 
  while(1){                               // 繰り返し処理 
    delay(100);                           // 100msの待ち時間処理 
    Serial.print(".");
  }                                       // 繰り返し中にスリープへ移行 
}

void beep() {
  analogWrite(PIN_BZR, 500);
  analogWriteFreq(440); // ラの音
  delay(500);
  analogWrite(PIN_BZR, 0);
  delay(500);
}

void beep3() {
  for (int i=0;i<3;i++) beep();
}

/*
  作りかけ。まだ動かない。

// Blynkマニュアルのwebhookの項目（thinkgspeakの例）を参考に
// IFTTT  https://maker.ifttt.com/trigger/{event}/with/key/{key}";
void ifttt_webhook(char *message) {
  WiFiSecureClient client;
  char url[256];
  char json[256];
  sprintf(url, "maker.ifttt.com/trigger/%s/with/%s", ifttt_event, ifttt_key);
  sprintf(json, "{ value1: %s, value2: %s, value3: %s }", v1, v2, v3);

  if (client.connect(url, 443)) {
    client.print("POST /update HTTP/1.1\n");
    client.print("Content-Type: application/json\n");
    client.print(json);
  }
//  WiFiClient client;
//  if (client.connect("api.thingspeak.com", 80)) {
//    client.print("POST /update HTTP/1.1\n");
//    client.print("Host: api.thingspeak.com\n");
//    client.print("Connection: close\n");
//    client.print("X-THINGSPEAKAPIKEY: " + apiKeyThingspeak1 + "\n");
//    client.print("Content-Type: application/x-www-form-urlencoded\n");
//    client.print("Content-Length: ");
//    client.print(postStr.length());
//    client.print("\n\n");
//    client.print(postStr);
//  }
}
*/

