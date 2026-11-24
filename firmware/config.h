#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// 1. KONFIGURASI KREDENSIAL WIFI & PLATFORM IOT (iot.reslab-csub.cloud)
// ============================================================================
// TODO: Isi dengan SSID dan Password WiFi Anda sebelum upload ke ESP32
#define WIFI_SSID "uhuy"
#define WIFI_PASSWORD "ozza2025"

// Access Token di bawah ini sudah otomatis mengidentifikasi perangkat di API
// https://iot.reslab-csub.cloud/api/v1/ymjOjiatNNoVAgmMlrIs/telemetry
// (Tidak perlu Device ID terpisah karena platform mengenali perangkat dari Access Token)
#define IOT_DEVICE_TOKEN "ymjOjiatNNoVAgmMlrIs"

// Pilihan protokol komunikasi ke iot.reslab-csub.cloud:
//   0 = HTTPS REST API POST (Direkomendasikan, paling stabil di ESP32)
//   1 = MQTT over TLS (Port 8883)
#define IOT_USE_MQTT 0 // (tidak dipakai lagi: iot_network.h sekarang selalu MQTT over TLS)

// --- MQTT over TLS (template RESLAB IoT Platform, topic: iot/<token>/...) ---
#define IOT_TOPIC_PREFIX "iot"
#define IOT_USE_TLS 1
#define IOT_TELEMETRY_INTERVAL_MS 10000UL // telemetri berkala
#define IOT_STATUS_HEARTBEAT_MS 300000UL  // heartbeat "online" tiap 5 menit

// --- Aktuator "lamp" (LED bawaan DevKit) & sensor PIR HC-SR501 ---
// Wiring PIR: VCC -> 5V (VIN), GND -> GND, OUT -> GPIO 27
#define IOT_LED_PIN 2
#define IOT_PIR_PIN 27
#define IOT_PIR_DEBOUNCE_MS 200UL

// Endpoint resmi berdasarkan dokumentasi RESLAB IoT Platform (berbasis
// ThingsBoard):
// - HTTP POST : https://iot.reslab-csub.cloud/api/v1/<ACCESS_TOKEN>/telemetry
// - MQTTS     : mqtts://iot.reslab-csub.cloud:8883 (Topic:
// v1/devices/me/telemetry)
//               Username = <ACCESS_TOKEN>, Password = <ACCESS_TOKEN>
#define IOT_HOST "iot.reslab-csub.cloud"
#define IOT_HTTPS_PORT 443
#define IOT_MQTTS_PORT 8883
#define IOT_MQTT_TOPIC "v1/devices/me/telemetry"

// Interval pengiriman & reconnect (non-blocking)
#define WIFI_RECONNECT_INTERVAL_MS                                             \
  10000UL // Coba reconnect WiFi tiap 10 detik bila putus
#define IOT_HEARTBEAT_INTERVAL_MS                                              \
  30000UL // Kirim heartbeat berkala tiap 30 detik

// ============================================================================
// 2. PIN MAP BLUINO ESP32 IOT STARTER SHIELD
// ============================================================================
// Catatan Hardware Bluino ESP32 IoT Starter Shield (ESP32 DevKit V1 30-Pin):
// - MPU6050 (0x68) menggunakan bus I2C hardware pada GPIO 21 (SDA) dan GPIO 22 (SCL).
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22
#define I2C_CLOCK_HZ 100000UL // Standard Mode 100 kHz agar MPU6050 di shield stabil

#define MPU6050_I2C_ADDR 0x68 // Alamat default MPU6050 (AD0 = GND)

// Slot Micro SD Card pada Bluino ESP32 Starter Shield (bus VSPI standar ESP32)
#define PIN_SD_CS 5
#define PIN_SD_SCK 18
#define PIN_SD_MISO 19
#define PIN_SD_MOSI 23

// Active Buzzer pada Bluino ESP32 Starter Shield (dikendalikan via transistor
// NPN)
// TODO: Pastikan jumper/revisi board Bluino Shield Anda (umumnya GPIO 12, GPIO
// 14, atau GPIO 2)
#define PIN_BUZZER 12
#define ENABLE_BUZZER 1 // 1 = Aktifkan beep saat aktivitas berubah, 0 = Matikan

// Indikator 3 LED Aktivitas (Diam=Merah, Jalan=Kuning, Lari=Hijau)
#define ENABLE_ACTIVITY_LEDS 1
#define PIN_LED_MERAH 25  // LED Merah  -> Diam (0)
#define PIN_LED_KUNING 26 // LED Kuning -> Jalan (1)
#define PIN_LED_HIJAU 33  // LED Hijau  -> Lari (2)

// ============================================================================
// 3. KONFIGURASI SAMPLING, WINDOWING, DAN EDGE INFERENCE
// ============================================================================
// Asumsi & Penyesuaian Dataset:
// - Dataset diambil pada frekuensi 50 Hz (interval 20 ms = 20.000 us) dalam
// satuan g.
// - Nilai akselerasi bersih pada dataset berada di rentang [-1.608 g, +1.832
// g].
// - Digunakan rentang MPU6050 +/-4g (MPU6050_RANGE_4_G) agar tidak terjadi
// saturasi
//   saat hentakan lari dan tetap memiliki resolusi tinggi.
#define SAMPLE_RATE_HZ 50
#define SAMPLE_INTERVAL_MS 20UL
#define SAMPLE_INTERVAL_US 20000UL

#define WINDOW_SIZE 100 // Jendela 2 detik @ 50 Hz
#define WINDOW_STEP                                                            \
  50 // Overlap 50% (inferensi baru setiap 1 detik / 50 sampel)
#define SMOOTHING_HISTORY_SIZE                                                 \
  3 // Majority vote 3 prediksi terakhir agar OLED tidak berkedip

// Batas filter glitch pembacaan I2C (sesuai preprocessing notebook: |a| > 4.0 g
// dianggap outlier)
#define ACCEL_OUTLIER_THRESHOLD_G 4.0f

// Posisi pemasangan alat pada tubuh (sesuai evaluasi notebook: "pinggang" atau
// "pergelangan")
#define DEVICE_MOUNT_POSITION "pinggang"

// ============================================================================
// 4. KONFIGURASI PENYIMPANAN SD CARD & MODE DATA COLLECTION (OPSIONAL)
// ============================================================================
#define SD_INFERENCE_LOG_PATH "/log_inferensi.csv"
#define SD_RAW_DATASET_PATH "/raw_dataset_esp32.csv"

// Set ke 1 jika ingin menyimpan data mentah 50 Hz ke SD Card dengan format
// kolom yang sama persis seperti dataset_aktivitas_esp32.csv:
// timestamp_ms,subjek,sesi,posisi,ax,ay,az,label
#define ENABLE_RAW_DATA_COLLECTION 0
#define RAW_DATA_SUBJEK "S07"
#define RAW_DATA_SESI 1
#define RAW_DATA_DEFAULT_LABEL "jalan"

#endif // CONFIG_H
