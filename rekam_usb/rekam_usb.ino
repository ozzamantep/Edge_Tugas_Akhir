/**
 * ============================================================================
 * PEREKAM DATASET VIA KABEL USB SERIAL KE LAPTOP (TANPA MICROSD, 50 Hz)
 * Board  : ESP32 DevKit V1 pada Bluino ESP32 IoT Starter Shield
 * Sensor : MPU6050 (I2C 0x68, SDA=21, SCL=22, Range +/-4g, Filter 21 Hz)
 * Output : OLED 128x64 (opsional), Active Buzzer (GPIO 12), USB Serial 115200
 *
 * Cara pakai:
 *   1. Upload sketch ini ke ESP32 (tidak butuh library eksternal & tanpa SD card).
 *   2. Tutup Serial Monitor di Arduino IDE.
 *   3. Jalankan script Python di laptop:  python rekam_usb.py
 * ============================================================================
 */

#include <Arduino.h>
#include <Wire.h>

#define PIN_I2C_SDA         21
#define PIN_I2C_SCL         22
#define MPU6050_ADDR        0x68
#define PIN_BUZZER          12

#define SAMPLE_INTERVAL_US  20000UL   // 50 Hz (tiap 20 ms)
#define MPU_LSB_PER_G_4G    8192.0f   // Range +/-4g

#define PREP_SECONDS        5
#define DIAM_SAMPLES        3000      // 60 detik @ 50 Hz
#define JALAN_SAMPLES       3000      // 60 detik @ 50 Hz
#define LARI_SAMPLES        1000      // 20 detik @ 50 Hz

// ============================================================================
// DRIVER MINI OLED 128x64 (SSD1306 & SH1106)
// ============================================================================
static const uint8_t FONT5X7[][5] PROGMEM = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00}, {0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7F,0x14,0x7F,0x14}, {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00}, {0x00,0x1C,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1C,0x00}, {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02}, {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31}, {0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39}, {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}, {0x00,0x36,0x36,0x00,0x00},
    {0x00,0x56,0x36,0x00,0x00}, {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06}, {0x32,0x49,0x79,0x41,0x3E},
    {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
};

static uint8_t oled_addr = 0;
static uint8_t oled_buf[128 * 8];

static void oledSendCmds(const uint8_t* cmds, size_t len) {
    if (oled_addr == 0) return;
    Wire.beginTransmission(oled_addr);
    Wire.write(0x00);
    for (size_t i = 0; i < len; ++i) Wire.write(cmds[i]);
    Wire.endTransmission();
}

static bool oledBegin() {
    delay(50);
    uint8_t candidates[2] = {0x3C, 0x3D};
    for (uint8_t a : candidates) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            oled_addr = a;
            break;
        }
    }
    if (oled_addr == 0) return false;
    static const uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0xAD, 0x8B, 0x20, 0x02, 0xA1, 0xC8,
        0xDA, 0x12, 0x81, 0xFF, 0xD9, 0xF1, 0xDB, 0x40,
        0xA4, 0xA6, 0xAF
    };
    oledSendCmds(init_cmds, sizeof(init_cmds));
    delay(30);
    return true;
}

static void oledDrawStr(int x, int y, const char* str, int scale) {
    while (*str) {
        char c = *str++;
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        if (c < 32 || c > 90) c = '?';
        const uint8_t* g = FONT5X7[c - 32];
        for (int col = 0; col < 5; ++col) {
            uint8_t bits = pgm_read_byte(&g[col]);
            for (int row = 0; row < 7; ++row) {
                if (bits & (1 << row)) {
                    for (int dx = 0; dx < scale; ++dx) {
                        for (int dy = 0; dy < scale; ++dy) {
                            int px = x + col * scale + dx;
                            int py = y + row * scale + dy;
                            if (px >= 0 && px < 128 && py >= 0 && py < 64) {
                                oled_buf[px + (py / 8) * 128] |= (1 << (py & 7));
                            }
                        }
                    }
                }
            }
        }
        x += 6 * scale;
    }
}

static void oledShow(const char* title, const char* bigLine, const char* subLine) {
    if (oled_addr == 0) return;
    memset(oled_buf, 0, sizeof(oled_buf));
    oledDrawStr(0, 0, title, 1);
    for (int i = 0; i < 128; ++i) oled_buf[i + 128] |= 0x04;
    oledDrawStr(0, 20, bigLine, 2);
    oledDrawStr(0, 52, subLine, 1);
    for (uint8_t page = 0; page < 8; ++page) {
        uint8_t cmd_p[] = {(uint8_t)(0xB0 + page), 0x00, 0x10};
        oledSendCmds(cmd_p, sizeof(cmd_p));
        for (uint8_t chunk = 0; chunk < 8; ++chunk) {
            Wire.beginTransmission(oled_addr);
            Wire.write(0x40);
            Wire.write(&oled_buf[page * 128 + chunk * 16], 16);
            Wire.endTransmission();
        }
    }
}

// ============================================================================
// DRIVER MPU6050 & BUZZER
// ============================================================================
static bool mpuBegin() {
    Wire.beginTransmission(MPU6050_ADDR);
    if (Wire.endTransmission() != 0) return false;
    uint8_t wake[]  = {0x6B, 0x00}; Wire.beginTransmission(MPU6050_ADDR); Wire.write(wake, 2);  Wire.endTransmission(); delay(10);
    uint8_t dlpf[]  = {0x1A, 0x04}; Wire.beginTransmission(MPU6050_ADDR); Wire.write(dlpf, 2);  Wire.endTransmission();
    uint8_t range[] = {0x1C, 0x08}; Wire.beginTransmission(MPU6050_ADDR); Wire.write(range, 2); Wire.endTransmission();
    return true;
}

static bool mpuReadG(float& ax, float& ay, float& az) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)MPU6050_ADDR, (uint8_t)6, (uint8_t)true) != 6) return false;
    int16_t rx = (int16_t)((Wire.read() << 8) | Wire.read());
    int16_t ry = (int16_t)((Wire.read() << 8) | Wire.read());
    int16_t rz = (int16_t)((Wire.read() << 8) | Wire.read());
    ax = (float)rx / MPU_LSB_PER_G_4G;
    ay = (float)ry / MPU_LSB_PER_G_4G;
    az = (float)rz / MPU_LSB_PER_G_4G;
    return true;
}

static void beep(int count, int on_ms, int off_ms = 80) {
    for (int i = 0; i < count; ++i) {
        digitalWrite(PIN_BUZZER, HIGH);
        delay(on_ms);
        digitalWrite(PIN_BUZZER, LOW);
        if (i + 1 < count) delay(off_ms);
    }
}

// ============================================================================
// LOGIKA STREAMING USB SERIAL 50 Hz
// ============================================================================
struct SessionTask {
    int sesi;
    const char* label;
    uint32_t target_samples;
};

static const SessionTask AUTO_TASKS[3] = {
    {1, "diam",  DIAM_SAMPLES},
    {2, "jalan", JALAN_SAMPLES},
    {3, "lari",  LARI_SAMPLES}
};

static bool recording = false;
static bool auto_mode = false;
static int auto_step = 0;
static int current_sesi = 1;
static const char* current_label = "diam";
static uint32_t target_samples = 0; // 0 = rekam terus sampai ketik 's'
static uint32_t sample_count = 0;
static uint32_t last_sample_us = 0;

static void startSession(int sesi, const char* label, uint32_t target) {
    current_sesi = sesi;
    current_label = label;
    target_samples = target;
    sample_count = 0;

    char big[20], sub[28];
    snprintf(big, sizeof(big), "USB %s", label);
    snprintf(sub, sizeof(sub), "SESI %d (50 HZ)", sesi);
    oledShow("REKAM KE LAPTOP", big, sub);

    Serial.printf("STATUS,START,%d,%s,%lu\n", sesi, label, (unsigned long)target);
    beep(1, 150);
    recording = true;
    last_sample_us = micros();
}

static void startAutoCountdown(int step_idx) {
    auto_mode = true;
    auto_step = step_idx;
    const SessionTask& t = AUTO_TASKS[step_idx];
    char big[20];
    snprintf(big, sizeof(big), "SIAP %s", t.label);
    for (int s = PREP_SECONDS; s > 0; --s) {
        char sub[28];
        snprintf(sub, sizeof(sub), "MULAI DLM %d DETIK", s);
        oledShow("PERSIAPAN", big, sub);
        Serial.printf("STATUS,PREP,%s,%d\n", t.label, s);
        beep(1, 40);
        delay(960);
    }
    startSession(t.sesi, t.label, t.target_samples);
}

static void handleCommand(char c) {
    c = (char)tolower(c);
    if (c == 's') {
        recording = false;
        auto_mode = false;
        oledShow("USB LOGGER", "STANDBY", "PILIH D / J / L / A");
        Serial.println(F("STATUS,STOPPED"));
    } else if (c == 'd') {
        auto_mode = false;
        startSession(1, "diam", 0);
    } else if (c == 'j') {
        auto_mode = false;
        startSession(2, "jalan", 0);
    } else if (c == 'l') {
        auto_mode = false;
        startSession(3, "lari", 0);
    } else if (c == 'a') {
        startAutoCountdown(0);
    }
}

void setup() {
    Serial.begin(115200);
    delay(300);

    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(100000UL);

    if (oledBegin()) {
        oledShow("USB LOGGER", "STANDBY", "JALANKAN REKAM_USB");
    }

    if (!mpuBegin()) {
        Serial.println(F("STATUS,ERROR,MPU6050_NOT_FOUND"));
        oledShow("ERROR SENSOR", "MPU6050 0X68", "CEK WIRING");
        while (true) { beep(1, 100); delay(1000); }
    }

    Serial.println(F("STATUS,READY"));
}

void loop() {
    while (Serial.available()) {
        char ch = (char)Serial.read();
        if (ch != '\n' && ch != '\r' && ch != ' ') {
            handleCommand(ch);
        }
    }

    if (!recording) {
        delay(10);
        return;
    }

    const uint32_t now_us = micros();
    if ((uint32_t)(now_us - last_sample_us) < SAMPLE_INTERVAL_US) {
        return;
    }
    if ((uint32_t)(now_us - last_sample_us) > (SAMPLE_INTERVAL_US * 3)) {
        last_sample_us = now_us;
    } else {
        last_sample_us += SAMPLE_INTERVAL_US;
    }

    float ax = 0.0f, ay = 0.0f, az = 1.0f;
    if (!mpuReadG(ax, ay, az)) return;

    const uint32_t ts_ms = sample_count * 20UL;
    Serial.printf("DATA,%lu,%d,%.3f,%.3f,%.3f,%s\n",
                  (unsigned long)ts_ms, current_sesi, ax, ay, az, current_label);
    sample_count++;

    if (auto_mode && target_samples > 0 && sample_count >= target_samples) {
        recording = false;
        Serial.printf("STATUS,SESSION_DONE,%d,%s,%lu\n",
                      current_sesi, current_label, (unsigned long)sample_count);
        if (auto_step + 1 < 3) {
            beep(2, 120, 80);
            startAutoCountdown(auto_step + 1);
        } else {
            auto_mode = false;
            beep(3, 250, 120);
            oledShow("USB LOGGER", "SELESAI!", "CEK FILE CSV LAPTOP");
            Serial.println(F("STATUS,ALL_DONE"));
        }
    }
}
