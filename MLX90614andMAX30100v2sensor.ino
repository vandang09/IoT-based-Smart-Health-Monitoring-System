#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// --- SỬA LỖI: Sử dụng thư viện đã cài đặt (MAX30100lb) ---
#include <MAX30100lb.h> 

#define LED_SOT D8   // GPIO15
#define LED_OK D0
#define BUZZER D5    

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1   
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Cảm biến nhiệt độ
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// --- SỬA LỖI: Sử dụng đúng Class 'MAX30100lb' ---
MAX30100lb pox;
// --- 
// --- Biến lưu giá trị (thư viện này dùng uint32_t) ---
uint32_t ir, red; 
// ------------------------------------

double temp_amb;
double temp_obj;
double calibration = 2.36; // hiệu chỉnh nếu cần

// --- Biến thời gian ---
#define REPORTING_PERIOD_MS 1000 
uint32_t tsLastReport = 0;


void setup() {
  Serial.begin(9600);
  pinMode(LED_SOT, OUTPUT);
  pinMode(LED_OK, OUTPUT);
  pinMode(BUZZER, OUTPUT);
    
  // Khởi động cảm biến MLX90614
  if (!mlx.begin()) {
    Serial.println("Không tìm thấy cảm biến MLX90614!");
    while (1);
  }

  // Khởi động màn hình OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED không khởi động được");
    while (1);
  }

  // --- Giao diện khởi động ---
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(25, 10);
  display.println("Thermometer");
  display.setCursor(20, 25);
  display.println("Initializing...");
  display.display();
  delay(1000); 

  // --- Khởi động MAX30100lb ---
  Serial.println("Khoi dong MAX30100lb...");
  // --- SỬA LỖI: Hàm begin() của thư viện này không cần tham số ---
  if (!pox.begin()) { 
      Serial.println("Khong tim thay MAX30100!");
      display.clearDisplay();
      display.setCursor(0, 10);
      display.println("MAX30100 ERROR");
      display.display();
      while (1);
  }
  Serial.println("MAX30100 OK.");
  
  // --- Thông báo sẵn sàng ---
  display.clearDisplay();
  display.setCursor(25, 15);
  display.println(" System Ready");
  display.display();
  delay(1500);
}

void loop() {
  
  // --- SỬA LỖI: Dùng đúng hàm của MAX30100lb ---
  pox.update(); // Cần gọi update() liên tục
  ir = pox.getIR();
  red = pox.getRed();
  // ------------------------------------------

  // Dùng millis() để cập nhật giá trị và màn hình
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    
    // 1. Đọc nhiệt độ từ MLX90614
    temp_amb = mlx.readAmbientTempC();
    temp_obj = mlx.readObjectTempC();
    double temp_obj_calibrated = temp_obj + calibration;

    // 2. Ghi ra Serial
    Serial.print("Ambient Temp: "); Serial.print(temp_amb); Serial.println(" C");
    Serial.print("Object Temp: "); Serial.print(temp_obj_calibrated); Serial.println(" C");
    Serial.print("IR: "); Serial.print(ir);
    Serial.print(", RED: "); Serial.println(red);

    // 4. Hiển thị lên OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(30, 0);
    display.println(" Nhom 7");

    display.setCursor(5, 15); 
    display.print("Nhiet do: ");
    display.print(temp_obj_calibrated, 1);
    display.print((char)247); 
    display.println("C");
    
    // --- Hiển thị giá trị thô ---
    display.setCursor(5, 28); 
    display.print("IR:  ");
    display.println(ir);
    
    display.setCursor(5, 41); 
    display.print("RED: ");
    display.println(red);
    // ----------------------------------

    // 5. Kiểm tra sốt (dựa trên nhiệt độ)
    display.setCursor(5, 54); 
    if (temp_obj_calibrated > 37.0) {
      display.setTextColor(WHITE);
      display.println("Status: Sot!");
      digitalWrite(LED_SOT, HIGH);
      digitalWrite(LED_OK, LOW); 
      digitalWrite(BUZZER, HIGH);
    } else {
      display.setTextColor(WHITE);
      display.println("Status: Binh thuong");
      digitalWrite(LED_SOT, LOW);
      digitalWrite(LED_OK, HIGH);   
      digitalWrite(BUZZER, LOW); 
    }

    display.display();
    
    // Đặt lại mốc thời gian
    tsLastReport = millis();
  }
}