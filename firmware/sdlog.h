#ifndef SDLOG_H
#define SDLOG_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "config.h"
#include "model.h"

class SdLogger {
public:
    SdLogger() : sd_ready_(false) {}

    bool begin() {
        SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
        if (!SD.begin(PIN_SD_CS, SPI)) {
            sd_ready_ = false;
            Serial.println(F("[SD] Kartu SD tidak terdeteksi. Sistem tetap berjalan tanpa log SD."));
            return false;
        }
        sd_ready_ = true;
        Serial.println(F("[SD] Kartu SD berhasil diinisialisasi."));

        // Buat header CSV log inferensi jika file belum ada
        if (!SD.exists(SD_INFERENCE_LOG_PATH)) {
            File f = SD.open(SD_INFERENCE_LOG_PATH, FILE_WRITE);
            if (f) {
                f.println(F("timestamp_ms,aktivitas,aktivitas_raw,confidence,prob_diam,prob_jalan,prob_lari,posisi,steps"));
                f.close();
            }
        }

#if ENABLE_RAW_DATA_COLLECTION
        // Buat header CSV data mentah (identik dengan dataset_aktivitas_esp32.csv)
        if (!SD.exists(SD_RAW_DATASET_PATH)) {
            File f = SD.open(SD_RAW_DATASET_PATH, FILE_WRITE);
            if (f) {
                f.println(F("timestamp_ms,subjek,sesi,posisi,ax,ay,az,label"));
                f.close();
            }
        }
#endif
        return true;
    }

    bool isReady() const {
        return sd_ready_;
    }

    /**
     * @brief Simpan satu baris hasil inferensi ke SD Card (dengan fallback aman bila SD dicabut)
     */
    bool logInference(
        uint32_t timestamp_ms,
        int smoothed_class,
        int raw_class,
        float confidence,
        const float* probs,
        uint32_t total_steps
    ) {
        if (!sd_ready_) return false;

        File f = SD.open(SD_INFERENCE_LOG_PATH, FILE_APPEND);
        if (!f) {
            // Jika gagal membuka file (mis. SD card dilepas saat jalan), tandai tidak siap tanpa merusak loop
            sd_ready_ = false;
            return false;
        }

        char line[160];
        snprintf(
            line, sizeof(line),
            "%lu,%s,%s,%.4f,%.4f,%.4f,%.4f,%s,%lu",
            (unsigned long)timestamp_ms,
            ACTIVITY_LABELS[smoothed_class],
            ACTIVITY_LABELS[raw_class],
            confidence,
            probs[0], probs[1], probs[2],
            DEVICE_MOUNT_POSITION,
            (unsigned long)total_steps
        );
        f.println(line);
        f.close();
        return true;
    }

#if ENABLE_RAW_DATA_COLLECTION
    /**
     * @brief Mode opsional: simpan sampel mentah 50 Hz dengan format identik dataset_aktivitas_esp32.csv
     */
    bool logRawSample(uint32_t timestamp_ms, float ax, float ay, float az, const char* label_override = nullptr) {
        if (!sd_ready_) return false;
        File f = SD.open(SD_RAW_DATASET_PATH, FILE_APPEND);
        if (!f) {
            sd_ready_ = false;
            return false;
        }
        const char* lbl = (label_override != nullptr) ? label_override : RAW_DATA_DEFAULT_LABEL;
        char line[128];
        snprintf(
            line, sizeof(line),
            "%lu,%s,%d,%s,%.3f,%.3f,%.3f,%s",
            (unsigned long)timestamp_ms,
            RAW_DATA_SUBJEK,
            (int)RAW_DATA_SESI,
            DEVICE_MOUNT_POSITION,
            ax, ay, az,
            lbl
        );
        f.println(line);
        f.close();
        return true;
    }
#endif

private:
    bool sd_ready_;
};

#endif // SDLOG_H

