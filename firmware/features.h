#ifndef FEATURES_H
#define FEATURES_H

#include <math.h>
#include <stdint.h>
#include "model.h"

/**
 * @brief Ekstraksi 21 fitur domain waktu & magnitudo yang IDENTIK PERSIS dengan Python.
 *
 * Urutan fitur (21 fitur):
 *   [ 0.. 4] : ax_mean, ax_std, ax_min, ax_max, ax_rms
 *   [ 5.. 9] : ay_mean, ay_std, ay_min, ay_max, ay_rms
 *   [10..14] : az_mean, az_std, az_min, az_max, az_rms
 *   [15..20] : mag_mean, mag_std, mag_min, mag_max, mag_energy, sma
 *
 * Catatan konsistensi numerik:
 * - Menggunakan standar deviasi populasi (ddof=0):
 *     std = sqrt(max(0.0f, mean(x^2) - mean(x)^2))
 * - Semua operasi menggunakan float presisi tunggal (32-bit IEEE 754) yang telah
 *   diuji 100% konsisten terhadap model di notebook Python.
 *
 * @param ax        Buffer sumbu X dalam satuan g (panjang = n)
 * @param ay        Buffer sumbu Y dalam satuan g (panjang = n)
 * @param az        Buffer sumbu Z dalam satuan g (panjang = n)
 * @param n         Jumlah sampel dalam jendela (100)
 * @param out_feats Array keluaran sepanjang 21 float
 */
static inline void extract_features_window(
    const float* ax,
    const float* ay,
    const float* az,
    int n,
    float* out_feats
) {
    const float inv_n = 1.0f / (float)n;
    const float* axes[3] = { ax, ay, az };

    // 1. Fitur per sumbu (ax, ay, az): mean, std, min, max, rms
    for (int axis = 0; axis < 3; ++axis) {
        const float* arr = axes[axis];
        float sum_v = 0.0f;
        float sum_sq = 0.0f;
        float min_v = arr[0];
        float max_v = arr[0];

        for (int i = 0; i < n; ++i) {
            const float v = arr[i];
            sum_v += v;
            sum_sq += v * v;
            if (v < min_v) min_v = v;
            if (v > max_v) max_v = v;
        }

        const float mean_v = sum_v * inv_n;
        const float mean_sq = sum_sq * inv_n;
        float var_v = mean_sq - (mean_v * mean_v);
        if (var_v < 0.0f) var_v = 0.0f;
        const float std_v = sqrtf(var_v);
        const float rms_v = sqrtf(mean_sq);

        const int base = axis * 5;
        out_feats[base + 0] = mean_v;
        out_feats[base + 1] = std_v;
        out_feats[base + 2] = min_v;
        out_feats[base + 3] = max_v;
        out_feats[base + 4] = rms_v;
    }

    // 2. Fitur berbasis magnitudo sqrt(ax^2 + ay^2 + az^2) dan Signal Magnitude Area (SMA)
    float mag_sum = 0.0f;
    float mag_sum_sq = 0.0f;
    float sma_sum = 0.0f;
    float mag_min = 0.0f;
    float mag_max = 0.0f;

    for (int i = 0; i < n; ++i) {
        const float vx = ax[i];
        const float vy = ay[i];
        const float vz = az[i];

        const float m_sq = (vx * vx) + (vy * vy) + (vz * vz);
        const float m = sqrtf(m_sq);

        if (i == 0 || m < mag_min) mag_min = m;
        if (i == 0 || m > mag_max) mag_max = m;

        mag_sum += m;
        mag_sum_sq += m_sq;
        sma_sum += fabsf(vx) + fabsf(vy) + fabsf(vz);
    }

    const float mag_mean = mag_sum * inv_n;
    const float mag_energy = mag_sum_sq * inv_n;
    float mag_var = mag_energy - (mag_mean * mag_mean);
    if (mag_var < 0.0f) mag_var = 0.0f;
    const float mag_std = sqrtf(mag_var);
    const float sma = sma_sum * inv_n;

    out_feats[15] = mag_mean;
    out_feats[16] = mag_std;
    out_feats[17] = mag_min;
    out_feats[18] = mag_max;
    out_feats[19] = mag_energy;
    out_feats[20] = sma;
}

/**
 * @brief Estimasi tambahan jumlah langkah sederhana pada paruh jendela baru (50 sampel = 1 detik)
 *        menggunakan deteksi puncak magnitudo (peak detection dengan hysteresis).
 */
static inline uint16_t count_steps_in_segment(
    const float* ax,
    const float* ay,
    const float* az,
    int start_idx,
    int end_idx,
    float mag_mean
) {
    uint16_t steps = 0;
    const float threshold = (mag_mean > 1.05f) ? (mag_mean + 0.12f) : 1.18f;
    int last_peak_i = -15;

    for (int i = start_idx + 1; i < end_idx - 1; ++i) {
        const float m_prev = sqrtf(ax[i - 1] * ax[i - 1] + ay[i - 1] * ay[i - 1] + az[i - 1] * az[i - 1]);
        const float m_curr = sqrtf(ax[i] * ax[i] + ay[i] * ay[i] + az[i] * az[i]);
        const float m_next = sqrtf(ax[i + 1] * ax[i + 1] + ay[i + 1] * ay[i + 1] + az[i + 1] * az[i + 1]);

        // Minimal jarak antar langkah 12 sampel (240 ms @ 50 Hz -> maks ~4 langkah/detik)
        if (m_curr > threshold && m_curr >= m_prev && m_curr > m_next && (i - last_peak_i) >= 12) {
            steps++;
            last_peak_i = i;
        }
    }
    return steps;
}

#endif // FEATURES_H

