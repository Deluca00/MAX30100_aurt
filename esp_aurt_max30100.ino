#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <U8g2lib.h>
#define REPORTING_PERIOD_MS 1000

// Sử dụng lớp U8G2 cho SH1106
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

PulseOximeter pox;
float BPM, SpO2;
uint32_t tsLastReport = 0;

void setup()
{
    Serial.begin(115200); // UART cho debug
    Serial2.begin(9600, SERIAL_8N1, 3, 1); // UART2 (TX: GPIO 17, RX: GPIO 16)
    
    oled.begin();
    oled.setPowerSave(0); // Bật màn hình
    oled.clearBuffer(); // Dùng clearBuffer thay vì clearDisplay
    oled.setFont(u8g2_font_chroma48medium8_8r); // Chọn font cho màn hình
    
    oled.drawStr(0, 10, "Initializing..");
    oled.sendBuffer();

    Serial.print("Initializing Pulse Oximeter..");
    if (!pox.begin())
    {
        Serial.println("FAILED");
        oled.clearBuffer();
        oled.drawStr(30, 30, "FAILED");
        oled.sendBuffer();
        for (;;);
    }
    else
    {
        oled.clearBuffer();
        oled.drawStr(30, 30, "SUCCESS");
        oled.sendBuffer();
        Serial.println("SUCCESS");
        pox.setOnBeatDetectedCallback(onBeatDetected);
    }
}

void onBeatDetected()
{
    Serial.println("Beat Detected!");
}

// Các icon bitmap (8x8) cho các trạng thái
const uint8_t warning_icon[] = {
  0b00111100, // Row 1:  ..####..
  0b01000010, // Row 2:  .#....#.
  0b10111101, // Row 3:  #.####.# 
  0b10111101, // Row 4:  #.####.# 
  0b10011001, // Row 5:  #..##..# 
  0b10100101, // Row 6:  #.#..#.# 
  0b01011010, // Row 7:  .#.##.#. 
  0b00100100  // Row 8:  ..#..#..
};

const uint8_t neutral_icon[] = {
    0b00111100,
    0b01000010,
    0b10100101,
    0b10000001,
    0b10111101,
    0b10000001,
    0b01000010,
    0b00111100
};

const uint8_t happy_icon[] = {
    0b00111100,
    0b01000010,
    0b10100101,
    0b10000001,
    0b10100001,
    0b10011001,
    0b01000010,
    0b00111100
};

const uint8_t confused_icon[] = {
    0b00111100,
    0b01000010,
    0b10010001,
    0b10001001,
    0b10010001,
    0b10011001,
    0b01000010,
    0b00111100
};

void loop()
{
    pox.update();
    BPM = pox.getHeartRate();
    SpO2 = pox.getSpO2();
    
    if (millis() - tsLastReport > REPORTING_PERIOD_MS)
    {
        oled.clearBuffer();
        oled.setFont(u8g2_font_chroma48medium8_8r);
        oled.drawStr(30, 20, "Heart BPM");
        oled.setCursor(60, 30);
        oled.print(BPM);
        oled.drawStr(30, 50, "SpO2");
        oled.setCursor(60, 60);
        oled.print(SpO2);

        // Hiển thị icon dựa trên điều kiện
        if (SpO2 < 88) {
            oled.drawXBMP(60, 0, 8, 8, warning_icon); // Icon nguy hiểm
        } else if (SpO2 >= 92 && SpO2 <= 95 && BPM <= 120) {
            oled.drawXBMP(60, 0, 8, 8, neutral_icon); // Icon cảnh báo
        } else if (SpO2 >= 96 && BPM >= 65 && BPM <= 105) {
            oled.drawXBMP(60, 0, 8, 8, happy_icon); // Icon bình thường
        } else {
            oled.drawXBMP(60, 0, 8, 8, confused_icon); // Icon không rõ ràng
        }

        oled.sendBuffer(); // Gửi buffer để hiển thị
        
        Serial2.print("BPM:");
        Serial2.print(BPM);
        Serial2.print(",");
        Serial2.print("SpO2:");
        Serial2.println(SpO2);

        tsLastReport = millis();
    }
}
