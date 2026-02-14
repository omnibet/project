/*
 * SIMD CPU Feature Detection for AMD Zen5 Optimization
 * Runtime detection of AVX2, AVX-512, and other CPU capabilities
 */

#ifndef SIMD_FEATURES_H
#define SIMD_FEATURES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// CPU feature flags
typedef struct {
    bool sse2;
    bool ssse3;
    bool avx;
    bool avx2;
    bool avx512f;        // AVX-512 Foundation
    bool avx512dq;       // AVX-512 Doubleword and Quadword Instructions
    bool avx512bw;       // AVX-512 Byte and Word Instructions
    bool avx512vl;       // AVX-512 Vector Length Extensions
    bool avx512cd;       // AVX-512 Conflict Detection
    bool avx512vnni;     // AVX-512 Vector Neural Network Instructions
    bool avx512bf16;     // AVX-512 BFLOAT16 Instructions
    bool is_zen5;        // AMD Zen5 architecture detected
    bool is_intel;
    bool is_amd;
} simd_features_t;

// Global feature detection result
extern simd_features_t g_cpu_features;

// Initialize and detect CPU features
void simd_detect_features(void);

// Query functions
bool simd_has_sse2(void);
bool simd_has_avx2(void);
bool simd_has_avx512f(void);
bool simd_has_avx512_full(void);  // Checks for F+DQ+BW+VL
bool simd_is_zen5(void);

// Print detected features (for debugging)
void simd_print_features(void);

#ifdef __cplusplus
}
#endif

#endif // SIMD_FEATURES_H
