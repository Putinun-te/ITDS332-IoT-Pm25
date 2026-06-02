#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "PMS.h"
#include <time.h> 

// นำเข้าไฟล์เสริมของ Firebase
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ================= กำหนดค่า Wi-Fi =================
#define WIFI_SSID "grace2.4Ghz"
#define WIFI_PASSWORD "grace814"

// ================= กำหนดค่า Firebase =================
#define API_KEY "AIzaSyBu5IxG3Ls4mYdsYJy1Saonfw6eF3RIfCs"
#define DATABASE_URL "https://itds332-pm25-default-rtdb.asia-southeast1.firebasedatabase.app/" 

// ================= กำหนดค่าเวลา (NTP) =================
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200; // เวลาไทย (UTC+7) = 7 * 3600
const int   daylightOffset_sec = 0;

// ตั้งค่าเซ็นเซอร์ PMS
HardwareSerial pmsSerial(2);
PMS pms(pmsSerial);
PMS::DATA data;

// ตัวแปรสำหรับ Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
unsigned long sendDataPrevMillis = 0;

// ตั้งเวลาส่งข้อมูล (แนะนำ: 3600000 = 1 ชม )
const long timerDelay = 3600000; 

void setup() {
  Serial.begin(115200);
  
  // ตั้งค่า Serial สื่อสารกับ PMS7003 ที่ขั้ว 16, 17
  pmsSerial.begin(9600, SERIAL_8N1, 16, 17);

  // 1. เชื่อมต่อ Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("กำลังเชื่อมต่อ Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nเชื่อมต่อ Wi-Fi สำเร็จ!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // 2. ตั้งค่าระบบดึงเวลาจากอินเทอร์เน็ต
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("รอซิงค์เวลาจากเซิร์ฟเวอร์...");
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("ดึงเวลาไม่สำเร็จ (อาจต้องรอสักครู่)");
  } else {
    Serial.println("ซิงค์เวลาเรียบร้อย!");
  }

  // 3. ตั้งค่าการเชื่อมต่อ Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // สมัครใช้งานแบบไม่ระบุตัวตน (ต้องไปเปิด Anonymous Auth ในเว็บ Firebase ด้วย)
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("เชื่อมต่อ Firebase สำเร็จ");
    signupOK = true;
  } else {
    Serial.printf("เชื่อมต่อ Firebase ไม่สำเร็จ: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback; 
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  // อ่านค่าจากเซ็นเซอร์ (อ่านตลอดเวลาที่ลูปทำงาน)
  if (pms.read(data)) {
    
    // ตรวจสอบว่าถึงเวลาส่งข้อมูล และ Firebase พร้อมหรือไม่
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
      sendDataPrevMillis = millis();

      // ดึงค่าฝุ่นมาพักไว้ในตัวแปร
      int pm25_value = data.PM_AE_UG_2_5;
      int pm10_value = data.PM_AE_UG_10_0;

      // ดึงเวลาปัจจุบัน
      struct tm timeinfo;
      String timeString = "กำลังโหลดเวลา..."; 
      if(getLocalTime(&timeinfo)){
        char timeStringBuff[50];
        // จัดรูปแบบเป็น 2026-04-20 22:30:00
        strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
        timeString = String(timeStringBuff);
      }

      // แสดงค่าที่อ่านได้ออกทางหน้าจอคอม
      Serial.println("====================================");
      Serial.print("เวลา: "); Serial.println(timeString);
      Serial.print("PM 2.5: "); Serial.print(pm25_value); Serial.println(" ug/m3");
      Serial.print("PM 10: ");  Serial.print(pm10_value); Serial.println(" ug/m3");
      
      // สร้างก้อนข้อมูล JSON สำหรับเตรียมส่ง
      FirebaseJson json;
      json.add("pm25", pm25_value);
      json.add("pm10", pm10_value);
      json.add("timestamp", timeString);

      // ใช้คำสั่ง pushJSON เพื่อบันทึกประวัติ (History) ไว้ใน Node ชื่อ /AirQuality/Log
      Serial.print("สถานะ Firebase: ");
      if (Firebase.RTDB.pushJSON(&fbdo, "/AirQuality/Log", &json)) {
        Serial.println("✅ บันทึกข้อมูลสำเร็จ");
      } else {
        Serial.println("❌ บันทึกพลาด: " + fbdo.errorReason());
      }
      Serial.println("====================================\n");
    }
  }
}
