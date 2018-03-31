// soil_moiture_blynk+ifttt.ino

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#define IFTTT_MAX_SIZE_STRING    512
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>

// Blynkで生成されるAuth Token
char auth[] = "eba203f264104d43a5b4916c3f5184ef";
// 無線ルーターのSSIDとパスワード
char ssid[] = "aterm-893793-g";
char pass[] = "35b2c26f4d6b8";

// IFTTT
char event[] = "ESP-WROOM-02_event";
char key[] = "bBbR_j_WXc27Ufaut-qKik";
//char url[] = "https://maker.ifttt.com/trigger/{event}/with/key/{key}";

char* status[] = {"乾いています", "ちょうど良い", "濡れています", "不明"};

int soil_moisture;
int t1 = 300; // 初期値threshold #1
int t2 = 700; // 初期値threshold #2


void setup() {
  // LED設定
  pinMode(14, OUTPUT);  // GPIO#14 -- green LED
  pinMode(16, OUTPUT);  // GPIO#16 -- red LED

  // put your setup code here, to run once:
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  Wire.begin(); 
}

void ifttt_post(char *v1, int v2, char *v3) {
  // URLを作成
  // maker.ifttt.com/trigger/{event}/with/key/{key}
  const char* host = "maker.ifttt.com";
  const char* event   = "ping";

  // Use WiFiClient class to create TCP connections
//  WiFiClientSecure client;
//  const int httpsPort = 443;
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
 
  // This will send POST to IFTTT server
  char str[IFTTT_MAX_SIZE_STRING] = {0};
//  char v1[16] = "";
//  char v2[16] = "";
//  char v3[16] = "";
  char header[256];
  sprintf(header, "POST /trigger/%s/with/key/%s HTTP/1.1\r\n", event, key);
  const char * host_header = "Host: maker.ifttt.com\r\n";
  char contentLen[50] = {0};
  const char * contentType = "Content-Type: application/json\r\n\r\n";
  char valueData [150] = {0};
  sprintf(valueData,"{\"value1\":\"%s\",\"value2\":\"%d\",\"value3\":\"%s\"}\r\n",v1,v2,v3);
  sprintf(contentLen,"Content-Length: %d\r\n",strlen(valueData));
  sprintf(str,"%s%s%s%s%s",header,host_header,contentLen,contentType,valueData);
   
  client.print(str);  
}


BLYNK_READ(V0) {
  // １回だと分からんから、複数回読むように要改良
  soil_moisture = analogRead(0);
  Serial.println(soil_moisture);

  //
  if (soil_moisture > 0 && soil_moisture <= t1) {
    Blynk.virtualWrite(V0, status[0]);
    ifttt_post(status[0], soil_moisture, "");
    Serial.println(status[0]);
    // green LED lighting
    digitalWrite(14, HIGH);
    digitalWrite(16, LOW);
  } else if (soil_moisture > t1 && soil_moisture <= t2) {
    Blynk.virtualWrite(V0, status[1]);
    ifttt_post(status[1], soil_moisture, "");
    Serial.println(status[1]);
    // both LED extinction
    digitalWrite(14, LOW);
    digitalWrite(16, LOW);
  } else if (soil_moisture > t2 && soil_moisture <= 1024) {
    Blynk.virtualWrite(V0, status[2]);
    ifttt_post(status[2], soil_moisture, "");
    Serial.println(status[2]);
    // red LED lighting
    digitalWrite(14, LOW);
    digitalWrite(16, HIGH);
  } else {
    Blynk.virtualWrite(V0, status[3]);
    ifttt_post(status[3], soil_moisture, "");
    Serial.println(status[3]);
    // green LED lighting
    digitalWrite(14, HIGH);
    digitalWrite(16, LOW);
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
  delay(10000); // every 10 seconds
}

