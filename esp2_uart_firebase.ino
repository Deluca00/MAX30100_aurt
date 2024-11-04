#include <Firebase_ESP_Client.h> 
#include <FirebaseFS.h>
#include <WiFi.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

const char* ssid = "DL";
const char* password = "123456789";
#define FIREBASE_HOST "https://max30100-heart-oxy-default-rtdb.firebaseio.com/"
#define DATABASE_SECRET "EURst2arJ7UaySQHMp5MXCAjQl93PPvGpLZ6D96v"

FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

void setup() {
    Serial.begin(115200); // UART debug
    Serial2.begin(9600, SERIAL_8N1, 3, 1); // UART2 (TX: GPIO 17, RX: GPIO 16)
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());

    firebaseConfig.host = FIREBASE_HOST;
    firebaseConfig.database_url = FIREBASE_HOST;
    firebaseConfig.signer.tokens.legacy_token = DATABASE_SECRET;
    Firebase.begin(&firebaseConfig, &firebaseAuth);
    Serial.println("Firebase initialized");
}

void loop() {
    if (Serial2.available()) {
        String data = Serial2.readStringUntil('\n'); // Đọc dữ liệu từ UART
        Serial.print("Received: ");
        Serial.println(data);

        int bpmIndex = data.indexOf("BPM:");
        int spo2Index = data.indexOf("SpO2:");
        
        if (bpmIndex != -1 && spo2Index != -1) {
            float bpm = data.substring(bpmIndex + 4, spo2Index - 1).toFloat();
            float spo2 = data.substring(spo2Index + 5).toFloat();
            
            if (Firebase.ready()) {
                if (Firebase.RTDB.setFloat(&firebaseData, "/heartRate", bpm)) {
                    Serial.println("Heart rate data sent to Firebase");
                } else {
                    Serial.print("Firebase error: ");
                    Serial.println(firebaseData.errorReason());
                }
                if (Firebase.RTDB.setFloat(&firebaseData, "/spo2", spo2)) {
                    Serial.println("SpO2 data sent to Firebase");
                } else {
                    Serial.print("Firebase error: ");
                    Serial.println(firebaseData.errorReason());
                }
            }
        }
    }
}
