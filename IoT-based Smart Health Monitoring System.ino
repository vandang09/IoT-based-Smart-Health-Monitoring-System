// =============================================================
// PHẦN 1: CẤU HÌNH BLYNK & THÔNG BÁO
// =============================================================
#define BLYNK_TEMPLATE_ID "TMPL6fku0eK5z"
#define BLYNK_TEMPLATE_NAME "nhiet do co the"
#define BLYNK_AUTH_TOKEN "zYTOhv2f9RmsEXZ3fAV8PQJrUoUO_aDP"
#define BLYNK_PRINT Serial

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MLX90614.h>
#include "MAX30100_PulseOximeter.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// =============================================================
// PHẦN 2: CẤU HÌNH
// =============================================================
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "lx911_home";
char pass[] = "????????";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
PulseOximeter pox;

#define LED_SOT D8    
#define LED_OK  D0    
#define BUZZER  D5    

#define TEMP_HIGH 37.5        
#define HEART_RATE_HIGH 100.0 
#define SPO2_LOW 90.0         

// Thời gian cập nhật (Non-blocking)
#define OLED_UPDATE_MS 500     
#define BLYNK_SEND_MS  2000     

// =============================================================
// PHẦN 3: BIẾN TOÀN CỤC
// =============================================================
uint32_t tsLastOled = 0;
uint32_t tsLastBlynk = 0;
uint32_t lastBuzzerTime = 0;
uint32_t lastNotificationTime = 0;

bool buzzerState = false;
double temp_obj = 0;
double calibration = 2.36; 
float heartRate = 0;
float SpO2 = 0;

bool max30100_working = false;
bool isAlarming = false;
String statusShort = "OK"; // Trạng thái ngắn gọn (OK/SOT/TIM/OXY)

void onBeatDetected() {
  // Callback nhịp tim
}

// =============================================================
// PHẦN 4: CÁC HÀM CHỨC NĂNG
// =============================================================

void connectWiFi() {
  display.clearDisplay();
  display.setTextSize(1); display.setCursor(0,0); 
  display.println("Dang ket noi...");
  display.display();
  
  WiFi.begin(ssid, pass);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500); attempts++;
    display.print("."); display.display();
  }
  
  display.clearDisplay();
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.config(auth);
    Blynk.connect();
    display.println("WiFi: OK");
  } else {
    display.println("WiFi: Offline");
  }
  display.display();
  delay(1000);
}

void readSensors() {
  temp_obj = mlx.readObjectTempC() + calibration;
  // HeartRate và SpO2 được cập nhật liên tục trong loop
}

void checkAlarms() {
  isAlarming = false;
  statusShort = "GOOD"; // Mặc định là Tốt

  // Kiểm tra logic (Ưu tiên lỗi nghiêm trọng)
  if (SpO2 > 0 && SpO2 < SPO2_LOW) {
    statusShort = "LOW O2"; // Oxi thấp
    isAlarming = true;
  }
  else if (temp_obj > TEMP_HIGH) {
    statusShort = "SOT !"; // Sốt
    isAlarming = true;
  }
  else if (heartRate > HEART_RATE_HIGH && SpO2 > 60) {
    statusShort = "TIM !"; // Tim nhanh
    isAlarming = true;
  }

  // Gửi thông báo mỗi 30s
  if (WiFi.status() == WL_CONNECTED && Blynk.connected()) {
    if (millis() - lastNotificationTime > 30000 && isAlarming) { 
        String msg = "CANH BAO: " + statusShort + " HR:" + String((int)heartRate);
        Blynk.logEvent("tim_nhanh", msg);
        lastNotificationTime = millis();
    }
  }
}

void handleOutputs() {
  if (isAlarming) {
    digitalWrite(LED_OK, LOW); digitalWrite(LED_SOT, HIGH);
    if (millis() - lastBuzzerTime > 200) {
      lastBuzzerTime = millis();
      buzzerState = !buzzerState;
      if (buzzerState) tone(BUZZER, 2000); else noTone(BUZZER);
    }
  } else {
    digitalWrite(LED_SOT, LOW); digitalWrite(LED_OK, HIGH); noTone(BUZZER);
  }
}

void sendToBlynk() {
  if (WiFi.status() == WL_CONNECTED && Blynk.connected()) {
    Blynk.virtualWrite(V4, temp_obj);
    if (heartRate > 0 && SpO2 > 50) { 
       Blynk.virtualWrite(V2, heartRate);
       Blynk.virtualWrite(V3, SpO2);
    }
  }
}

// --- HÀM HIỂN THỊ GIAO DIỆN MỚI (GỌN GÀNG) ---
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Kẻ khung lưới chia 4 phần
  display.drawLine(64, 0, 64, 64, WHITE);  // Đường dọc giữa
  display.drawLine(0, 32, 128, 32, WHITE); // Đường ngang giữa

  // --- GÓC 1: NHIỆT ĐỘ (Trái-Trên) ---
  display.setTextSize(1); display.setCursor(2, 2); display.print("TEMP");
  display.setTextSize(2); display.setCursor(2, 14); 
  display.print(temp_obj, 1); 
  // display.setTextSize(1); display.print("C"); // Bỏ chữ C cho đỡ chật

  // --- GÓC 2: NHỊP TIM (Phải-Trên) ---
  display.setTextSize(1); display.setCursor(68, 2); display.print("HEART");
  display.setTextSize(2); display.setCursor(68, 14);
  if (heartRate > 0 && SpO2 > 50) display.print((int)heartRate); 
  else display.print("--");

  // --- GÓC 3: SpO2 (Trái-Dưới) ---
  display.setTextSize(1); display.setCursor(2, 35); display.print("SpO2 %");
  display.setTextSize(2); display.setCursor(2, 47);
  if (SpO2 > 0 && SpO2 > 50) display.print((int)SpO2); 
  else display.print("--");

  // --- GÓC 4: TRẠNG THÁI (Phải-Dưới) ---
  display.setTextSize(1); display.setCursor(68, 35); display.print("STATUS");
  
  // Nếu đang báo động thì chữ STATUS nhấp nháy hoặc in đậm
  display.setTextSize(2); display.setCursor(68, 47);
  display.print(statusShort);

  // Biểu tượng WiFi nhỏ xíu ở góc 1 (nếu cần)
  /*
  if(WiFi.status() == WL_CONNECTED) display.drawPixel(127,0, WHITE);
  */

  display.display();
}

// =============================================================
// SETUP & LOOP
// =============================================================
void setup() {
  Serial.begin(115200);
  pinMode(LED_SOT, OUTPUT); pinMode(LED_OK, OUTPUT); pinMode(BUZZER, OUTPUT);

  Wire.begin(D2, D1);
  Wire.setClock(100000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println(F("Loi OLED!"));
  display.clearDisplay(); display.display();

  connectWiFi();
  mlx.begin();
  
  if (pox.begin()) {
    pox.setIRLedCurrent(MAX30100_LED_CURR_27_1MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
    max30100_working = true;
  } else {
    max30100_working = false;
    statusShort = "ERR";
  }
}

void loop() {
  // 1. Cập nhật MAX30100 liên tục
  if (max30100_working) {
    pox.update();
    heartRate = pox.getHeartRate();
    SpO2 = pox.getSpO2();
  }

  // 2. Blynk
  if (WiFi.status() == WL_CONNECTED) Blynk.run();

  // 3. Output
  handleOutputs();

  // 4. Hiển thị OLED (0.5s/lần)
  if (millis() - tsLastOled > OLED_UPDATE_MS) {
    readSensors();
    checkAlarms();
    updateDisplay();
    tsLastOled = millis();
  }

  // 5. Gửi Blynk (2s/lần)
  if (millis() - tsLastBlynk > BLYNK_SEND_MS) {
    sendToBlynk();
    tsLastBlynk = millis();
  }
}