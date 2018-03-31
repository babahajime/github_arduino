// MainOTA.ino

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space

BlynkTimer timer;

static char* status[] = {"乾いています", "ちょうど良い", "濡れています", "不明"};
static int soil_moisture = 0; // 初期値0
static int t1 = 300; // 初期値threshold #1
static int t2 = 700; // 初期値threshold #2


// http://docs.blynk.cc/#blynk-firmware-blynktimer
void myTimerEvent() {
//  BLYNK_READ(V0);
//  BLYNK_READ(V1);
//  BLYNK_WRITE(V2);
//  BLYNK_WRITE(V3);
//}
  // 生値0-1024
//BLYNK_READ(V0) {
  // １回だと分からんから、複数回読むように要改良
  soil_moisture = analogRead(0); // AO==TOUTから読む
  Serial.println(soil_moisture);
  Blynk.virtualWrite(V0, soil_moisture);
//}

  // ステータス文字列
//BLYNK_READ(V1) {
  if (soil_moisture > 0 && soil_moisture <= t1) {
    Blynk.virtualWrite(V1, status[0]);
    Serial.println(status[0]);
    // green LED lighting
//    digitalWrite(14, HIGH);
//    digitalWrite(12, LOW);
  } else if (soil_moisture > t1 && soil_moisture <= t2) {
    Blynk.virtualWrite(V1, status[1]);
    Serial.println(status[1]);
    // both LED extinction
//    digitalWrite(14, LOW);
//    digitalWrite(12, LOW);
  } else if (soil_moisture > t2 && soil_moisture <= 1024) {
    Blynk.virtualWrite(V1, status[2]);
    Serial.println(status[2]);
    // red LED lighting
//    digitalWrite(14, LOW);
//    digitalWrite(12, HIGH);
  } else {
    Blynk.virtualWrite(V1, status[3]);
    Serial.println(status[3]);
    // both LED lighting
//    digitalWrite(14, HIGH);
//    digitalWrite(12, HIGH);
  }
}

BLYNK_WRITE(V2) {
  //スマホ側 Blynk アプリで設定したスライダー値の受信
  t1 = param[0].asInt();
  Serial.printf("Threshold#1 changed to: %d\r\n", t1);
}

BLYNK_WRITE(V3) {
  //スマホ側 Blynk アプリで設定したスライダー値の受信
  t2 = param[0].asInt();
  Serial.printf("Threshold#2 changed to: %d\r\n", t2);
}


void OTA_setup() {
  // put your setup code here, to run once:
  // LED設定
  //pinMode(14, OUTPUT);  // GPIO#14 -- green LED
  //pinMode(12, OUTPUT);  // GPIO#12 -- red LED

  //Blynk.begin(blynk_auth, ssid, password);
  Blynk.config(blynk_auth); // or Blynk.config(auth,server,port);
  bool result = Blynk.connect();
  Wire.begin();
  // Blynk Setup a function to be called every second
  timer.setInterval(60*1000L, myTimerEvent); // every 10 seconds
}

void OTA_loop() {
  // put your main code here, to run repeatedly:
  Blynk.run();
  timer.run();  // Initiates BlynkTimer
  //delay(1*1000L); // every 10 seconds

  //DEEP SLEEPモード突入命令
//  Serial.println("DEEP SLEEP START!!");
  //1:μ秒での復帰までのタイマー時間設定  2:復帰するきっかけの設定（モード設定）
//  ESP.deepSleep(60 * 1000 * 1000 , WAKE_RF_DEFAULT);
  //deepsleepモード移行までのダミー命令
//  delay(1000);
}

