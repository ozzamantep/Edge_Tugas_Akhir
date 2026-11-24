/**
 * ============================================================================
 * PROYEK   : Klasifikasi Aktivitas Manusia (Diam, Jalan, Lari) Berbasis Edge
 *            Inference di ESP32 (Bluino ESP32 IoT Starter Shield)
 * BOARD    : ESP32 DevKit V1 (30-Pin) pada Bluino ESP32 IoT Starter Shield
 * SENSOR   : MPU6050 (I2C 0x68, Akselerometer ax, ay, az dalam satuan g @ 50 Hz)
 * OUTPUT   : 3 LED Indikator (Merah/Kuning/Hijau), Active Buzzer, MicroSD Card (SPI),
 *            dan Telemetri Cloud ke https://iot.reslab-csub.cloud/
 *
 * CATATAN KOMPILASI:
 * - Sketch ini mendukung library Adafruit_MPU6050 bila sudah
 *   terinstal, NAMUN juga memiliki driver I2C built-in (<Wire.h>) otomatis
 *   sehingga BISA LANGSUNG DI-COMPILE & DI-UPLOAD tanpa error meskipun library
 *   eksternal belum diinstal di Arduino IDE!
 * ============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#if __has_include(<PubSubClient.h>)
#define HAS_PUBSUBCLIENT 1
#include <PubSubClient.h>
#else
#define HAS_PUBSUBCLIENT 0
#endif

// Gunakan library Adafruit_MPU6050 bila tersedia, atau driver I2C register langsung (<Wire.h>) bila belum diinstal
#if __has_include(<Adafruit_MPU6050.h>) && __has_include(<Adafruit_Sensor.h>)
#define USE_ADAFRUIT_MPU6050 1
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#else
#define USE_ADAFRUIT_MPU6050 0
#endif

#include "config.h"
#include "model.h"
#include "features.h"
#include "sdlog.h"

#if USE_ADAFRUIT_MPU6050
static constexpr float GRAVITY_MS2 = 9.80665f;
Adafruit_MPU6050 mpu;
#else
// Driver I2C langsung untuk MPU6050 pada alamat 0x68 (Range +/-4g = 8192 LSB/g, DLPF 21 Hz)
static constexpr float MPU6050_LSB_PER_G_4G = 8192.0f;

static bool mpu6050InitDirect() {
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    // Wake up MPU6050 (PWR_MGMT_1 reg 0x6B = 0x00)
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    Wire.write(0x6B);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(10);

    // Set Digital Low Pass Filter 21 Hz (CONFIG reg 0x1A = 0x04)
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    Wire.write(0x1A);
    Wire.write(0x04);
    Wire.endTransmission();

    // Set Accelerometer Range +/-4g (ACCEL_CONFIG reg 0x1C = 0x08 -> AFS_SEL = 1)
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    Wire.write(0x1C);
    Wire.write(0x08);
    Wire.endTransmission();
    return true;
}

static bool mpu6050ReadAccelG(float& ax_g, float& ay_g, float& az_g) {
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    Wire.write(0x3B); // ACCEL_XOUT_H
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    if (Wire.requestFrom((uint8_t)MPU6050_I2C_ADDR, (uint8_t)6, (uint8_t)true) != 6) {
        return false;
    }
    const int16_t raw_ax = (int16_t)((Wire.read() << 8) | Wire.read());
    const int16_t raw_ay = (int16_t)((Wire.read() << 8) | Wire.read());
    const int16_t raw_az = (int16_t)((Wire.read() << 8) | Wire.read());

    ax_g = (float)raw_ax / MPU6050_LSB_PER_G_4G;
    ay_g = (float)raw_ay / MPU6050_LSB_PER_G_4G;
    az_g = (float)raw_az / MPU6050_LSB_PER_G_4G;
    return true;
}
#endif

// Objek periferal
SdLogger sdLogger;

// ============================================================================
// KONEKSI WIFI & PENGIRIMAN IOT LANGSUNG DI .INO (CORE 0 NON-BLOCKING)
// ============================================================================
struct IoTPacket {
    int activity_code;
    float confidence;
    uint32_t steps;
    float sma;
};

static QueueHandle_t iotQueue = nullptr;
static int lastSentActivity = -1;
static uint32_t lastSentMs = 0;

static void iotEnqueue(uint32_t now_ms, int activity_code, float confidence, uint32_t steps, float sma) {
    const bool changed = (activity_code != lastSentActivity);
    const bool heartbeat = (now_ms - lastSentMs) >= IOT_TELEMETRY_INTERVAL_MS;
    if (!changed && !heartbeat) return;

    IoTPacket pkt = { activity_code, confidence, steps, sma };
    lastSentActivity = activity_code;
    lastSentMs = now_ms;
    if (iotQueue != nullptr) {
        xQueueOverwrite(iotQueue, &pkt);
    }
}

static void iotNetworkTask(void* param) {
    (void)param;
    Serial.printf("[WiFi] Menghubungkan ke SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t lastWifiCheck = 0;
#if HAS_PUBSUBCLIENT
    WiFiClientSecure tlsClient;
    tlsClient.setInsecure();
    PubSubClient mqttClient(tlsClient);
    mqttClient.setServer(IOT_HOST, IOT_MQTTS_PORT);
    mqttClient.setBufferSize(1024);
    String topicTelemetry = String(IOT_TOPIC_PREFIX) + "/" + IOT_DEVICE_TOKEN + "/telemetry";
    String topicStatus    = String(IOT_TOPIC_PREFIX) + "/" + IOT_DEVICE_TOKEN + "/status";
#endif

    IoTPacket pkt;
    char payload[160];

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            if (millis() - lastWifiCheck >= WIFI_RECONNECT_INTERVAL_MS) {
                lastWifiCheck = millis();
                Serial.println(F("[WiFi] Mencoba reconnect WiFi..."));
                WiFi.disconnect();
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            }
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

#if HAS_PUBSUBCLIENT
        if (!mqttClient.connected()) {
            String clientId = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
            Serial.printf("[MQTT] Menyambung ke %s:%u ... ", IOT_HOST, (unsigned)IOT_MQTTS_PORT);
            if (mqttClient.connect(clientId.c_str(), IOT_DEVICE_TOKEN, IOT_DEVICE_TOKEN,
                                   topicStatus.c_str(), 1, true, "offline")) {
                Serial.println(F("OK!"));
                mqttClient.publish(topicStatus.c_str(), "online", true);
            } else {
                Serial.printf("gagal (rc=%d)\n", mqttClient.state());
                vTaskDelay(pdMS_TO_TICKS(3000));
                continue;
            }
        }
        mqttClient.loop();
#endif

        if (xQueueReceive(iotQueue, &pkt, pdMS_TO_TICKS(200)) == pdTRUE) {
            snprintf(
                payload, sizeof(payload),
                "{\"activity_code\":%d,\"confidence\":%.3f,\"steps\":%lu,\"sma\":%.3f}",
                pkt.activity_code,
                pkt.confidence,
                (unsigned long)pkt.steps,
                pkt.sma
            );

#if HAS_PUBSUBCLIENT
            bool ok = mqttClient.publish(topicTelemetry.c_str(), payload);
            Serial.printf("[MQTT] %s (%s) -> %s\n", topicTelemetry.c_str(), ok ? "OK" : "FAIL", payload);
#else
            WiFiClientSecure httpsClient;
            httpsClient.setInsecure();
            HTTPClient http;
            String url = String("https://") + IOT_HOST + "/api/v1/" + IOT_DEVICE_TOKEN + "/telemetry";
            if (http.begin(httpsClient, url)) {
                http.addHeader("Content-Type", "application/json");
                int code = http.POST((uint8_t*)payload, strlen(payload));
                http.end();
                Serial.printf("[HTTPS] HTTP %d -> %s\n", code, payload);
            }
#endif
        }
    }
}

static void startIoTNetwork() {
    iotQueue = xQueueCreate(1, sizeof(IoTPacket));
    xTaskCreatePinnedToCore(iotNetworkTask, "IoTTask", 16384, nullptr, 1, nullptr, 0);
}

// Buffer jendela sinyal akselerometer (100 sampel = 2.0 detik @ 50 Hz)
static float win_ax[WINDOW_SIZE];
static float win_ay[WINDOW_SIZE];
static float win_az[WINDOW_SIZE];
static int win_fill_count = 0;
static int samples_since_last_infer = 0;

// Nilai sampel valid terakhir untuk menggantikan outlier/glitch I2C (> 4.0 g)
static float last_valid_ax = 0.0f;
static float last_valid_ay = 0.0f;
static float last_valid_az = 1.0f;

// Riwayat prediksi untuk smoothing (Majority Vote 3 jendela terakhir)
static int pred_history[SMOOTHING_HISTORY_SIZE] = { 0, 0, 0 };
static int pred_history_count = 0;
static int pred_history_idx = 0;
static int current_smoothed_activity = -1;

// Indikator Buzzer non-blocking
struct BuzzerPattern {
    uint8_t beeps_remaining;
    bool pin_state_high;
    uint32_t next_toggle_ms;
    uint16_t on_duration_ms;
    uint16_t off_duration_ms;
};
static BuzzerPattern buzzerState = { 0, false, 0, 80, 80 };

// Statistik langkah & waktu sampling
static uint32_t total_steps = 0;
static uint32_t last_sample_us = 0;

/**
 * @brief Memicu pola beep non-blocking pada buzzer saat aktivitas berubah:
 *        - DIAM  (0): 1x beep pendek (60 ms)
 *        - JALAN (1): 2x beep sedang (80 ms)
 *        - LARI  (2): 3x beep cepat  (110 ms)
 */
static void updateActivityLeds(int activity_id) {
#if ENABLE_ACTIVITY_LEDS
    digitalWrite(PIN_LED_MERAH,  (activity_id == 0) ? HIGH : LOW);
    digitalWrite(PIN_LED_KUNING, (activity_id == 1) ? HIGH : LOW);
    digitalWrite(PIN_LED_HIJAU,  (activity_id == 2) ? HIGH : LOW);
#else
    (void)activity_id;
#endif
}

static void triggerBuzzerForActivity(int activity_id) {
#if ENABLE_BUZZER
    if (activity_id == 0) {
        buzzerState.beeps_remaining = 1;
        buzzerState.on_duration_ms = 60;
        buzzerState.off_duration_ms = 60;
    } else if (activity_id == 1) {
        buzzerState.beeps_remaining = 2;
        buzzerState.on_duration_ms = 80;
        buzzerState.off_duration_ms = 70;
    } else {
        buzzerState.beeps_remaining = 3;
        buzzerState.on_duration_ms = 110;
        buzzerState.off_duration_ms = 60;
    }
    buzzerState.pin_state_high = true;
    digitalWrite(PIN_BUZZER, HIGH);
    buzzerState.next_toggle_ms = millis() + buzzerState.on_duration_ms;
#else
    (void)activity_id;
#endif
}

static void updateBuzzerNonBlocking() {
#if ENABLE_BUZZER
    if (buzzerState.beeps_remaining == 0 && !buzzerState.pin_state_high) {
        return;
    }
    const uint32_t now_ms = millis();
    if ((int32_t)(now_ms - buzzerState.next_toggle_ms) >= 0) {
        if (buzzerState.pin_state_high) {
            digitalWrite(PIN_BUZZER, LOW);
            buzzerState.pin_state_high = false;
            buzzerState.beeps_remaining--;
            if (buzzerState.beeps_remaining > 0) {
                buzzerState.next_toggle_ms = now_ms + buzzerState.off_duration_ms;
            }
        } else if (buzzerState.beeps_remaining > 0) {
            digitalWrite(PIN_BUZZER, HIGH);
            buzzerState.pin_state_high = true;
            buzzerState.next_toggle_ms = now_ms + buzzerState.on_duration_ms;
        }
    }
#endif
}

/**
 * @brief Majority vote atas 3 prediksi terakhir agar tampilan OLED tidak berkedip.
 */
static int computeMajorityVote(int new_pred) {
    pred_history[pred_history_idx] = new_pred;
    pred_history_idx = (pred_history_idx + 1) % SMOOTHING_HISTORY_SIZE;
    if (pred_history_count < SMOOTHING_HISTORY_SIZE) {
        pred_history_count++;
    }

    int counts[MODEL_NUM_CLASSES] = { 0, 0, 0 };
    for (int i = 0; i < pred_history_count; ++i) {
        int c = pred_history[i];
        if (c >= 0 && c < MODEL_NUM_CLASSES) {
            counts[c]++;
        }
    }

    int best_class = new_pred;
    int best_count = counts[new_pred];
    for (int c = 0; c < MODEL_NUM_CLASSES; ++c) {
        if (counts[c] > best_count) {
            best_count = counts[c];
            best_class = c;
        }
    }
    return best_class;
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println(F("\n========================================================"));
    Serial.println(F("  KLASIFIKASI AKTIVITAS MANUSIA - EDGE INFERENCE ESP32"));
    Serial.println(F("  Board : Bluino ESP32 IoT Starter Shield"));
    Serial.println(F("========================================================"));

#if ENABLE_BUZZER
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
#endif

#if ENABLE_ACTIVITY_LEDS
    pinMode(PIN_LED_MERAH, OUTPUT);
    pinMode(PIN_LED_KUNING, OUTPUT);
    pinMode(PIN_LED_HIJAU, OUTPUT);
    // Tes nyala singkat saat boot lalu default ke Diam (Merah)
    digitalWrite(PIN_LED_MERAH, HIGH);
    digitalWrite(PIN_LED_KUNING, HIGH);
    digitalWrite(PIN_LED_HIJAU, HIGH);
    delay(250);
    updateActivityLeds(0);
#endif

    // 1. Inisialisasi Bus I2C (MPU6050 0x68)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(I2C_CLOCK_HZ);
    delay(100);

    // Scan perangkat I2C yang terpasang di bus (SDA=21, SCL=22) untuk diagnostik
    Serial.print(F("[I2C] Memindai bus I2C (SDA=21, SCL=22): "));
    int i2c_found = 0;
    for (uint8_t addr = 1; addr < 127; ++addr) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("0x%02X ", addr);
            i2c_found++;
        }
    }
    if (i2c_found == 0) Serial.print(F("TIDAK ADA"));
    Serial.println();

    // 2. Inisialisasi Sensor MPU6050 pada alamat 0x68
#if USE_ADAFRUIT_MPU6050
    if (!mpu.begin(MPU6050_I2C_ADDR, &Wire)) {
        Serial.println(F("[MPU6050] ERROR: Sensor MPU6050 tidak terdeteksi di 0x68!"));
        while (true) {
            delay(500);
        }
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
#else
    if (!mpu6050InitDirect()) {
        Serial.println(F("[MPU6050] ERROR: Sensor MPU6050 tidak terdeteksi di 0x68!"));
        while (true) {
            delay(500);
        }
    }
#endif
    Serial.println(F("[MPU6050] Siap pada alamat 0x68 (Range: +/-4g, Filter: 21Hz)."));

    // 3. Inisialisasi Micro SD Card (tetap lanjut berjalan walaupun SD tidak terpasang)
    sdLogger.begin();

    // 4. Inisialisasi WiFi & Task Pengiriman IoT Non-blocking (Core 0)
    startIoTNetwork();

    last_sample_us = micros();
}

void loop() {
    updateBuzzerNonBlocking();

    // Penjadwalan sampling presisi 50 Hz (tiap 20.000 mikrodetik = 20 ms)
    const uint32_t now_us = micros();
    if ((uint32_t)(now_us - last_sample_us) < SAMPLE_INTERVAL_US) {
        return;
    }
    if ((uint32_t)(now_us - last_sample_us) > (SAMPLE_INTERVAL_US * 3)) {
        last_sample_us = now_us;
    } else {
        last_sample_us += SAMPLE_INTERVAL_US;
    }

    // 1. Baca akselerometer MPU6050 dalam satuan g
    float ax_g = last_valid_ax;
    float ay_g = last_valid_ay;
    float az_g = last_valid_az;

#if USE_ADAFRUIT_MPU6050
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    ax_g = a.acceleration.x / GRAVITY_MS2;
    ay_g = a.acceleration.y / GRAVITY_MS2;
    az_g = a.acceleration.z / GRAVITY_MS2;
#else
    mpu6050ReadAccelG(ax_g, ay_g, az_g);
#endif

    // 2. Proteksi outlier / glitch I2C (identik dengan preprocessing dataset: |a| > 4.0 g)
    if (isnan(ax_g) || fabsf(ax_g) > ACCEL_OUTLIER_THRESHOLD_G) ax_g = last_valid_ax;
    else last_valid_ax = ax_g;

    if (isnan(ay_g) || fabsf(ay_g) > ACCEL_OUTLIER_THRESHOLD_G) ay_g = last_valid_ay;
    else last_valid_ay = ay_g;

    if (isnan(az_g) || fabsf(az_g) > ACCEL_OUTLIER_THRESHOLD_G) az_g = last_valid_az;
    else last_valid_az = az_g;

#if ENABLE_RAW_DATA_COLLECTION
    sdLogger.logRawSample(millis(), ax_g, ay_g, az_g);
#endif

    // 3. Masukkan sampel ke buffer jendela (100 sampel)
    if (win_fill_count < WINDOW_SIZE) {
        win_ax[win_fill_count] = ax_g;
        win_ay[win_fill_count] = ay_g;
        win_az[win_fill_count] = az_g;
        win_fill_count++;
        if (win_fill_count < WINDOW_SIZE) {
            return;
        }
        samples_since_last_infer = WINDOW_STEP;
    } else {
        win_ax[WINDOW_SIZE - 1] = ax_g;
        win_ay[WINDOW_SIZE - 1] = ay_g;
        win_az[WINDOW_SIZE - 1] = az_g;
        samples_since_last_infer++;
    }

    if (samples_since_last_infer < WINDOW_STEP) {
        memmove(&win_ax[0], &win_ax[1], (WINDOW_SIZE - 1) * sizeof(float));
        memmove(&win_ay[0], &win_ay[1], (WINDOW_SIZE - 1) * sizeof(float));
        memmove(&win_az[0], &win_az[1], (WINDOW_SIZE - 1) * sizeof(float));
        return;
    }

    // 4. EKSTRAKSI FITUR & EDGE INFERENCE RANDOM FOREST (SETIAP 50 SAMPEL = 1.0 DETIK)
    samples_since_last_infer = 0;
    const uint32_t infer_start_us = micros();

    float features[MODEL_NUM_FEATURES];
    extract_features_window(win_ax, win_ay, win_az, WINDOW_SIZE, features);

    float probs[MODEL_NUM_CLASSES];
    float confidence = 0.0f;
    const int raw_pred = predict_activity(features, &confidence, probs);
    const int smoothed_pred = computeMajorityVote(raw_pred);
    const uint32_t infer_duration_us = micros() - infer_start_us;

    if (smoothed_pred == 1 || smoothed_pred == 2) {
        total_steps += count_steps_in_segment(
            win_ax, win_ay, win_az,
            WINDOW_SIZE - WINDOW_STEP, WINDOW_SIZE,
            features[15]
        );
    }

    updateActivityLeds(smoothed_pred);

    if (smoothed_pred != current_smoothed_activity) {
        current_smoothed_activity = smoothed_pred;
        triggerBuzzerForActivity(smoothed_pred);
    }

    const uint32_t now_ms = millis();

    // 5. Simpan log ke MicroSD Card
    sdLogger.logInference(now_ms, smoothed_pred, raw_pred, confidence, probs, total_steps);

    // 6. Jadwalkan pengiriman ke platform IoT (langsung dari .ino)
    iotEnqueue(now_ms, smoothed_pred, confidence, total_steps, features[20]);

    // 7. Cetak ringkasan ke Serial Monitor
    Serial.printf(
        "[EDGE-ML] t=%lums | Raw=%s | Smooth=%s | Conf=%.1f%% (D:%.2f J:%.2f L:%.2f) | SMA=%.2fg | Steps=%lu | dt=%luus\n",
        (unsigned long)now_ms,
        ACTIVITY_LABELS_UPPER[raw_pred],
        ACTIVITY_LABELS_UPPER[smoothed_pred],
        confidence * 100.0f,
        probs[0], probs[1], probs[2],
        features[20],
        (unsigned long)total_steps,
        (unsigned long)infer_duration_us
    );

    memmove(&win_ax[0], &win_ax[1], (WINDOW_SIZE - 1) * sizeof(float));
    memmove(&win_ay[0], &win_ay[1], (WINDOW_SIZE - 1) * sizeof(float));
    memmove(&win_az[0], &win_az[1], (WINDOW_SIZE - 1) * sizeof(float));
}
