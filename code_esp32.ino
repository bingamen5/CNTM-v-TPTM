#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>

//===================== CẤU HÌNH =====================
#define RST_PIN  13         // Chân RST của RFID
#define SS_PIN   15         // Chân SDA (SS) của RFID
#define BUZZER   32         // Chân buzzer

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD 16x2, địa chỉ 0x27 

// WiFi
const char* WIFI_SSID = "FPT Telecom";
const char* WIFI_PASSWORD = "123456789";

// Google Apps Script URL (đảm bảo không có dấu hỏi ?uid= ở cuối)
const String sheet_url = "https://script.google.com/macros/s/AKfycbzATjMyBNj55a2MHd5EYrx773KOKSmx7ISGMzyjsPuKW8g7unVP9q0kIN7yk-7fFk1q/exec";

// Danh sách UID hợp lệ
const byte VALID_UIDS[3][4] = {
  {0x0E, 0x39, 0x4A, 0x01},
  {0x87, 0x68, 0x48, 0x01},
  {0x0B, 0xDF, 0xC9, 0x05}
};

//===================== SETUP =====================
void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(1000);

  // Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  lcd.clear();
  lcd.print("WiFi Connected");

  pinMode(BUZZER, OUTPUT);
  SPI.begin();  
  mfrc522.PCD_Init();
  delay(1000);
}

//===================== LOOP =====================
void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan your card");

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String uidStr = "";
  byte* uid = mfrc522.uid.uidByte;

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (uid[i] < 0x10) uidStr += "0";  // Thêm số 0 trước UID nếu cần
    uidStr += String(uid[i], HEX);   // Chuyển đổi mỗi byte UID thành chuỗi Hex
  }
  uidStr.toUpperCase(); // Đảm bảo chuỗi UID là chữ hoa

  Serial.print("UID: ");
  Serial.println(uidStr);  // Hiển thị UID trên Serial Monitor

  // Kiểm tra UID có hợp lệ không và hiển thị thông báo
  if (isValidUID(uid)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Correct Card");

    // Buzzer: 2 tiếng ngắn
    for (int i = 0; i < 2; i++) {
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      delay(500);
    }

    // Gửi yêu cầu lên Google Sheet
    sendUIDToGoogleSheet(uidStr);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Incorrect Card");

    // Buzzer: 1 tiếng dài 3s
    digitalWrite(BUZZER, HIGH);
    delay(3000);
    digitalWrite(BUZZER, LOW);
  }

  delay(3000);  // Chờ 3s trước khi quét tiếp
}

//===================== GỬI YÊU CẦU LÊN GOOGLE SHEET =====================
void sendUIDToGoogleSheet(String uidStr) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();  // Tắt kiểm tra chứng chỉ SSL để tránh lỗi không thể kết nối

    HTTPClient https;
    String fullURL = sheet_url + "?uid=" + uidStr;  // Gửi yêu cầu GET với UID

    Serial.println("Sending to: " + fullURL);

    if (https.begin(client, fullURL)) {  // Khởi tạo kết nối HTTPS
      int httpCode = https.GET();  // Gửi yêu cầu GET

      if (httpCode > 0) {
        Serial.printf("HTTP GET code: %d\n", httpCode);
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Data Sent");
      } else {
        Serial.println("Send failed: " + https.errorToString(httpCode));
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Send Failed");
      }
      https.end();  // Kết thúc phiên HTTP
    } else {
      Serial.println("HTTPS begin failed");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("HTTPS Failed");
    }
  } else {
    Serial.println("WiFi disconnected");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No WiFi");
  }
}

//===================== KIỂM TRA UID HỢP LỆ =====================
bool isValidUID(byte* uid) {
  for (int i = 0; i < 3; i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (uid[j] != VALID_UIDS[i][j]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}
