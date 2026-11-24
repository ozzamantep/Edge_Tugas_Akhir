#ifndef IOT_NETWORK_H
#define IOT_NETWORK_H

// IoT network manager: MQTT (TLS) + lamp + PIR + blink/ping + OTA + rule ack + Last Will
// + telemetri Edge-ML. Seluruhnya berjalan di task FreeRTOS Core 0, sehingga
// sampling 50 Hz di Core 1 tidak terganggu.
// Library: PubSubClient (Nick O'Leary), ArduinoJson 7 (Benoit Blanchon).
// Antarmuka ke sketch utama tidak berubah: begin(), isWifiConnected(), enqueueIfNeeded().

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <time.h>
#include "config.h"
#include "model.h"

#define FW_TITLE   "ESP32 Activity+PIR node"
#define FW_VERSION "2.0.0"
#define FW_BOARD   "esp32"

#if IOT_USE_TLS
static const char* const IOT_ROOT_CA = R"PEM(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)PEM";
#endif

struct MlSnapshot {
    bool     valid = false;
    bool     changed = false;   // aktivitas berubah -> kirim segera
    int      activity = 0;
    float    confidence = 0.0f; // 0..1
    float    probs[3] = {0, 0, 0};
    float    sma = 0.0f;
    uint32_t steps = 0;
};

class IoTNetworkManager {
public:
    void begin() {
        pinMode(IOT_LED_PIN, OUTPUT);
        pinMode(IOT_PIR_PIN, INPUT);
        setLamp(false);

        String base = String(IOT_TOPIC_PREFIX) + "/" + IOT_DEVICE_TOKEN;
        topicTelemetry = base + "/telemetry";
        topicDesired   = base + "/desired";
        topicCommands  = base + "/commands/";
        topicErrors    = base + "/errors";
        topicFirmware  = base + "/firmware";
        topicStatus    = base + "/status";
        topicRules     = base + "/rules/";

        selfRef() = this;
        xTaskCreatePinnedToCore(taskEntry, "IoTNetTask", 16384, this, 1, nullptr, 0);
    }

    bool isWifiConnected() const { return WiFi.status() == WL_CONNECTED; }

    // Dipanggil dari loop() Core 1 setiap 1 detik (hasil inferensi terbaru).
    void enqueueIfNeeded(uint32_t now_ms, int activity, float confidence,
                         const float* probs, float sma, uint32_t steps) {
        (void)now_ms;
        portENTER_CRITICAL(&mux);
        if (!snap.valid || snap.activity != activity) snap.changed = true;
        snap.valid = true;
        snap.activity = activity;
        snap.confidence = confidence;
        for (int i = 0; i < 3; i++) snap.probs[i] = probs[i];
        snap.sma = sma;
        snap.steps = steps;
        portEXIT_CRITICAL(&mux);
    }

private:
#if IOT_USE_TLS
    WiFiClientSecure network;
#else
    WiFiClient network;
#endif
    PubSubClient mqtt{network};
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    MlSnapshot snap;

    String topicTelemetry, topicDesired, topicCommands, topicErrors, topicFirmware, topicStatus, topicRules;
    bool lampOn = false;
    bool motionState = false, lastPirReading = false;
    unsigned long lastPirChange = 0, lastSend = 0, lastStatus = 0;

    long otaUpdateId = 0, lastOtaUpdateId = 0;
    String otaUrl;
    int otaLastPercent = -1;

    static IoTNetworkManager*& selfRef() { static IoTNetworkManager* p = nullptr; return p; }
    static void taskEntry(void* arg) { static_cast<IoTNetworkManager*>(arg)->run(); }

    // ---- helper ----------------------------------------------------------------
    void publishJson(const String& topic, JsonDocument& doc) {
        String text;
        serializeJson(doc, text);
        mqtt.publish(topic.c_str(), text.c_str());
        Serial.printf("[MQTT] -> %s  %s\n", topic.c_str(), text.c_str());
    }

    void setLamp(bool on) {
        lampOn = on;
        digitalWrite(IOT_LED_PIN, on ? HIGH : LOW);
    }

    void reportFirmwareVersion() {
        JsonDocument doc;
        doc["title"] = FW_TITLE;
        doc["version"] = FW_VERSION;
        doc["board"] = FW_BOARD;
        doc["mac"] = WiFi.macAddress();
        publishJson(topicFirmware, doc);
    }

    void reportOtaState(const char* state, int progress = -1, const String& error = "") {
        JsonDocument doc;
        doc["update"] = otaUpdateId;
        doc["state"] = state;
        if (progress >= 0) doc["progress"] = progress;
        if (error.length()) doc["error"] = error;
        publishJson(topicFirmware, doc);
        mqtt.loop();
    }

    void reportLamp() {
        JsonDocument doc;
        doc["lamp"] = lampOn ? 1 : 0;
        publishJson(topicTelemetry, doc);
    }

    void reportMotion() {
        JsonDocument doc;
        doc["motion"] = motionState ? 1 : 0;
        publishJson(topicTelemetry, doc);
    }

    void sendTelemetry() {
        JsonDocument doc;
        doc["chip_temperature"] = temperatureRead();
        doc["rssi"] = WiFi.RSSI();
        doc["lamp"] = lampOn ? 1 : 0;
        doc["motion"] = motionState ? 1 : 0;

        MlSnapshot s;
        portENTER_CRITICAL(&mux);
        s = snap;
        snap.changed = false;
        portEXIT_CRITICAL(&mux);

        if (s.valid) {
            const int a = (s.activity >= 0 && s.activity < MODEL_NUM_CLASSES) ? s.activity : 0;
            // Nama field sama dengan payload lama agar dashboard tetap cocok
            doc["activity"] = ACTIVITY_LABELS[a];
            doc["activity_code"] = a;
            doc["confidence"] = s.confidence;
            doc["prob_diam"] = s.probs[0];
            doc["prob_jalan"] = s.probs[1];
            doc["prob_lari"] = s.probs[2];
            doc["sma"] = s.sma;
            doc["steps"] = s.steps;
            doc["position"] = DEVICE_MOUNT_POSITION;
            doc["event"] = s.changed ? "activity_change" : "heartbeat";
        }
        publishJson(topicTelemetry, doc);
    }

    // ---- PIR ------------------------------------------------------------------------
    void checkPir() {
        const bool reading = digitalRead(IOT_PIR_PIN) == HIGH;
        if (reading != lastPirReading) {
            lastPirReading = reading;
            lastPirChange = millis();
        }
        if (reading != motionState && millis() - lastPirChange >= IOT_PIR_DEBOUNCE_MS) {
            motionState = reading;
            Serial.printf("[PIR] %s\n", motionState ? "gerakan terdeteksi" : "tidak ada gerakan");
            if (mqtt.connected()) reportMotion();
        }
    }

    // ---- OTA ------------------------------------------------------------------------
    static void onOtaProgress(int current, int total) {
        IoTNetworkManager* s = selfRef();
        if (!s) return;
        const int percent = total > 0 ? (int)((long long)current * 100 / total) : 0;
        if (percent >= s->otaLastPercent + 10 || percent == 100) {
            s->otaLastPercent = percent;
            s->reportOtaState("DOWNLOADING", percent);
        }
    }

    void runOta() {
        Serial.printf("[OTA] download %s\n", otaUrl.c_str());
        otaLastPercent = -1;
        reportOtaState("DOWNLOADING", 0);

        httpUpdate.rebootOnUpdate(false);
        httpUpdate.onProgress(onOtaProgress);
        t_httpUpdate_return result;
        if (otaUrl.startsWith("https://")) {
            WiFiClientSecure https;
#if IOT_USE_TLS
            https.setCACert(IOT_ROOT_CA);
#else
            https.setInsecure();
#endif
            result = httpUpdate.update(https, otaUrl);
        } else {
#if IOT_USE_TLS
            reportOtaState("FAILED", -1, "Refusing http:// download in TLS mode");
            otaUpdateId = 0;
            return;
#else
            WiFiClient http;
            result = httpUpdate.update(http, otaUrl);
#endif
        }

        if (result == HTTP_UPDATE_OK) {
            reportOtaState("DOWNLOADED");
            reportOtaState("VERIFIED");
            reportOtaState("UPDATING");
            for (int i = 0; i < 10; i++) { mqtt.loop(); vTaskDelay(pdMS_TO_TICKS(50)); }
            Serial.println(F("[OTA] restart ke firmware baru"));
            ESP.restart();
        } else {
            String error = result == HTTP_UPDATE_NO_UPDATES ? String("Server sent no update")
                                                            : httpUpdate.getLastErrorString();
            Serial.printf("[OTA] gagal: %s\n", error.c_str());
            reportOtaState("FAILED", -1, error);
        }
        otaUpdateId = 0;
    }

    // ---- desired / command / rule -----------------------------------------------------
    void handleDesired(JsonDocument& desired) {
        if (!desired["lamp"].isNull()) {
            setLamp(desired["lamp"].as<float>() >= 0.5f);
            reportLamp();
        }
    }

    static bool clockIsSet() { return time(nullptr) > 1700000000; }

    template <typename T>
    static long secondsExpired(const T& command) {
        long expiresAt = command["expires_at"] | 0L;
        if (expiresAt <= 0 || !clockIsSet()) return 0;
        long late = (long)time(nullptr) - expiresAt;
        return late > 2 ? late : 0;
    }

    void handleCommand(const String& id, JsonDocument& command) {
        const char* method = command["method"] | "";
        long late = secondsExpired(command);
        if (late > 0) {
            Serial.printf("   command %s (%s) kedaluwarsa %ld s: tidak dijalankan\n", id.c_str(), method, late);
            return;
        }
        JsonDocument result;
        if (strcmp(method, "ota") == 0) {
            long update = command["params"]["update"] | 0L;
            const char* version = command["params"]["version"] | "";
            const char* title = command["params"]["title"] | "";
            if (update == lastOtaUpdateId) return;
            if (strcmp(version, FW_VERSION) == 0 && strcasecmp(title, FW_TITLE) == 0) {
                result["success"] = false;
                result["error"] = String("Already running ") + FW_TITLE + " " + FW_VERSION;
            } else if (otaUpdateId != 0) {
                result["success"] = false;
                result["error"] = "Another update is running";
            } else {
                otaUpdateId = lastOtaUpdateId = update;
                otaUrl = String(command["params"]["url"] | "");
                result["success"] = true;
                result["message"] = "Update accepted";
            }
        } else if (strcmp(method, "blink") == 0) {
            int times = command["params"]["times"] | 3;
            for (int i = 0; i < times; i++) {
                setLamp(!lampOn); vTaskDelay(pdMS_TO_TICKS(150));
                setLamp(!lampOn); vTaskDelay(pdMS_TO_TICKS(150));
            }
            result["success"] = true;
            result["blinked"] = times;
        } else if (strcmp(method, "ping") == 0) {
            result["success"] = true;
            result["uptime_ms"] = millis();
            result["version"] = FW_VERSION;
        } else {
            result["success"] = false;
            result["error"] = String("Unknown method: ") + method;
        }
        publishJson(topicCommands + id + "/response", result);
    }

    void handleRule(const String& ruleId, byte* payload, unsigned int length) {
        if (length == 0) {
            Serial.printf("   rule %s withdrawn\n", ruleId.c_str());
            return;
        }
        JsonDocument doc;
        JsonDocument reply;
        DeserializationError error = deserializeJson(doc, payload, length);
        reply["publish"] = error ? 0 : (long)(doc["publish"]["id"] | 0);
        reply["size"] = length;
        if (error) {
            reply["ok"] = false;
            reply["error"] = String("JSON error: ") + error.c_str();
        } else {
            JsonArray devices = doc["devices"].as<JsonArray>();
            String mine = WiFi.macAddress();
            int me = -1;
            bool anyMac = false;
            for (size_t i = 0; i < devices.size(); i++) {
                const char* mac = devices[i]["mac"] | "";
                if (*mac) anyMac = true;
                if (mine.equalsIgnoreCase(mac)) me = (int)i;
            }
            if (anyMac && me < 0) {
                reply["ok"] = false;
                reply["error"] = "MAC " + mine + " is not in the rule";
            } else {
                reply["ok"] = true;
                reply["rule"] = (const char*)(doc["name"] | "");
                Serial.printf("   rule %s \"%s\" diterima (%u bytes, board #%d)\n", ruleId.c_str(),
                              (const char*)(doc["name"] | "?"), length, me);
            }
        }
        publishJson(topicRules + ruleId + "/ack", reply);
    }

    void onMessage(char* topic, byte* payload, unsigned int length) {
        String t(topic);
        if (t.startsWith(topicRules)) {
            Serial.printf("[MQTT] <- %s  (%u bytes)\n", topic, length);
            handleRule(t.substring(topicRules.length()), payload, length);
            return;
        }
        Serial.printf("[MQTT] <- %s  %.*s\n", topic, (int)length, (const char*)payload);
        JsonDocument doc;
        if (deserializeJson(doc, payload, length)) {
            Serial.println(F("   (bukan JSON valid, diabaikan)"));
            return;
        }
        if (t == topicDesired) {
            handleDesired(doc);
        } else if (t.startsWith(topicCommands)) {
            handleCommand(t.substring(topicCommands.length()), doc);
        } else if (t == topicErrors) {
            Serial.printf("   PLATFORM MENOLAK PESAN: %s\n", (const char*)(doc["error"] | "?"));
        }
    }

    static void mqttCallback(char* topic, byte* payload, unsigned int length) {
        if (selfRef()) selfRef()->onMessage(topic, payload, length);
    }

    // ---- koneksi ---------------------------------------------------------------------
    void connectWifi() {
        Serial.printf("[WiFi] menyambung ke %s", WIFI_SSID);
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        while (WiFi.status() != WL_CONNECTED) {
            vTaskDelay(pdMS_TO_TICKS(500));
            Serial.print(".");
        }
        Serial.printf("\n[WiFi] terhubung, IP %s\n", WiFi.localIP().toString().c_str());
    }

    void connectMqtt() {
        while (!mqtt.connected()) {
            if (WiFi.status() != WL_CONNECTED) return;
            String clientId = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
            Serial.printf("[MQTT] menyambung %s:%u sebagai %s ... ", IOT_HOST, (unsigned)IOT_MQTTS_PORT, clientId.c_str());
            if (mqtt.connect(clientId.c_str(), IOT_DEVICE_TOKEN, IOT_DEVICE_TOKEN,
                             topicStatus.c_str(), 1, true, "offline")) {
                Serial.println(F("ok"));
                mqtt.subscribe(topicDesired.c_str(), 1);
                mqtt.subscribe((topicCommands + "+").c_str(), 1);
                mqtt.subscribe(topicErrors.c_str(), 0);
                mqtt.subscribe((topicRules + "+").c_str(), 1);
                mqtt.publish(topicStatus.c_str(), "online", true);
                lastStatus = millis();
                reportFirmwareVersion();
                reportLamp();
                reportMotion();
            } else {
                Serial.printf("gagal (state %d), ulang 5 s\n", mqtt.state());
                vTaskDelay(pdMS_TO_TICKS(5000));
            }
        }
    }

    // ---- task utama --------------------------------------------------------------------
    void run() {
        connectWifi();
        configTime(0, 0, "pool.ntp.org", "time.google.com");
#if IOT_USE_TLS
        network.setCACert(IOT_ROOT_CA);
#endif
        mqtt.setServer(IOT_HOST, IOT_MQTTS_PORT);
        mqtt.setCallback(mqttCallback);
        mqtt.setBufferSize(8192);

        for (;;) {
            if (WiFi.status() != WL_CONNECTED) connectWifi();
            if (!mqtt.connected()) connectMqtt();
            mqtt.loop();

            if (otaUpdateId != 0) {
                runOta();
                continue;
            }

            checkPir();

            bool mlChanged;
            portENTER_CRITICAL(&mux);
            mlChanged = snap.valid && snap.changed;
            portEXIT_CRITICAL(&mux);

            if (mqtt.connected()) {
                if (mlChanged || millis() - lastSend >= IOT_TELEMETRY_INTERVAL_MS) {
                    lastSend = millis();
                    sendTelemetry();
                }
                if (millis() - lastStatus >= IOT_STATUS_HEARTBEAT_MS) {
                    lastStatus = millis();
                    mqtt.publish(topicStatus.c_str(), "online", true);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
};

#endif // IOT_NETWORK_H
