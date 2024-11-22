

#define ERA_DEBUG
#define DEFAULT_MQTT_HOST "mqtt1.eoh.io"
#define ERA_AUTH_TOKEN "5e9aa423-c97b-411e-9bd6-d6fd3f571137"

#include <Arduino.h>
#include <ERa.hpp>
#include <ERa/ERaTimer.hpp>
// #include <Wire.h>
// #include <SPI.h>
//  #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

//  #define SCREEN_WIDTH 128    // OLED display width, in pixels
//  #define SCREEN_HEIGHT 64    // OLED display height, in pixels
//  // Tạo đối tượng màn hình
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
//  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

int sensorPin = 34;
const int RH = 19;
const int RL = 18;
const int RM = 5;
const int Stop = 21;
const int MAN_PIN = 26;  // Chân IO25 cho chế độ MAN
const int AUTO_PIN = 25; // Chân IO26 cho chế độ AUTO
float height;
float c, v;
int H, M, L, S, CD;
int chedo_auto = 0, chedo_man = 0;

float h = 0;                        // Biến lưu giá trị chiều cao trung bình sau 1 giây
unsigned long prevMillis = 0;       // Thời gian lần cuối cập nhật
const unsigned long interval = 100; // 0,1 giây
float heightArray[10];              // Mảng lưu các giá trị trong 1 giây (0,1 giây/lần -> 10 giá trị)
int currentIndex = 0;               // Chỉ số mảng

//_____________wifi congig_______________//
#define BUTTON_PIN 0
#if defined(BUTTON_PIN)
// Active low (false), Active high (true)
#define BUTTON_INVERT false
#define BUTTON_HOLD_TIMEOUT 3000UL

// This directive is used to specify whether the configuration should be erased.
// If it's set to true, the configuration will be erased.
#define ERA_ERASE_CONFIG false
#endif
#if defined(BUTTON_PIN)
#include <pthread.h>
#include <ERa/ERaButton.hpp>
#endif
const char ssid[] = "ZenThenk";
const char pass[] = "vanthanh04";

void dokhoanhgcach_2(void);
void dokhoangcach(void);
void sosanh(void);
void chedo(void);
void bandau(void);

WiFiClient mbTcpClient;
#if defined(BUTTON_PIN)
ERaButton button;
pthread_t pthreadButton;
static void *handlerButton(void *args)
{
  for (;;)
  {
    button.run();
    ERaDelay(10);
  }
  pthread_exit(NULL);
}
#if ERA_VERSION_NUMBER >= ERA_VERSION_VAL(1, 2, 0)
static void eventButton(uint8_t pin, ButtonEventT event)
{
  if (event != ButtonEventT::BUTTON_ON_HOLD)
  {
    return;
  }
  ERa.switchToConfig(ERA_ERASE_CONFIG);
  (void)pin;
}
#else
static void eventButton(ButtonEventT event)
{
  if (event != ButtonEventT::BUTTON_ON_HOLD)
  {
    return;
  }
  ERa.switchToConfig(ERA_ERASE_CONFIG);
}
#endif

void initButton()
{
  pinMode(BUTTON_PIN, INPUT);
  button.setButton(BUTTON_PIN, digitalRead, eventButton,
                   BUTTON_INVERT)
      .onHold(BUTTON_HOLD_TIMEOUT);
  pthread_create(&pthreadButton, NULL, handlerButton, NULL);
}
#endif

ERA_CONNECTED()
{
  ERA_LOG("ERa", "ERa connected!");
  Serial.println("ERa connected!");
}
ERA_DISCONNECTED()
{
  bandau();
  ERA_LOG("ERa", "ERa disconnected!");
  Serial.println("ERa disconnected!");
}
//_______________wifi-confic-end________________________//

/* Chế độ MAN */
ERA_WRITE(V3)
{
  if (digitalRead(MAN_PIN) == HIGH && digitalRead(AUTO_PIN) == LOW)
  {
    //  Serial.print("Man");
    float value = param.getInt();
    digitalWrite(RL, value);
    Serial.print("Bien tan chay toc do cham ");
    Serial.println(value);
    ERa.virtualWrite(V3, value);
  }
}

ERA_WRITE(V4)
{
  if (digitalRead(MAN_PIN) == HIGH && digitalRead(AUTO_PIN) == LOW)
  {
    // Serial.print("Man");
    float value = param.getInt();
    digitalWrite(RM, value);
    Serial.print("Bien tan chay toc do trung binh ");
    Serial.println(value);
    ERa.virtualWrite(V4, value);
  }
}

ERA_WRITE(V5)
{
  if (digitalRead(MAN_PIN) == HIGH && digitalRead(AUTO_PIN) == LOW)
  {
    // Serial.print("Man");
    float value = param.getInt();
    digitalWrite(RH, value);
    Serial.print("Bien tan chay toc do cao ");
    Serial.println(value);
    ERa.virtualWrite(V5, value);
  }
}

ERA_WRITE(V6)
{
  if (digitalRead(MAN_PIN) == HIGH && digitalRead(AUTO_PIN) == LOW)
  {
    float value = param.getInt();
    // Serial.print("Man");
    digitalWrite(Stop, value);
    Serial.print("Bien tan dung  ");
    Serial.println(value);
    ERa.virtualWrite(V6, value);
  }
}
ERaTimer timer;
/* This function print uptime every second */
void timerEvent()
{
  ERa.virtualWrite(V0, v);
  ERa.virtualWrite(V1, c);
  ERa.virtualWrite(V2, h);
  ERa.virtualWrite(V7, chedo_man);
  ERa.virtualWrite(V8, chedo_auto);

  // Serial.println("Online");
  ERA_LOG("Timer", "Uptime: %d", ERaMillis() / 1000L);
}

void setup()
{

  Serial.begin(115200);
  pinMode(RH, OUTPUT);
  pinMode(RM, OUTPUT);
  pinMode(RL, OUTPUT);
  pinMode(Stop, OUTPUT);
  pinMode(MAN_PIN, INPUT);
  pinMode(AUTO_PIN, INPUT);

  digitalWrite(RH, LOW);
  digitalWrite(RM, LOW);
  digitalWrite(RL, LOW);
  digitalWrite(Stop, LOW);
  digitalWrite(MAN_PIN, LOW);
  digitalWrite(AUTO_PIN, LOW);

  // //Khởi tạo màn hình OLED

  // display.clearDisplay();
  // // Hiển thị text
  // display.setTextSize(1);              // Cỡ chữ
  // display.setTextColor(SSD1306_WHITE); // Màu chữ
  // display.setCursor(0, 10);            // Vị trí bắt đầu vẽ
  // display.println(F("Hello, ESP32!"));
  // display.display(); // Cập nhật màn hình

  // display.println("------------start_oled----------\n\r");
  // delay(1000);

  ERa.begin(ssid, pass);

  //  timer.setInterval(1000L, timerEvent);
}

void loop()
{
  dokhoanhgcach_2();
  chedo();
  ERa.run();
  /// timer.run();
}
void dokhoanhgcach_2(void)
{
  unsigned long currentMillis = millis();

  if (currentMillis - prevMillis >= interval)
  {
    prevMillis = currentMillis;

    // Đọc giá trị cảm biến và tính chiều cao
    int sensorValue = analogRead(sensorPin);
    float voltage = sensorValue * (5.0 / 4095.0) * 0.72;
    v = voltage;
    float current = (voltage) / 250.0; // Tính dòng điện qua điện trở 250Ω
    c = current;
    float height = -118.64 * voltage + 212.39;

    // Lưu giá trị chiều cao vào mảng
    heightArray[currentIndex] = height;
    currentIndex++;

    // Nếu mảng đầy (sau 1 giây)
    if (currentIndex >= 10)
    {
      currentIndex = 0; // Reset chỉ số

      // Tính trung bình
      float sum = 0;
      for (int i = 0; i < 10; i++)
      {
        sum += heightArray[i];
      }

      h = sum / 10.0;

      // Hiển thị kết quả
      Serial.print("Chiều cao trung bình: ");
      Serial.println(h);
    }
  }
}
void sosanh(void)
{
  // Kiểm tra các khoảng giá trị của height
  if (h > -5 && h <= 25)
  {
    Serial.println("RH");
    digitalWrite(RH, HIGH);
    digitalWrite(RM, LOW);
    digitalWrite(RL, LOW);
    digitalWrite(Stop, LOW);
  }
  else if (h > 25 && h <= 50)
  {
    Serial.println("RM");
    digitalWrite(RM, HIGH);
    digitalWrite(RH, LOW);
    digitalWrite(RL, LOW);
    digitalWrite(Stop, LOW);
  }
  else if (h > 50 && h <= 65)
  {
    Serial.println("RL");
    digitalWrite(RL, HIGH);
    digitalWrite(RH, LOW);
    digitalWrite(RM, LOW);
    digitalWrite(Stop, LOW);
  }
  else if (h > 70)
  {
    Serial.println("Stop");
    digitalWrite(Stop, HIGH);
    digitalWrite(RH, LOW);
    digitalWrite(RM, LOW);
    digitalWrite(RL, LOW);
  }
  else
  {
    // Trường hợp giá trị height không hợp lệ hoặc ngoài khoảng
    Serial.println("Ngoài dải đo");
    digitalWrite(Stop, LOW);
    digitalWrite(RH, LOW);
    digitalWrite(RM, LOW);
    digitalWrite(RL, LOW);
  }
}

void chedo(void)
{
  if (digitalRead(AUTO_PIN) == HIGH && digitalRead(MAN_PIN) == LOW)
  {
    sosanh();
    dokhoanhgcach_2();
    // Serial.println("Auto");
    // delay(2000);
    chedo_auto = 1;
    chedo_man = 0;
    // bandau();
  }
  else if (digitalRead(MAN_PIN) == HIGH && digitalRead(AUTO_PIN) == LOW)
  {
    // Serial.println("Man");
    dokhoanhgcach_2();
    // delay(2000);
    chedo_auto = 0;
    chedo_man = 1;
    // bandau();
  }
  else
  {
    // Serial.println("Khong hoat dong o che do nao ca");
    // delay(2000);
    bandau();
    chedo_auto = 0;
    chedo_man = 0;
    return;
  }
}

void bandau(void)
{

  digitalWrite(RH, LOW);
  digitalWrite(RM, LOW);
  digitalWrite(RL, LOW);
  digitalWrite(Stop, LOW);
}
// void khung_oled()
// {
//   display.drawLine(0, 10, 128, 10, SSD1306_WHITE); //__ hang 1
//   display.drawLine(0, 20, 128, 20, SSD1306_WHITE); //__ hang 2
//   display.drawLine(0, 30, 128, 30, SSD1306_WHITE); //__ hang 3
//   display.drawLine(0, 40, 128, 40, SSD1306_WHITE); //__ hang 4
//   display.drawLine(34, 0, 34, 40, SSD1306_WHITE);  // |
// }