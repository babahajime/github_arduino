// omts - OMuTsu Sensor

#define USE_WIFI_MODE 1  // or 0

#ifdef USE_WIFI_MODE
#include <ESP8266WiFi.h>
// Blynk用
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
BlynkTimer timer;
// 個別設定（SSID,パスワード,AuthToken等）
#include "config.h"
#endif

#define PIN_LED 16                          // IO 16(19番ピン)にLEDを接続する 
#define PIN_BZR 5                           // IO  5(16番ピン)にブザーを接続する 
#define PIN_TOUT 18                         // 18番ピンがTOUT（センサ）を接続する 
#define SLEEP_P 20*60*1000000               // スリープ時間 20分(uint32_t) 
#define SLEEP_N 36                          // 最長スリープ時間 SLEEP_P×SLEEP_N 
#define DEADZONE 0.3                        // 前回値との相違に対する閾値(℃) 

static char* status[] = {"乾いています", "ちょうど良い", "濡れています", "不明"};
//static char* status[] = {"dry", "humid", "wet", "unknown"};
static int soil_moisture = 0; // 初期値0
static int t1 = 300; // 初期値threshold #1
static int t2 = 700; // 初期値threshold #2
static int mem;                             // RTCメモリからの数値データ保存用 
extern int WAKE_COUNT; 
unsigned long start_ms;                     // 初期化開始時のタイマー値を保存 

void sleep(); 
void beep();


// http://docs.blynk.cc/#blynk-firmware-blynktimer
void myTimerEvent() {
  // １回だと分からんから、複数回読むように要改良
  soil_moisture = analogRead(0); // AO==TOUTから読む
  Serial.println(soil_moisture);

#ifdef USE_WIFI_MODE
  Blynk.virtualWrite(V0, soil_moisture);
#endif
  if (soil_moisture > 0 && soil_moisture <= t1) {
#ifdef USE_WIFI_MODE
    Blynk.virtualWrite(V1, status[0]);
#endif
    Serial.println(status[0]);
  } else if (soil_moisture > t1 && soil_moisture <= t2) {
#ifdef USE_WIFI_MODE
    Blynk.virtualWrite(V1, status[1]);
#endif
    Serial.println(status[1]);
  } else if (soil_moisture > t2 && soil_moisture <= 1024) {
#ifdef USE_WIFI_MODE
    Blynk.virtualWrite(V1, status[2]);
#endif
    Serial.println(status[2]);
    // red LED lighting
    digitalWrite(12, HIGH);
    delay(500);
    digitalWrite(12, LOW);
    // and beep
    beep();
  } else {
#ifdef USE_WIFI_MODE
    Blynk.virtualWrite(V1, status[3]);
#endif
    Serial.println(status[3]);
  }
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
  
  // １回だと分からんから、複数回読むように要改良
  soil_moisture = analogRead(0);          // AO==TOUTから読む
  Serial.println(soil_moisture);

  mem = fabs(readRtcInt()-soil_moisture); // RTCメモリの温度値と比較する 
  if( WAKE_COUNT % SLEEP_N &&             // SLEEP_Nが0以外 かつ 
      mem <= DEADZONE )                   // 閾値以下 のときに 
    sleep();                              // スリープを実行

#ifdef USE_WIFI_MODE
  WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定 
  WiFi.begin(ssid,password);              // 無線LANアクセスポイントへ接続 
  while(WiFi.status() != WL_CONNECTED){   // 接続に成功するまで待つ 
    delay(100);                           // 待ち時間処理 
    waiting++;                            // 待ち時間カウンタを1加算する 
    if(waiting%10==0)Serial.print('.');   // 進捗表示 
    if(waiting > 300) sleep();            // 300回(30秒)を過ぎたらスリープ 
  } 
  Serial.println(WiFi.localIP());         // 本機のIPアドレスをシリアル出力   

  //Blynk.begin(blynk_auth, ssid, password);
  Blynk.config(blynk_auth); // or Blynk.config(auth,server,port);
  bool result = Blynk.connect();
  Wire.begin(); // ???
#endif
}

void loop() {
  WAKE_COUNT=1;                           // 起動回数をリセット 
  writeRtcInt(soil_moisture);             // 湿度値をRTCメモリへ保存 
  delay(200);                             // 送信待ち時間 
  sleep();
}

void sleep(){ 
  digitalWrite(PIN_LED,LOW);              // LEDの消灯 
  ESP.deepSleep(SLEEP_P,WAKE_RF_DEFAULT); // スリープモードへ移行する 
  while(1){                               // 繰り返し処理 
    delay(100);                           // 100msの待ち時間処理 
  }                                       // 繰り返し中にスリープへ移行 
}

void beep() {
  for (int i=0;i<3;i++) {
    analogWrite(PIN_BZR, 500);
    analogWriteFreq(440); // ラの音
    delay(500);
    analogWrite(PIN_BZR, 0);
    delay(500);
  }
}
