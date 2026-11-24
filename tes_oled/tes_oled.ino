/**
 * SKETCH TES OLED VERSI PERBAIKAN CHARGE PUMP - Bluino ESP32 Starter Shield
 * Mengirim perintah 0x8D 0x14 (Charge Pump ON) dalam 1 paket I2C tanpa terputus.
 */
#include <Arduino.h>
#include <Wire.h>

#define OLED_ADDR 0x3C

// Kirim daftar perintah dalam SATU paket I2C tanpa STOP di tengah-tengah
static void sendCmdList(const uint8_t* cmds, size_t len) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x00); // Co = 0, D/C = 0 (stream of commands)
    for (size_t i = 0; i < len; ++i) {
        Wire.write(cmds[i]);
    }
    Wire.endTransmission();
}

static void sendSingleCmd(uint8_t c) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x80); // Co = 1, D/C = 0 (single command)
    Wire.write(c);
    Wire.endTransmission();
}

static void initOled() {
    delay(100);
    // Inisialisasi lengkap dengan Charge Pump (0x8D 0x14 untuk SSD1306 & 0xAD 0x8B untuk SH1106)
    // PENTING: Harus dikirim dalam 1 burst agar parameter 0x14 tidak dianggap sebagai perintah baru!
    static const uint8_t init_cmds[] = {
        0xAE,             // Display OFF
        0xD5, 0x80,       // Clock divide ratio
        0xA8, 0x3F,       // Multiplex 64 lines
        0xD3, 0x00,       // No display offset
        0x40,             // Start line 0
        0x8D, 0x14,       // << CHARGE PUMP ON (SSD1306) >>
        0xAD, 0x8B,       // << DC-DC ON (SH1106) >>
        0x20, 0x02,       // Page addressing mode
        0xA1,             // Segment re-map (col 127)
        0xC8,             // COM scan dec
        0xDA, 0x12,       // COM pins config (128x64)
        0x81, 0xFF,       // Contrast MAX (255)
        0xD9, 0xF1,       // Pre-charge
        0xDB, 0x40,       // VCOMH
        0xA4,             // Output follows RAM
        0xA6,             // Normal display
        0xAF              // Display ON!
    };
    sendCmdList(init_cmds, sizeof(init_cmds));
    delay(50);
}

static void fillPattern(uint8_t pattern, uint8_t col_offset) {
    for (uint8_t page = 0; page < 8; ++page) {
        // Set page and column address
        uint8_t page_cmd[] = {
            (uint8_t)(0xB0 + page),
            (uint8_t)(0x00 + (col_offset & 0x0F)),
            (uint8_t)(0x10 + (col_offset >> 4))
        };
        sendCmdList(page_cmd, sizeof(page_cmd));

        // Tulis data 128 piksel dalam chunk 16 byte
        for (uint8_t chunk = 0; chunk < 8; ++chunk) {
            Wire.beginTransmission(OLED_ADDR);
            Wire.write(0x40); // Data stream
            for (uint8_t i = 0; i < 16; ++i) {
                Wire.write(pattern);
            }
            Wire.endTransmission();
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("\n===== TES NYALA OLED (CHARGE PUMP FIX) ====="));

    Wire.begin(21, 22);
    Wire.setClock(100000);

    Wire.beginTransmission(OLED_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.printf("Device 0x%02X tidak merespon di I2C (SDA=21, SCL=22)!\n", OLED_ADDR);
        return;
    }
    Serial.printf("Device 0x%02X terdeteksi! Mengaktifkan Charge Pump & Display ON...\n", OLED_ADDR);

    initOled();

    // Tes Hardware: Perintah 0xA5 memaksa SEMUA piksel menyala langsung dari hardware chip
    Serial.println(F(">> Uji 1: Hardware Force All Pixels ON (0xA5) - Layar harus PUTIH!"));
    sendSingleCmd(0xA5);
    delay(2000);

    // Kembalikan ke mode RAM (0xA4) lalu gambar pola
    sendSingleCmd(0xA4);
    Serial.println(F(">> Uji 2: Gambar pola kotak putih penuh dari RAM..."));
    fillPattern(0xFF, 0);
    delay(2000);

    Serial.println(F(">> Uji 3: Pola garis-garis..."));
    fillPattern(0xAA, 0);
    delay(2000);

    Serial.println(F(">> Uji 4: Bersihkan layar (Hitam)..."));
    fillPattern(0x00, 0);

    Serial.println(F(">> Selesai! Jika layar menyala pada Uji 1/2/3, OLED berfungsi normal."));
}

void loop() {}
