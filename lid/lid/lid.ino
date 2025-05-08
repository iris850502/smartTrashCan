#include <Servo.h> // 引用伺服馬達控制函式庫
#include <WiFi.h>
#include <HTTPClient.h>

#define TRIG_PIN 2   // 超音波 Trig 腳位
#define ECHO_PIN 3   // 超音波 Echo 腳位
#define SERVO_PIN 15 // SG90 馬達訊號腳位

const char* ssid = "RT-2600AC";
const char* password = "0919128163";
const char* server_url = "http://192.168.2.129:3000/api/lid/update";

Servo myservo; // 宣告一個 Servo 物件
bool lidOpen = false; // 記錄蓋子狀態

void setup() {
  Serial.begin(9600); // 打開序列埠方便除錯
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  myservo.attach(SERVO_PIN, 500, 2500);
  myservo.write(0); // 預設馬達在 0 度（蓋上）

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected!!!");
}

void loop() {
  float distance = measureDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance == -1) {
    return ; 
  }

/*  if (distance > 0 && distance < 20) {  // 距離小於 20cm 就開蓋
    if(!lidOpen){ //只有當蓋子是關著的時候才開蓋
      myservo.write(90);
      lidOpen = true;
      Serial.println("Lid opened");
    }*/
/*    myservo.write(90); // 馬達轉到90度
    delay(3000);       // 維持3秒
    myservo.write(0);  // 關回來
    delay(500);        // 稍微等一下，避免誤觸*/
/*  }else if(distance >= 20){
    if(lidOpen){ //只有當蓋子是開著的時候才關蓋
      myservo.write(0);
      lidOpen = false;
      Serial.println("Lid closed");
    }
  }*/
  if (distance < 20) {
    if (!lidOpen) {
      myservo.write(90);
      lidOpen = true;
      sendLidStatus("open");
      Serial.println("Lid opened");
    }
  } else {
    if (lidOpen) {
      myservo.write(0);
      lidOpen = false;
      sendLidStatus("closed");
      Serial.println("Lid closed");
    }
  }
   delay(1000); // 每1秒測一次
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 最多等30ms，防止死等

  if (duration == 0) {
    return -1; // timeout，量不到
  }

  float distance = duration * 0.0343 / 2.0; // 公式換算距離(cm)
  return distance;
}

void sendLidStatus(const char* status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(server_url);
    http.addHeader("Content-Type", "application/json");

    String payload = String("{\"status\":\"") + status + "\"}";
    int httpResponseCode = http.POST(payload);
    http.end();

    Serial.print("Lid status POST: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.println("WiFi not connected");
  }
}

