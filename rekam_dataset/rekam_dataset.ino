/**
 * ============================================================================
 * SKETCH PENGAMBILAN DATASET AKSELEROMETER KE MICROSD (50 Hz)
 * Board  : ESP32 DevKit V1 pada Bluino ESP32 IoT Starter Shield
 * Sensor : MPU6050 (I2C 0x68, SDA=21, SCL=22, Range +/-4g)
 * Output : MicroSD (SPI VSPI: CS=5, SCK=18, MISO=19, MOSI=23)
 *          OLED 128x64 (I2C 0x3C/0x3D) + Active Buzzer (GPIO 12)
 *
 * Format file CSV di MicroSD (/dataset_baru.csv) IDENTIK dengan
 * dataset_aktivitas_esp32.csv:
 *   timestamp_ms,subjek,sesi,posisi,ax,ay,az,label
 *
 * Tanpa perlu install library eksternal (langsung pakai Wire.h, SPI.h, SD.h).
 * ============================================================================
 */

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

// ============================================================================
// 1. PENGATURAN DATA YANG AKAN DIREKAM (UBAH SESUAI KEBUTUHAN)
// ============================================================================
#define SUBJEK_ID "S07"             // Ganti kode subjek: S07, S08, dst.
#define POSISI_SENSOR "pinggang"    // "pinggang" atau "pergelangan"
#define NAMA_FILE_CSV "/dataset_baru.csv"

// Mode Perekaman:
//   1 = OTOMATIS BERURUTAN (cocok pakai powerbank tanpa laptop):
//       Countdown 5 dtk -> DIAM (60 dtk) -> Jeda 5 dtk -> JALAN (60 dtk) ->
//       Jeda 5 dtk -> LARI (20 dtk) -> SELESAI
//   0 = MANUAL (langsung rekam 1 aktivitas di bawah, atau dikontrol lewat
//   Serial Monitor: d/j/l/s)
#define AUTO_SEQUENCE_MODE 1

// Dipakai jika AUTO_SEQUENCE_MODE == 0:
#define MANUAL_SESI 1
#define MANUAL_LABEL "diam" // "diam", "jalan", atau "lari"
#define MANUAL_MAX_SAMPLES                                                     \
  3000 // 3000 sampel = 60 detik @ 50 Hz (0 = rekam terus sampai ketik 's')

// Durasi Mode Otomatis (samakan dengan dataset_aktivitas_esp32.csv: 60s diam,
// 60s jalan, 20s lari)
#define PREP_SECONDS 5     // Waktu persiapan sebelum tiap sesi (detik)
#define DIAM_SAMPLES 3000  // 60 detik @ 50 Hz
#define JALAN_SAMPLES 3000 // 60 detik @ 50 Hz
#define LARI_SAMPLES 1000  // 20 detik @ 50 Hz

// ============================================================================
// 2. PIN MAP BLUINO ESP32 IOT STARTER SHIELD
// ============================================================================
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22
#define MPU6050_ADDR 0x68
#define PIN_SD_CS 5
#define PIN_SD_SCK 18
#define PIN_SD_MISO 19
#define PIN_SD_MOSI 23
#define PIN_BUZZER 12

#define SAMPLE_INTERVAL_US 20000UL // 50 Hz (tiap 20.000 mikrodetik = 20 ms)
#define MPU_LSB_PER_G_4G 8192.0f   // Range +/-4g

// ============================================================================
// 3. DRIVER MINI OLED 128x64 (SSD1306 & SH1106)
// ============================================================================
static const uint8_t FONT5X7[][5] PROGMEM = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00},
    {0x00, 0x07, 0x00, 0x07, 0x00}, {0x14, 0x7F, 0x14, 0x7F, 0x14},
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00},
    {0x00, 0x1C, 0x22, 0x41, 0x00}, {0x00, 0x41, 0x22, 0x1C, 0x00},
    {0x14, 0x08, 0x3E, 0x08, 0x14}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08},
    {0x00, 0x60, 0x60, 0x00, 0x00}, {0x20, 0x10, 0x08, 0x04, 0x02},
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E},
    {0x00, 0x36, 0x36, 0x00, 0x00}, {0x00, 0x56, 0x36, 0x00, 0x00},
    {0x08, 0x14, 0x22, 0x41, 0x00}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x51, 0x09, 0x06},
    {0x32, 0x49, 0x79, 0x41, 0x3E}, {0x7E, 0x11, 0x11, 0x11, 0x7E},
    {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41},
    {0x7F, 0x09, 0x09, 0x09, 0x01}, {0x3E, 0x41, 0x49, 0x49, 0x7A},
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41},
    {0x7F, 0x40, 0x40, 0x40, 0x40}, {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E},
    {0x7F, 0x09, 0x19, 0x29, 0x46}, {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F},
    {0x63, 0x14, 0x08, 0x14, 0x63}, {0x07, 0x08, 0x70, 0x08, 0x07},
    {0x61, 0x51, 0x49, 0x45, 0x43}};

static uint8_t oled_addr = 0;
static uint8_t oled_buf[128 * 8];

static void oledSendCmds(const uint8_t *cmds, size_t len) {
  if (oled_addr == 0)
    return;
  Wire.beginTransmission(oled_addr);
  Wire.write(0x00);
  for (size_t i = 0; i < len; ++i)
    Wire.write(cmds[i]);
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
  if (oled_addr == 0)
    return false;

  static const uint8_t init_cmds[] = {0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
                                      0x40, 0x8D, 0x14, 0xAD, 0x8B, 0x20, 0x02,
                                      0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xFF, 0xD9,
                                      0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF};
  oledSendCmds(init_cmds, sizeof(init_cmds));
  delay(30);
  return true;
}

static void oledDrawStr(int x, int y, const char *str, int scale) {
  while (*str) {
    char c = *str++;
    if (c >= 'a' && c <= 'z')
      c = c - 'a' + 'A';
    if (c < 32 || c > 90)
      c = '?';
    const uint8_t *g = FONT5X7[c - 32];
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

static void oledShow(const char *title, const char *bigLine,
                     const char *subLine) {
  if (oled_addr == 0)
    return;
  memset(oled_buf, 0, sizeof(oled_buf));
  oledDrawStr(0, 0, title, 1);
  for (int i = 0; i < 128; ++i)
    oled_buf[i + 128] |= 0x04;
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
// 4. DRIVER MPU6050 & BUZZER
// ============================================================================
static bool mpuBegin() {
  Wire.beginTransmission(MPU6050_ADDR);
  if (Wire.endTransmission() != 0)
    return false;

  uint8_t wake[] = {0x6B, 0x00};
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(wake, 2);
  Wire.endTransmission();
  delay(10);

  uint8_t dlpf[] = {0x1A, 0x04}; // Filter 21 Hz
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(dlpf, 2);
  Wire.endTransmission();

  uint8_t range[] = {0x1C, 0x08}; // +/-4g
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(range, 2);
  Wire.endTransmission();
  return true;
}

static bool mpuReadG(float &ax, float &ay, float &az) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0)
    return false;
  if (Wire.requestFrom((uint8_t)MPU6050_ADDR, (uint8_t)6, (uint8_t)true) != 6)
    return false;

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
    if (i + 1 < count)
      delay(off_ms);
  }
}

// ============================================================================
// 5. LOGIKA PEREKAMAN DATASET KE MICROSD
// ============================================================================
enum RecState { STATE_IDLE, STATE_PREP, STATE_RECORDING, STATE_DONE };

struct SessionTask {
  int sesi;
  const char *label;
  uint32_t target_samples;
};

static const SessionTask AUTO_TASKS[3] = {{1, "diam", DIAM_SAMPLES},
                                          {2, "jalan", JALAN_SAMPLES},
                                          {3, "lari", LARI_SAMPLES}};

static File csvFile;
static RecState state = STATE_IDLE;
static int auto_step = 0;
static int current_sesi = MANUAL_SESI;
static const char *current_label = MANUAL_LABEL;
static uint32_t current_target = MANUAL_MAX_SAMPLES;
static uint32_t sample_count = 0;
static uint32_t total_saved = 0;
static uint32_t last_sample_us = 0;

static bool openCsvForAppend() {
  bool exists = SD.exists(NAMA_FILE_CSV);
  csvFile = SD.open(NAMA_FILE_CSV, FILE_APPEND);
  if (!csvFile)
    return false;

  if (!exists || csvFile.size() == 0) {
    csvFile.println(F("timestamp_ms,subjek,sesi,posisi,ax,ay,az,label"));
    csvFile.flush();
  }
  return true;
}

static void stopRecording() {
  if (csvFile) {
    csvFile.flush();
    csvFile.close();
  }
  state = STATE_IDLE;
  Serial.printf(
      "[SD] Rekaman berhenti. Total baris tersimpan sejauh ini: %lu\n",
      (unsigned long)total_saved);
}

static void startRecordingSession(int sesi, const char *label,
                                  uint32_t target_samples) {
  current_sesi = sesi;
  current_label = label;
  current_target = target_samples;
  sample_count = 0;

  if (!openCsvForAppend()) {
    Serial.println(F("[ERR] Gagal membuka file CSV di MicroSD!"));
    oledShow("ERROR MICROSD", "GAGAL BUKA", NAMA_FILE_CSV);
    beep(4, 150, 100);
    state = STATE_IDLE;
    return;
  }

  char sub[32];
  snprintf(sub, sizeof(sub), "%s S%d %s", SUBJEK_ID, sesi, POSISI_SENSOR);
  char big[24];
  snprintf(big, sizeof(big), "REC %s", label);
  oledShow("MEREKAM 50HZ", big, sub);

  Serial.printf("[REC] Mulai rekam -> Subjek=%s, Sesi=%d, Posisi=%s, Label=%s, "
                "Target=%lu sampel\n",
                SUBJEK_ID, sesi, POSISI_SENSOR, label,
                (unsigned long)target_samples);

  beep(1, 150);
  state = STATE_RECORDING;
  last_sample_us = micros();
}

static void runCountdownAndStart(int sesi, const char *label,
                                 uint32_t target_samples) {
  state = STATE_PREP;
  char big[24];
  snprintf(big, sizeof(big), "SIAP %s", label);
  for (int s = PREP_SECONDS; s > 0; --s) {
    char sub[32];
    snprintf(sub, sizeof(sub), "MULAI DLM %d DETIK", s);
    oledShow("PERSIAPAN", big, sub);
    Serial.printf("[PREP] Siap '%s' (sesi %d) dalam %d detik...\n", label, sesi,
                  s);
    beep(1, 40);
    delay(960);
  }
  startRecordingSession(sesi, label, target_samples);
}

static void handleSerialCommands() {
  if (!Serial.available())
    return;
  char cmd = (char)tolower(Serial.read());
  if (cmd == '\n' || cmd == '\r' || cmd == ' ')
    return;

  if (cmd == 's') {
    stopRecording();
    oledShow("RECORD PAUSED", "STOPPED", "KETIK D / J / L");
  } else if (cmd == 'd') {
    stopRecording();
    startRecordingSession(1, "diam", MANUAL_MAX_SAMPLES);
  } else if (cmd == 'j') {
    stopRecording();
    startRecordingSession(2, "jalan", MANUAL_MAX_SAMPLES);
  } else if (cmd == 'l') {
    stopRecording();
    startRecordingSession(3, "lari", MANUAL_MAX_SAMPLES);
  } else if (cmd == 'a') {
    stopRecording();
    auto_step = 0;
    runCountdownAndStart(AUTO_TASKS[0].sesi, AUTO_TASKS[0].label,
                         AUTO_TASKS[0].target_samples);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(
      F("\n========================================================"));
  Serial.println(F("  DATA LOGGER DATASET AKTIVITAS ESP32 -> MICROSD (50 Hz)"));
  Serial.println(F("========================================================"));

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(100000UL);

  if (oledBegin()) {
    oledShow("DATA LOGGER", "CEK SENSOR...", SUBJEK_ID);
  }

  if (!mpuBegin()) {
    Serial.println(F("[ERR] MPU6050 tidak terdeteksi di 0x68!"));
    oledShow("ERROR SENSOR", "MPU6050 0X68", "CEK WIRING");
    while (true) {
      beep(1, 100);
      delay(1000);
    }
  }
  Serial.println(F("[OK] MPU6050 siap (+/-4g, 21Hz)."));

  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  if (!SD.begin(PIN_SD_CS, SPI)) {
    Serial.println(F("[ERR] Kartu MicroSD tidak terdeteksi! Pastikan terpasang "
                     "& format FAT32."));
    oledShow("ERROR MICROSD", "TIDAK ADA SD", "CEK SLOT SD");
    while (true) {
      beep(2, 100, 100);
      delay(1500);
    }
  }
  Serial.printf("[OK] MicroSD siap. File target: %s\n", NAMA_FILE_CSV);
  Serial.println(
      F("Perintah Serial: 'd'=diam, 'j'=jalan, 'l'=lari, 's'=stop, 'a'=auto"));

#if AUTO_SEQUENCE_MODE
  auto_step = 0;
  runCountdownAndStart(AUTO_TASKS[0].sesi, AUTO_TASKS[0].label,
                       AUTO_TASKS[0].target_samples);
#else
  startRecordingSession(MANUAL_SESI, MANUAL_LABEL, MANUAL_MAX_SAMPLES);
#endif
}

void loop() {
  handleSerialCommands();

  if (state != STATE_RECORDING) {
    delay(20);
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
  if (!mpuReadG(ax, ay, az)) {
    return;
  }

  // Format persis dataset_aktivitas_esp32.csv:
  // timestamp_ms,subjek,sesi,posisi,ax,ay,az,label
  const uint32_t ts_ms = sample_count * 20UL;
  char line[96];
  snprintf(line, sizeof(line), "%lu,%s,%d,%s,%.3f,%.3f,%.3f,%s",
           (unsigned long)ts_ms, SUBJEK_ID, current_sesi, POSISI_SENSOR, ax, ay,
           az, current_label);

  csvFile.println(line);
  sample_count++;
  total_saved++;

  // Flush tiap 25 sampel (0.5 detik) agar aman di SD tanpa mem-block sampling
  // 50 Hz
  if ((sample_count % 25) == 0) {
    csvFile.flush();
  }
  // Cetak progres ke Serial tiap 1 detik (50 sampel)
  if ((sample_count % 50) == 0) {
    Serial.printf("[REC %s] %lu sampel | ax=%.3f ay=%.3f az=%.3f\n",
                  current_label, (unsigned long)sample_count, ax, ay, az);
  }

  // Cek apakah target jumlah sampel untuk sesi ini sudah tercapai
  if (current_target > 0 && sample_count >= current_target) {
    csvFile.flush();
    csvFile.close();
    Serial.printf("[DONE] Sesi %d (%s) selesai: %lu sampel tersimpan.\n",
                  current_sesi, current_label, (unsigned long)sample_count);

#if AUTO_SEQUENCE_MODE
    auto_step++;
    if (auto_step < 3) {
      beep(2, 120, 80);
      runCountdownAndStart(AUTO_TASKS[auto_step].sesi,
                           AUTO_TASKS[auto_step].label,
                           AUTO_TASKS[auto_step].target_samples);
    } else {
      state = STATE_DONE;
      beep(3, 250, 120);
      char info[32];
      snprintf(info, sizeof(info), "TOTAL: %lu BARIS",
               (unsigned long)total_saved);
      oledShow("REKAM SELESAI", "DATA AMAN", info);
      Serial.println(
          F("===== SEMUA SESI (DIAM, JALAN, LARI) SELESAI DIREKAM! ====="));
    }
#else
    state = STATE_DONE;
    beep(2, 200, 100);
    oledShow("REKAM SELESAI", current_label, "CEK MICROSD");
#endif
  }
}
