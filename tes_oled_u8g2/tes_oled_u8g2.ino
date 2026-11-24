/**
 * TES OLED TAHAP 2 (pakai library U8g2) - Bluino ESP32 IoT Starter Shield
 * Library: install "U8g2" by oliver lewat Library Manager.
 * Sketch ini bergantian mencoba 3 jenis chip OLED (SSD1306, SH1106, SSD1309).
 * Perhatikan NAMA CHIP yang muncul di layar, lalu laporkan.
 */
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// Ganti sesuai baris ">> OLED DITEMUKAN ..." dari sketch tes_oled sebelumnya
#define TEST_SDA 21
#define TEST_SCL 22
#define TEST_ADDR_7BIT 0x3C   // ganti 0x3D jika hasil scan menunjukkan 0x3D

U8G2_SSD1306_128X64_NONAME_F_HW_I2C oledSsd1306(U8G2_R0, U8X8_PIN_NONE, TEST_SCL, TEST_SDA);
U8G2_SH1106_128X64_NONAME_F_HW_I2C  oledSh1106(U8G2_R0, U8X8_PIN_NONE, TEST_SCL, TEST_SDA);
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C oledSsd1309(U8G2_R0, U8X8_PIN_NONE, TEST_SCL, TEST_SDA);

static void tryDriver(U8G2& d, const char* name) {
    Serial.printf(">> Mencoba driver %s ... lihat layar 4 detik\n", name);
    d.setI2CAddress(TEST_ADDR_7BIT << 1);
    d.begin();
    d.setContrast(255);
    d.clearBuffer();
    d.drawFrame(0, 0, 128, 64);
    d.setFont(u8g2_font_ncenB14_tr);
    d.drawStr(8, 30, name);
    d.setFont(u8g2_font_6x10_tr);
    d.drawStr(8, 50, "OLED TEST OK");
    d.sendBuffer();
    delay(4000);
    d.clearBuffer();
    d.sendBuffer();
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("\n===== TES OLED U8g2 (SSD1306 / SH1106 / SSD1309) ====="));
    Serial.printf("Pin SDA=%d SCL=%d, alamat 0x%02X\n", TEST_SDA, TEST_SCL, TEST_ADDR_7BIT);
}

void loop() {
    tryDriver(oledSsd1306, "SSD1306");
    tryDriver(oledSh1106, "SH1106");
    tryDriver(oledSsd1309, "SSD1309");
}

