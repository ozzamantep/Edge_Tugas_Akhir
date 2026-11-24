#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "model.h"

// Font 5x7 untuk karakter ASCII 32 (' ') s.d. 90 ('Z')
static const uint8_t OLED_FONT5X7[][5] PROGMEM = {
    {0x00,0x00,0x00,0x00,0x00}, // ' ' (32)
    {0x00,0x00,0x5F,0x00,0x00}, // '!' (33)
    {0x00,0x07,0x00,0x07,0x00}, // '"' (34)
    {0x14,0x7F,0x14,0x7F,0x14}, // '#' (35)
    {0x24,0x2A,0x7F,0x2A,0x12}, // '$' (36)
    {0x23,0x13,0x08,0x64,0x62}, // '%' (37)
    {0x36,0x49,0x55,0x22,0x50}, // '&' (38)
    {0x00,0x05,0x03,0x00,0x00}, // ''' (39)
    {0x00,0x1C,0x22,0x41,0x00}, // '(' (40)
    {0x00,0x41,0x22,0x1C,0x00}, // ')' (41)
    {0x14,0x08,0x3E,0x08,0x14}, // '*' (42)
    {0x08,0x08,0x3E,0x08,0x08}, // '+' (43)
    {0x00,0x50,0x30,0x00,0x00}, // ',' (44)
    {0x08,0x08,0x08,0x08,0x08}, // '-' (45)
    {0x00,0x60,0x60,0x00,0x00}, // '.' (46)
    {0x20,0x10,0x08,0x04,0x02}, // '/' (47)
    {0x3E,0x51,0x49,0x45,0x3E}, // '0' (48)
    {0x00,0x42,0x7F,0x40,0x00}, // '1' (49)
    {0x42,0x61,0x51,0x49,0x46}, // '2' (50)
    {0x21,0x41,0x45,0x4B,0x31}, // '3' (51)
    {0x18,0x14,0x12,0x7F,0x10}, // '4' (52)
    {0x27,0x45,0x45,0x45,0x39}, // '5' (53)
    {0x3C,0x4A,0x49,0x49,0x30}, // '6' (54)
    {0x01,0x71,0x09,0x05,0x03}, // '7' (55)
    {0x36,0x49,0x49,0x49,0x36}, // '8' (56)
    {0x06,0x49,0x49,0x29,0x1E}, // '9' (57)
    {0x00,0x36,0x36,0x00,0x00}, // ':' (58)
    {0x00,0x56,0x36,0x00,0x00}, // ';' (59)
    {0x08,0x14,0x22,0x41,0x00}, // '<' (60)
    {0x14,0x14,0x14,0x14,0x14}, // '=' (61)
    {0x00,0x41,0x22,0x14,0x08}, // '>' (62)
    {0x02,0x01,0x51,0x09,0x06}, // '?' (63)
    {0x32,0x49,0x79,0x41,0x3E}, // '@' (64)
    {0x7E,0x11,0x11,0x11,0x7E}, // 'A' (65)
    {0x7F,0x49,0x49,0x49,0x36}, // 'B' (66)
    {0x3E,0x41,0x41,0x41,0x22}, // 'C' (67)
    {0x7F,0x41,0x41,0x22,0x1C}, // 'D' (68)
    {0x7F,0x49,0x49,0x49,0x41}, // 'E' (69)
    {0x7F,0x09,0x09,0x09,0x01}, // 'F' (70)
    {0x3E,0x41,0x49,0x49,0x7A}, // 'G' (71)
    {0x7F,0x08,0x08,0x08,0x7F}, // 'H' (72)
    {0x00,0x41,0x7F,0x41,0x00}, // 'I' (73)
    {0x20,0x40,0x41,0x3F,0x01}, // 'J' (74)
    {0x7F,0x08,0x14,0x22,0x41}, // 'K' (75)
    {0x7F,0x40,0x40,0x40,0x40}, // 'L' (76)
    {0x7F,0x02,0x0C,0x02,0x7F}, // 'M' (77)
    {0x7F,0x04,0x08,0x10,0x7F}, // 'N' (78)
    {0x3E,0x41,0x41,0x41,0x3E}, // 'O' (79)
    {0x7F,0x09,0x09,0x09,0x06}, // 'P' (80)
    {0x3E,0x41,0x51,0x21,0x5E}, // 'Q' (81)
    {0x7F,0x09,0x19,0x29,0x46}, // 'R' (82)
    {0x46,0x49,0x49,0x49,0x31}, // 'S' (83)
    {0x01,0x01,0x7F,0x01,0x01}, // 'T' (84)
    {0x3F,0x40,0x40,0x40,0x3F}, // 'U' (85)
    {0x1F,0x20,0x40,0x20,0x1F}, // 'V' (86)
    {0x3F,0x40,0x38,0x40,0x3F}, // 'W' (87)
    {0x63,0x14,0x08,0x14,0x63}, // 'X' (88)
    {0x07,0x08,0x70,0x08,0x07}, // 'Y' (89)
    {0x61,0x51,0x49,0x45,0x43}  // 'Z' (90)
};

class OledDisplay {
public:
    OledDisplay() : addr_(OLED_I2C_ADDR), initialized_(false) {
        memset(buf_, 0, sizeof(buf_));
    }

    bool begin() {
        delay(100);

        if (probeAddress(0x3C)) {
            addr_ = 0x3C;
        } else if (probeAddress(0x3D)) {
            addr_ = 0x3D;
        } else {
            initialized_ = false;
            return false;
        }

        // Urutan inisialisasi dikirim dalam SATU paket I2C burst agar Charge Pump (0x8D 0x14)
        // aktif dengan benar dan tidak terpotong oleh I2C STOP condition!
        static const uint8_t init_cmds[] = {
            0xAE,             // Display OFF
            0xD5, 0x80,       // Set Oscillator Frequency / Clock Divide
            0xA8, 0x3F,       // Multiplex Ratio (64 lines)
            0xD3, 0x00,       // Display Offset = 0
            0x40,             // Start Line = 0
            0x8D, 0x14,       // << CHARGE PUMP ON (SSD1306) >>
            0xAD, 0x8B,       // << DC-DC ON (SH1106) >>
            0x20, 0x02,       // Page Addressing Mode
            0xA1,             // Segment re-map (col 127)
            0xC8,             // COM Output Scan Direction remapped
            0xDA, 0x12,       // COM Pins configuration (128x64)
            0x81, 0xFF,       // Contrast MAX (255)
            0xD9, 0xF1,       // Pre-charge period
            0xDB, 0x40,       // VCOMH level
            0xA4,             // Output follows RAM
            0xA6,             // Normal Display (not inverted)
            0xAF              // Display ON!
        };

        sendCmdList(init_cmds, sizeof(init_cmds));
        delay(50);

        initialized_ = true;
        clearBuffer();
        flush();
        showStatusMessage("BLUINO ESP32", "EDGE ML SIAP");
        return true;
    }

    uint8_t getAddress() const {
        return addr_;
    }

    bool isInitialized() const {
        return initialized_;
    }

    void showStatusMessage(const char* line1, const char* line2 = "") {
        if (!initialized_ && !begin()) return;
        clearBuffer();
        drawString(0, 0, "BLUINO ESP32 EDGE ML", 1);
        drawHLine(0, 10, OLED_WIDTH);
        drawString(0, 18, line1, 1);
        if (line2 && line2[0] != '\0') {
            drawString(0, 32, line2, 1);
        }
        flush();
    }

    void renderInference(
        int activity_id,
        float confidence,
        bool wifi_connected,
        bool sd_ready,
        uint32_t total_steps
    ) {
        if (!initialized_ && !begin()) return;
        clearBuffer();

        char top_line[32];
        snprintf(
            top_line, sizeof(top_line),
            "WIFI:%s SD:%s P:%s",
            wifi_connected ? "OK" : "--",
            sd_ready ? "OK" : "NO",
            (DEVICE_MOUNT_POSITION[0] == 'p' && DEVICE_MOUNT_POSITION[1] == 'i') ? "PGG" : "PRG"
        );
        drawString(0, 0, top_line, 1);
        drawHLine(0, 10, OLED_WIDTH);

        const char* label_txt = (activity_id >= 0 && activity_id < MODEL_NUM_CLASSES)
                                ? ACTIVITY_LABELS_UPPER[activity_id]
                                : "WAIT";
        int text_w = (int)strlen(label_txt) * 6 * 3;
        int cursor_x = (OLED_WIDTH - text_w) / 2;
        if (cursor_x < 0) cursor_x = 0;
        drawString(cursor_x, 18, label_txt, 3);

        drawHLine(0, 48, OLED_WIDTH);
        char bot_line[32];
        snprintf(
            bot_line, sizeof(bot_line),
            "CONF:%d%% STEP:%lu",
            (int)(confidence * 100.0f + 0.5f),
            (unsigned long)total_steps
        );
        drawString(0, 54, bot_line, 1);
        flush();
    }

private:
    uint8_t buf_[OLED_WIDTH * (OLED_HEIGHT / 8)];
    uint8_t addr_;
    bool initialized_;

    static bool probeAddress(uint8_t address) {
        Wire.beginTransmission(address);
        return (Wire.endTransmission() == 0);
    }

    void sendCmdList(const uint8_t* cmds, size_t len) {
        Wire.beginTransmission(addr_);
        Wire.write(0x00); // Co = 0, D/C = 0 (stream of commands)
        for (size_t i = 0; i < len; ++i) {
            Wire.write(cmds[i]);
        }
        Wire.endTransmission();
    }

    void clearBuffer() {
        memset(buf_, 0, sizeof(buf_));
    }

    void setPixel(int x, int y) {
        if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
        buf_[x + (y / 8) * OLED_WIDTH] |= (1 << (y & 7));
    }

    void drawHLine(int x, int y, int w) {
        for (int i = 0; i < w; ++i) setPixel(x + i, y);
    }

    void drawChar(int x, int y, char c, int scale) {
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
        if (c < 32 || c > 90) c = '?';
        const uint8_t* glyph = OLED_FONT5X7[c - 32];
        for (int col = 0; col < 5; ++col) {
            uint8_t bits = pgm_read_byte(&glyph[col]);
            for (int row = 0; row < 7; ++row) {
                if (bits & (1 << row)) {
                    for (int dx = 0; dx < scale; ++dx) {
                        for (int dy = 0; dy < scale; ++dy) {
                            setPixel(x + col * scale + dx, y + row * scale + dy);
                        }
                    }
                }
            }
        }
    }

    void drawString(int x, int y, const char* str, int scale) {
        while (*str) {
            drawChar(x, y, *str++, scale);
            x += 6 * scale;
        }
    }

    void flush() {
        for (uint8_t page = 0; page < 8; ++page) {
            uint8_t page_cmd[] = {
                (uint8_t)(0xB0 + page),
                0x00,
                0x10
            };
            sendCmdList(page_cmd, sizeof(page_cmd));

            for (uint8_t chunk = 0; chunk < 8; ++chunk) {
                Wire.beginTransmission(addr_);
                Wire.write(0x40); // Data stream
                for (uint8_t i = 0; i < 16; ++i) {
                    Wire.write(buf_[page * OLED_WIDTH + chunk * 16 + i]);
                }
                Wire.endTransmission();
            }
        }
    }
};

#endif // OLED_H
