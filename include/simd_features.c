/*
 * SIMD CPU Feature Detection Implementation
 * Uses CPUID instruction to detect CPU capabilities
 */

#include "simd_features.h"
#include <stdio.h>
#include <string.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#define HAS_CPUID 1
#endif

// Global CPU features
simd_features_t g_cpu_features = {0};

#ifdef HAS_CPUID

#if defined(_MSC_VER)
#include <intrin.h>
#define cpuid(info, x) __cpuidex(info, x, 0)
#else
#include <cpuid.h>
static void cpuid(int info[4], int function_id) {
    __cpuid_count(function_id, 0, info[0], info[1], info[2], info[3]);
}
#endif

static void detect_vendor(char *vendor) {
    int info[4];
    cpuid(info, 0);
    memcpy(vendor, &info[1], 4);
    memcpy(vendor + 4, &info[3], 4);
    memcpy(vendor + 8, &info[2], 4);
    vendor[12] = '\0';
}

static bool is_amd_cpu(const char *vendor) {
    return strcmp(vendor, "AuthenticAMD") == 0;
}

static bool is_intel_cpu(const char *vendor) {
    return strcmp(vendor, "GenuineIntel") == 0;
}

void simd_detect_features(void) {
    int info[4];
    char vendor[13];
    
    // Reset features
    memset(&g_cpu_features, 0, sizeof(g_cpu_features));
    
    // Get vendor string
    detect_vendor(vendor);
    g_cpu_features.is_amd = is_amd_cpu(vendor);
    g_cpu_features.is_intel = is_intel_cpu(vendor);
    
    // Get feature flags from CPUID
    cpuid(info, 1);
    
    // ECX features (info[2])
    g_cpu_features.sse2 = (info[3] & (1 << 26)) != 0;
    g_cpu_features.ssse3 = (info[2] & (1 << 9)) != 0;
    g_cpu_features.avx = (info[2] & (1 << 28)) != 0;
    
    // Check for AVX2 and AVX-512
    cpuid(info, 7);
    
    // EBX features (info[1])
    g_cpu_features.avx2 = (info[1] & (1 << 5)) != 0;
    g_cpu_features.avx512f = (info[1] & (1 << 16)) != 0;
    g_cpu_features.avx512dq = (info[1] & (1 << 17)) != 0;
    g_cpu_features.avx512cd = (info[1] & (1 << 28)) != 0;
    g_cpu_features.avx512bw = (info[1] & (1 << 30)) != 0;
    g_cpu_features.avx512vl = (info[1] & (1 << 31)) != 0;
    
    // ECX features (info[2])
    g_cpu_features.avx512vnni = (info[2] & (1 << 11)) != 0;
    
    // EAX features from leaf 7, subleaf 1 (info[0])
    cpuid(info, 0x00000007);
    if (info[0] >= 1) {
        int info_sub[4];
        #if defined(_MSC_VER)
        __cpuidex(info_sub, 7, 1);
        #else
        __cpuid_count(7, 1, info_sub[0], info_sub[1], info_sub[2], info_sub[3]);
        #endif
        g_cpu_features.avx512bf16 = (info_sub[0] & (1 << 5)) != 0;
    }
    
    // Detect AMD Zen5 (Family 19h, Model >= 0x60)
    if (g_cpu_features.is_amd) {
        cpuid(info, 1);
        int family = ((info[0] >> 8) & 0x0F) + ((info[0] >> 20) & 0xFF);
        int model = ((info[0] >> 4) & 0x0F) | ((info[0] >> 12) & 0xF0);
        
        // Zen5 is Family 1Ah (26) or Family 19h with model >= 0x60
        g_cpu_features.is_zen5 = (family == 26) || (family == 25 && model >= 0x60);
    }
}

#else  // No CPUID support

void simd_detect_features(void) {
    // Fallback: assume basic features based on compile-time flags
    memset(&g_cpu_features, 0, sizeof(g_cpu_features));
    
    #if defined(__SSE2__)
    g_cpu_features.sse2 = true;
    #endif
    
    #if defined(__AVX2__)
    g_cpu_features.avx2 = true;
    g_cpu_features.avx = true;
    #endif
    
    #if defined(__AVX512F__)
    g_cpu_features.avx512f = true;
    #endif
    
    #if defined(__AVX512DQ__)
    g_cpu_features.avx512dq = true;
    #endif
    
    #if defined(__AVX512BW__)
    g_cpu_features.avx512bw = true;
    #endif
    
    #if defined(__AVX512VL__)
    g_cpu_features.avx512vl = true;
    #endif
}

#endif  // HAS_CPUID

// Query functions
bool simd_has_sse2(void) {
    return g_cpu_features.sse2;
}

bool simd_has_avx2(void) {
    return g_cpu_features.avx2;
}

bool simd_has_avx512f(void) {
    return g_cpu_features.avx512f;
}

bool simd_has_avx512_full(void) {
    return g_cpu_features.avx512f && 
           g_cpu_features.avx512dq && 
           g_cpu_features.avx512bw && 
           g_cpu_features.avx512vl;
}

bool simd_is_zen5(void) {
    return g_cpu_features.is_zen5;
}

void simd_print_features(void) {
    printf("CPU Features Detected:\n");
    printf("  Vendor: %s%s\n", 
           g_cpu_features.is_amd ? "AMD" : "",
           g_cpu_features.is_intel ? "Intel" : "");
    printf("  Zen5: %s\n", g_cpu_features.is_zen5 ? "Yes" : "No");
    printf("  SSE2: %s\n", g_cpu_features.sse2 ? "Yes" : "No");
    printf("  SSSE3: %s\n", g_cpu_features.ssse3 ? "Yes" : "No");
    printf("  AVX: %s\n", g_cpu_features.avx ? "Yes" : "No");
    printf("  AVX2: %s\n", g_cpu_features.avx2 ? "Yes" : "No");
    printf("  AVX-512F: %s\n", g_cpu_features.avx512f ? "Yes" : "No");
    printf("  AVX-512DQ: %s\n", g_cpu_features.avx512dq ? "Yes" : "No");
    printf("  AVX-512BW: %s\n", g_cpu_features.avx512bw ? "Yes" : "No");
    printf("  AVX-512VL: %s\n", g_cpu_features.avx512vl ? "Yes" : "No");
    printf("  AVX-512 Full: %s\n", simd_has_avx512_full() ? "Yes" : "No");
}

#ifdef SIMD_TEST_MAIN
int main(void) {
    simd_detect_features();
    simd_print_features();
    return 0;
}
#endif
