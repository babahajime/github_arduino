// gmnk - GoMu-No-Ki sensor
//
// もっと良いサンプルが以下にあった、、、
// https://jp.mathworks.com/help/thingspeak/MoistureMonitor.html
//

#include <ESP8266WiFi.h>
// https://github.com/tzapu/WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

// ThingSpeak Settings
const int channelID = 455143;
String writeAPIKey = "I52YPHQYVFC5NBXD"; // write API key for your ThingSpeak Channel
const char* server = "api.thingspeak.com";
const int postingInterval = 1 * 60 * 1000; // post data every 1 minutes


#define PIN_LED 4                           // GPIO 4(12番ピン)にLEDを接続する 
#define PIN_BZR 5                           // GPIO 5(16番ピン)にブザーを接続する 
#define PIN_TOUT 18                         // 18番ピンがTOUT（センサ）を接続する 
// ESP.deepSleepするためには、
// IO16(19番ピン)を RST(リセット,17番ピン) に繋いでおく必要がある。
// → 指定時間経過後にリセットが実行され、再起動がかかる。

#define SPEED 74880                         // or 115200
#define SLEEP_P 5*60*1000000                // スリープ時間 5分(uint32_t) 
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
void thingspeak();


// http://docs.blynk.cc/#blynk-firmware-blynktimer
void myTimerEvent() {
  // １回だと分からんから、複数回読むように要改良
//  soil_moisture = analogRead(0); // AO==TOUTから読む
  Serial.print("soil_moisture=");
  Serial.println(soil_moisture);
  thingspeak(soil_moisture);

  if (soil_moisture > 0 && soil_moisture <= t1) {
    Serial.println(status[0]);
  } else if (soil_moisture > t1 && soil_moisture <= t2) {
    Serial.println(status[1]);
  } else if (soil_moisture > t2 && soil_moisture <= 1024) {  // 濡れています
    Serial.println(message);
//    led_flash(500); // LED lighting
//    beep3(); // and beep
  } else {
    Serial.println(status[3]);
  }
  delay(200);                             // 送信待ち時間 
}


// https://github.com/bokunimowakaru/esp/blob/master/2_example/example09m_hum/example09m_hum.ino
// センサ値を読取、Wifi接続送信、
void setup(){                             // 起動時に一度だけ実行する関数 
  int waiting=0;                          // アクセスポイント接続待ち用 
  pinMode(PIN_LED,OUTPUT);
  pinMode(PIN_BZR,OUTPUT);
  Serial.begin(SPEED);
  Serial.println("Booting");
//  led_flash(200); // msec
  
  // １回だと分からんから、複数回読むように要改良
  soil_moisture = analogRead(0);          // AO==TOUTから読む
  Serial.print("soil_moisture=");
  Serial.println(soil_moisture);
/*
  mem = fabs(readRtcInt()-soil_moisture); // RTCメモリの温度値と比較する 
  Serial.print("mem="); Serial.println(mem);
  if( WAKE_COUNT % SLEEP_N &&             // SLEEP_Nが0以外 かつ 
      mem <= DEADZONE )                   // 閾値以下 のときに 
    sleep();                              // スリープを実行
*/
  
  Serial.println("Wifi Booting...");
  WiFiManager wifiManager;
  wifiManager.autoConnect();
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

  myTimerEvent();                         // every 5 seconds
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


void thingspeak(int soil_moisture) {
  // Construct API request body
  String postStr = "field1=";
  postStr += String(soil_moisture);
  
  WiFiClient client;
  if (client.connect(server, 80)) {
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: ");
    client.print(server);
    client.print("\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + writeAPIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
  }
}


