#ifndef MODEL_H
#define MODEL_H

// AUTO-GENERATED EDGE ML MODEL HEADER FOR ESP32
// Model   : RandomForestClassifier (12 trees, max_depth=6, balanced)
// Dataset : dataset_gabungan.csv (dataset2 + dataset5 = 34.349 baris @ 50 Hz)
#include <stdint.h>

#define MODEL_NUM_FEATURES 21
#define MODEL_NUM_CLASSES 3
#define MODEL_NUM_TREES 12

static const char* const ACTIVITY_LABELS[MODEL_NUM_CLASSES] = { "diam", "jalan", "lari" };
static const char* const ACTIVITY_LABELS_UPPER[MODEL_NUM_CLASSES] = { "DIAM", "JALAN", "LARI" };

static const float FEATURE_MEAN[MODEL_NUM_FEATURES] = { 0.30847393f, 0.35821628f, -0.39007188f, 1.08761311f, 0.56760556f, 0.98942814f, 0.26223625f, 0.51140592f, 1.65012051f, 1.05747708f, -0.11003915f, 0.20317669f, -0.51502326f, 0.36762156f, 0.25921351f, 1.22644776f, 0.28456188f, 0.73328122f, 1.96907404f, 1.80967148f, 1.73190626f };
static const float FEATURE_SCALE[MODEL_NUM_FEATURES] = { 0.27862613f, 0.44138735f, 0.76961644f, 0.96557351f, 0.41683848f, 0.20944885f, 0.27702870f, 0.48584182f, 0.72698042f, 0.22381204f, 0.19726165f, 0.23041255f, 0.59113418f, 0.36755186f, 0.27964235f, 0.37438338f, 0.29044620f, 0.31656438f, 1.02597476f, 1.33693795f, 0.69499609f };

static inline void score_activity(const float* input, float* output) {
    float scores[MODEL_NUM_CLASSES] = {0.0f, 0.0f, 0.0f};
    // Tree 1
    if (input[6] <= 0.03720645f) { // ay_std
        scores[0] += 1.00000000f;
        scores[1] += 0.00000000f;
        scores[2] += 0.00000000f;
    } else {
        if (input[14] <= 0.24555999f) { // az_rms
            if (input[18] <= 2.19962382f) { // mag_max
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 0.29761905f;
                scores[2] += 0.70238095f;
            }
        } else {
            if (input[20] <= 1.81042004f) { // sma
                scores[0] += 0.00000000f;
                scores[1] += 0.62893082f;
                scores[2] += 0.37106918f;
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 0.00000000f;
                scores[2] += 1.00000000f;
            }
        }
    }
    // Tree 2
    if (input[1] <= 0.33478737f) { // ax_std
        if (input[7] <= 0.87950003f) { // ay_min
            if (input[15] <= 1.08719850f) { // mag_mean
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 0.62893082f;
                scores[2] += 0.37106918f;
            }
        } else {
            scores[0] += 1.00000000f;
            scores[1] += 0.00000000f;
            scores[2] += 0.00000000f;
        }
    } else {
        if (input[12] <= -0.60249996f) { // az_min
            scores[0] += 0.00000000f;
            scores[1] += 0.00000000f;
            scores[2] += 1.00000000f;
        } else {
            scores[0] += 0.00000000f;
            scores[1] += 0.62893082f;
            scores[2] += 0.37106918f;
        }
    }
    // Tree 3
    if (input[15] <= 1.09785199f) { // mag_mean
        if (input[17] <= 0.97541898f) { // mag_min
            if (input[7] <= 0.08200000f) { // ay_min
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            }
        } else {
            scores[0] += 1.00000000f;
            scores[1] += 0.00000000f;
            scores[2] += 0.00000000f;
        }
    } else {
        if (input[19] <= 1.96373630f) { // mag_energy
            scores[0] += 0.00000000f;
            scores[1] += 0.29761905f;
            scores[2] += 0.70238095f;
        } else {
            scores[0] += 0.00000000f;
            scores[1] += 0.00000000f;
            scores[2] += 1.00000000f;
        }
    }
    // Tree 4
    if (input[1] <= 0.37102717f) { // ax_std
        if (input[17] <= 0.97541898f) { // mag_min
            if (input[2] <= -0.88549995f) { // ax_min
                scores[0] += 0.00000000f;
                scores[1] += 0.45871560f;
                scores[2] += 0.54128440f;
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            }
        } else {
            scores[0] += 1.00000000f;
            scores[1] += 0.00000000f;
            scores[2] += 0.00000000f;
        }
    } else {
        scores[0] += 0.00000000f;
        scores[1] += 0.00000000f;
        scores[2] += 1.00000000f;
    }
    // Tree 5
    if (input[15] <= 1.13647842f) { // mag_mean
        if (input[8] <= 1.02900004f) { // ay_max
            if (input[3] <= 0.46550000f) { // ax_max
                if (input[5] <= 0.90057999f) { // ay_mean
                    scores[0] += 1.00000000f;
                    scores[1] += 0.00000000f;
                    scores[2] += 0.00000000f;
                } else {
                    scores[0] += 1.00000000f;
                    scores[1] += 0.00000000f;
                    scores[2] += 0.00000000f;
                }
            } else {
                scores[0] += 0.64520049f;
                scores[1] += 0.35479951f;
                scores[2] += 0.00000000f;
            }
        } else {
            if (input[4] <= 0.12108675f) { // ax_rms
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            }
        }
    } else {
        scores[0] += 0.00000000f;
        scores[1] += 0.00000000f;
        scores[2] += 1.00000000f;
    }
    // Tree 6
    if (input[8] <= 1.05150008f) { // ay_max
        scores[0] += 1.00000000f;
        scores[1] += 0.00000000f;
        scores[2] += 0.00000000f;
    } else {
        if (input[14] <= 0.23211654f) { // az_rms
            if (input[4] <= 0.73400170f) { // ax_rms
                if (input[15] <= 1.08718884f) { // mag_mean
                    scores[0] += 0.00000000f;
                    scores[1] += 1.00000000f;
                    scores[2] += 0.00000000f;
                } else {
                    scores[0] += 0.00000000f;
                    scores[1] += 0.77220077f;
                    scores[2] += 0.22779923f;
                }
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 0.45871560f;
                scores[2] += 0.54128440f;
            }
        } else {
            if (input[11] <= 0.21740377f) { // az_std
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 0.00000000f;
                scores[2] += 1.00000000f;
            }
        }
    }
    // Tree 7
    if (input[16] <= 0.00987254f) { // mag_std
        scores[0] += 1.00000000f;
        scores[1] += 0.00000000f;
        scores[2] += 0.00000000f;
    } else {
        if (input[1] <= 0.37255079f) { // ax_std
            if (input[14] <= 0.23408328f) { // az_rms
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 0.62893082f;
                scores[2] += 0.37106918f;
            }
        } else {
            scores[0] += 0.00000000f;
            scores[1] += 0.00000000f;
            scores[2] += 1.00000000f;
        }
    }
    // Tree 8
    if (input[20] <= 1.63452506f) { // sma
        if (input[8] <= 1.03799999f) { // ay_max
            if (input[6] <= 0.00677484f) { // ay_std
                scores[0] += 1.00000000f;
                scores[1] += 0.00000000f;
                scores[2] += 0.00000000f;
            } else {
                scores[0] += 0.78434269f;
                scores[1] += 0.21565731f;
                scores[2] += 0.00000000f;
            }
        } else {
            if (input[10] <= -0.11879000f) { // az_mean
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            }
        }
    } else {
        scores[0] += 0.00000000f;
        scores[1] += 0.00000000f;
        scores[2] += 1.00000000f;
    }
    // Tree 9
    if (input[19] <= 1.79709959f) { // mag_energy
        if (input[6] <= 0.01294314f) { // ay_std
            scores[0] += 1.00000000f;
            scores[1] += 0.00000000f;
            scores[2] += 0.00000000f;
        } else {
            if (input[3] <= 0.13900000f) { // ax_max
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            }
        }
    } else {
        scores[0] += 0.00000000f;
        scores[1] += 0.00000000f;
        scores[2] += 1.00000000f;
    }
    // Tree 10
    if (input[4] <= 0.77373064f) { // ax_rms
        if (input[16] <= 0.00939420f) { // mag_std
            scores[0] += 1.00000000f;
            scores[1] += 0.00000000f;
            scores[2] += 0.00000000f;
        } else {
            if (input[12] <= -0.57350004f) { // az_min
                if (input[15] <= 1.35912025f) { // mag_mean
                    scores[0] += 0.00000000f;
                    scores[1] += 0.17482517f;
                    scores[2] += 0.82517483f;
                } else {
                    scores[0] += 0.00000000f;
                    scores[1] += 0.00000000f;
                    scores[2] += 1.00000000f;
                }
            } else {
                if (input[1] <= 0.08757305f) { // ax_std
                    scores[0] += 0.00000000f;
                    scores[1] += 1.00000000f;
                    scores[2] += 0.00000000f;
                } else {
                    scores[0] += 0.00000000f;
                    scores[1] += 1.00000000f;
                    scores[2] += 0.00000000f;
                }
            }
        }
    } else {
        scores[0] += 0.00000000f;
        scores[1] += 0.00000000f;
        scores[2] += 1.00000000f;
    }
    // Tree 11
    if (input[15] <= 1.15246689f) { // mag_mean
        if (input[1] <= 0.05818257f) { // ax_std
            if (input[6] <= 0.00657573f) { // ay_std
                scores[0] += 1.00000000f;
                scores[1] += 0.00000000f;
                scores[2] += 0.00000000f;
            } else {
                scores[0] += 0.44696970f;
                scores[1] += 0.55303030f;
                scores[2] += 0.00000000f;
            }
        } else {
            if (input[3] <= 0.13900000f) { // ax_max
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            }
        }
    } else {
        scores[0] += 0.00000000f;
        scores[1] += 0.00000000f;
        scores[2] += 1.00000000f;
    }
    // Tree 12
    if (input[16] <= 0.00987254f) { // mag_std
        scores[0] += 1.00000000f;
        scores[1] += 0.00000000f;
        scores[2] += 0.00000000f;
    } else {
        if (input[11] <= 0.22020036f) { // az_std
            if (input[2] <= -0.61800003f) { // ax_min
                if (input[6] <= 0.29940957f) { // ay_std
                    scores[0] += 0.00000000f;
                    scores[1] += 0.55970149f;
                    scores[2] += 0.44029851f;
                } else {
                    scores[0] += 0.00000000f;
                    scores[1] += 1.00000000f;
                    scores[2] += 0.00000000f;
                }
            } else {
                scores[0] += 0.00000000f;
                scores[1] += 1.00000000f;
                scores[2] += 0.00000000f;
            }
        } else {
            scores[0] += 0.00000000f;
            scores[1] += 0.00000000f;
            scores[2] += 1.00000000f;
        }
    }
    output[0] = scores[0] / (float)MODEL_NUM_TREES;
    output[1] = scores[1] / (float)MODEL_NUM_TREES;
    output[2] = scores[2] / (float)MODEL_NUM_TREES;
}

static inline int predict_activity(const float* input, float* out_confidence = nullptr, float* out_probs = nullptr) {
    float probs[MODEL_NUM_CLASSES]; score_activity(input, probs);
    int best_class = 0; float best_prob = probs[0];
    for (int i = 1; i < MODEL_NUM_CLASSES; ++i) { if (probs[i] > best_prob) { best_prob = probs[i]; best_class = i; } }
    if (out_confidence != nullptr) *out_confidence = best_prob;
    if (out_probs != nullptr) { out_probs[0] = probs[0]; out_probs[1] = probs[1]; out_probs[2] = probs[2]; }
    return best_class;
}

#endif // MODEL_H
