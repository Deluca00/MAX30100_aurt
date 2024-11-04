#include <Firebase_ESP_Client.h> 
#include <FirebaseFS.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <U8x8lib.h>
#include <WiFi.h>
#include "addons/TokenHelper.h" // Provide the token generation process info.
#include "addons/RTDBHelper.h" 

#define REPORTING_PERIOD_MS 1000

const char* ssid "DL";
const char* password = "H";
#define FIREBASE_HOST "https://max30100-heart-oxy-default-rtdb.firebaseio.com/"
#define DATABASE_SECRET "EURst2arJ7UaySQHMp5MXCAjQl93PPvGpLZ6D96v" // Database secret

U8X8_SSD1306_128X64_NONAME_HW_I2C oled(U8X8_PIN_NONE);
PulseOximeter pox;
FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

float BPM, SpO2;
uint32_t tsLastReport = 2000;

// Task handles
TaskHandle_t PostToFirebase;
TaskHandle_t GetReadings;

void initializeOLED() {
    Serial.begin(115200);
    oled.begin();
    oled.setPowerSave(0); // Turn on the display
    oled.clearDisplay();
    oled.setFont(u8x8_font_chroma48medium8_r);
    oled.drawString(0, 0, "Initializing OLED..");
}

void initializePulseOximeter() {
    Serial.print("Initializing Pulse Oximeter..");
    if (!pox.begin()) {
        Serial.println("FAILED");
        oled.clearDisplay();
        oled.drawString(4, 4, "FAILED");
        while (true);
    } else {
        oled.clearDisplay();
        oled.drawString(4, 4, "SUCCESS");
        Serial.println("SUCCESS");
        pox.setOnBeatDetectedCallback(onBeatDetected);
    }
}

void connectWiFi() {
    Serial.print("Connecting to wifi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
}

void initializeFirebase() {
    firebaseConfig.host = FIREBASE_HOST;
    firebaseConfig.database_url = FIREBASE_HOST;
    firebaseConfig.signer.tokens.legacy_token = DATABASE_SECRET;

    Firebase.begin(&firebaseConfig, &firebaseAuth);
    Serial.println("Firebase initialized");
}

void onBeatDetected() {
    Serial.println("Beat Detected!");
}

void displayDataOnOLED(float heartRate, float spo2) {
    oled.clearDisplay();
    oled.setFont(u8x8_font_chroma48medium8_r);
    oled.drawString(4, 2, "Heart BPM");
    oled.setCursor(6, 3);
    oled.print(heartRate);
    oled.drawString(4, 5, "SpO2");
    oled.setCursor(6, 6);
    oled.print(spo2);
}

void readSensorData(void * parameter) {
    while (true) {
        pox.update();
        BPM = pox.getHeartRate();
        SpO2 = pox.getSpO2();

        displayDataOnOLED(BPM, SpO2);
        delay(1000); // Delay for sensor read interval
    }
}
  void sendDataToFirebase(void * parameter) {
    while (true) {
        if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
            Serial.print("Heart rate: ");
            Serial.print(BPM);
            Serial.print(" bpm / SpO2: ");
            Serial.print(SpO2);
            Serial.println(" %");

            // Tắt cảm biến
            pox.setIRLedCurrent(MAX30100_LED_CURR_0MA); // Tắt LED
            delay(100); // Chờ một chút trước khi gửi dữ liệu

            if (Firebase.ready()) {
                if (Firebase.RTDB.setFloat(&firebaseData, "/heartRate", BPM)) {
                    Serial.println("Heart rate data sent to Firebase");
                } else {
                    Serial.print("Firebase error: ");
                    Serial.println(firebaseData.errorReason());
                }

                if (Firebase.RTDB.setFloat(&firebaseData, "/spo2", SpO2)) {
                    Serial.println("SpO2 data sent to Firebase");
                } else {
                    Serial.print("Firebase error: ");
                    Serial.println(firebaseData.errorReason());
                }
            } else {
                Serial.println("Firebase not ready");
            }

            // Bật lại cảm biến
            pox.setIRLedCurrent(MAX30100_LED_CURR_50MA); // Thay đổi LED sang mức hiện tại mong muốn
            tsLastReport = millis(); // Cập nhật thời gian báo cáo cuối cùng
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Ngăn chặn vòng lặp chặt chẽ
    }
}



void setup() {
    initializeOLED();
    initializePulseOximeter();
    connectWiFi();
    initializeFirebase();

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(readSensorData, "GetReadings", 2048, NULL, 1, &GetReadings, 0);
    xTaskCreatePinnedToCore(sendDataToFirebase, "PostToFirebase", 2048, NULL, 2, &PostToFirebase, 1);
}
void loop() {
    // Main loop can be left empty as tasks are handling everything
}
