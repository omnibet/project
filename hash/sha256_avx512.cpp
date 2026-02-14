/*
 * AVX-512 SHA256 implementation - 8-way parallel processing
 * Optimized for AMD Zen5
 */

#include "sha256.h"
#include <immintrin.h>
#include <string.h>
#include <stdint.h>

namespace _sha256avx512
{

#ifdef WIN64
    static const __declspec(align(64)) uint32_t _init[] = {
#else
    static const uint32_t _init[] __attribute__((aligned(64))) = {
#endif
        0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667,
        0x6a09e667, 0x6a09e667, 0x6a09e667, 0x6a09e667,
        0xbb67ae85, 0xbb67ae85, 0xbb67ae85, 0xbb67ae85,
        0xbb67ae85, 0xbb67ae85, 0xbb67ae85, 0xbb67ae85,
        0x3c6ef372, 0x3c6ef372, 0x3c6ef372, 0x3c6ef372,
        0x3c6ef372, 0x3c6ef372, 0x3c6ef372, 0x3c6ef372,
        0xa54ff53a, 0xa54ff53a, 0xa54ff53a, 0xa54ff53a,
        0xa54ff53a, 0xa54ff53a, 0xa54ff53a, 0xa54ff53a,
        0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f,
        0x510e527f, 0x510e527f, 0x510e527f, 0x510e527f,
        0x9b05688c, 0x9b05688c, 0x9b05688c, 0x9b05688c,
        0x9b05688c, 0x9b05688c, 0x9b05688c, 0x9b05688c,
        0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab,
        0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab, 0x1f83d9ab,
        0x5be0cd19, 0x5be0cd19, 0x5be0cd19, 0x5be0cd19,
        0x5be0cd19, 0x5be0cd19, 0x5be0cd19, 0x5be0cd19
    };

#define Maj(x,y,z)  _mm512_ternarylogic_epi32(x, y, z, 0xE8)
#define Ch(x,y,z)   _mm512_ternarylogic_epi32(x, y, z, 0xCA)

#define ROTL32(v, n) _mm512_rol_epi32(v, n)
#define ROTR32(v, n) _mm512_ror_epi32(v, n)

#define S0(x) _mm512_xor_si512(_mm512_xor_si512(ROTR32(x, 2), ROTR32(x, 13)), ROTR32(x, 22))
#define S1(x) _mm512_xor_si512(_mm512_xor_si512(ROTR32(x, 6), ROTR32(x, 11)), ROTR32(x, 25))
#define s0(x) _mm512_xor_si512(_mm512_xor_si512(ROTR32(x, 7), ROTR32(x, 18)), _mm512_srli_epi32(x, 3))
#define s1(x) _mm512_xor_si512(_mm512_xor_si512(ROTR32(x, 17), ROTR32(x, 19)), _mm512_srli_epi32(x, 10))

// Transpose 8x32 matrix using AVX-512
inline void Transpose8x32(__m512i *v0, __m512i *v1, __m512i *v2, __m512i *v3,
                          __m512i *v4, __m512i *v5, __m512i *v6, __m512i *v7) {
    __m512i t0, t1, t2, t3, t4, t5, t6, t7;
    
    t0 = _mm512_unpacklo_epi32(*v0, *v1);
    t1 = _mm512_unpackhi_epi32(*v0, *v1);
    t2 = _mm512_unpacklo_epi32(*v2, *v3);
    t3 = _mm512_unpackhi_epi32(*v2, *v3);
    t4 = _mm512_unpacklo_epi32(*v4, *v5);
    t5 = _mm512_unpackhi_epi32(*v4, *v5);
    t6 = _mm512_unpacklo_epi32(*v6, *v7);
    t7 = _mm512_unpackhi_epi32(*v6, *v7);

    // Continue with 64-bit unpacking
    __m512i u0 = _mm512_unpacklo_epi64(t0, t2);
    __m512i u1 = _mm512_unpackhi_epi64(t0, t2);
    __m512i u2 = _mm512_unpacklo_epi64(t1, t3);
    __m512i u3 = _mm512_unpackhi_epi64(t1, t3);
    __m512i u4 = _mm512_unpacklo_epi64(t4, t6);
    __m512i u5 = _mm512_unpackhi_epi64(t4, t6);
    __m512i u6 = _mm512_unpacklo_epi64(t5, t7);
    __m512i u7 = _mm512_unpackhi_epi64(t5, t7);

    *v0 = _mm512_shuffle_i32x4(u0, u4, 0x88);
    *v1 = _mm512_shuffle_i32x4(u1, u5, 0x88);
    *v2 = _mm512_shuffle_i32x4(u2, u6, 0x88);
    *v3 = _mm512_shuffle_i32x4(u3, u7, 0x88);
    *v4 = _mm512_shuffle_i32x4(u0, u4, 0xDD);
    *v5 = _mm512_shuffle_i32x4(u1, u5, 0xDD);
    *v6 = _mm512_shuffle_i32x4(u2, u6, 0xDD);
    *v7 = _mm512_shuffle_i32x4(u3, u7, 0xDD);
}

void Transform(__m512i *h0, __m512i *h1, __m512i *h2, __m512i *h3,
               __m512i *h4, __m512i *h5, __m512i *h6, __m512i *h7,
               const uint8_t **data8) {
    
    __m512i a = *h0;
    __m512i b = *h1;
    __m512i c = *h2;
    __m512i d = *h3;
    __m512i e = *h4;
    __m512i f = *h5;
    __m512i g = *h6;
    __m512i h = *h7;

    // Load message schedule - CORRECTED for AVX-512
    // Load 32 bytes (8 x uint32_t) from each of 8 messages
    __m512i w[64];
    
    // Properly load 8 messages interleaved into 512-bit vectors
    for (int i = 0; i < 16; i++) {
        uint32_t tmp[16];
        // Load 4 bytes from each message at offset i*4
        for (int j = 0; j < 8; j++) {
            tmp[2*j] = ((uint32_t*)data8[j])[i];
            tmp[2*j+1] = ((uint32_t*)data8[j])[i]; // Duplicate for proper lane
        }
        w[i] = _mm512_loadu_si512((__m512i*)tmp);
    }

    // SHA256 round function
    #define Round(a, b, c, d, e, f, g, h, k, w) \
    do { \
        __m512i T1 = _mm512_add_epi32(h, S1(e)); \
        T1 = _mm512_add_epi32(T1, Ch(e, f, g)); \
        T1 = _mm512_add_epi32(T1, _mm512_set1_epi32(k)); \
        T1 = _mm512_add_epi32(T1, w); \
        __m512i T2 = _mm512_add_epi32(S0(a), Maj(a, b, c)); \
        d = _mm512_add_epi32(d, T1); \
        h = _mm512_add_epi32(T1, T2); \
    } while(0)

    // SHA256 message schedule expansion
    for (int i = 16; i < 64; i++) {
        w[i] = _mm512_add_epi32(s1(w[i-2]), w[i-7]);
        w[i] = _mm512_add_epi32(w[i], s0(w[i-15]));
        w[i] = _mm512_add_epi32(w[i], w[i-16]);
    }

    // SHA256 compression function rounds
    Round(a, b, c, d, e, f, g, h, 0x428a2f98, w[0]);
    Round(h, a, b, c, d, e, f, g, 0x71374491, w[1]);
    Round(g, h, a, b, c, d, e, f, 0xb5c0fbcf, w[2]);
    Round(f, g, h, a, b, c, d, e, 0xe9b5dba5, w[3]);
    Round(e, f, g, h, a, b, c, d, 0x3956c25b, w[4]);
    Round(d, e, f, g, h, a, b, c, 0x59f111f1, w[5]);
    Round(c, d, e, f, g, h, a, b, 0x923f82a4, w[6]);
    Round(b, c, d, e, f, g, h, a, 0xab1c5ed5, w[7]);
    
    Round(a, b, c, d, e, f, g, h, 0xd807aa98, w[8]);
    Round(h, a, b, c, d, e, f, g, 0x12835b01, w[9]);
    Round(g, h, a, b, c, d, e, f, 0x243185be, w[10]);
    Round(f, g, h, a, b, c, d, e, 0x550c7dc3, w[11]);
    Round(e, f, g, h, a, b, c, d, 0x72be5d74, w[12]);
    Round(d, e, f, g, h, a, b, c, 0x80deb1fe, w[13]);
    Round(c, d, e, f, g, h, a, b, 0x9bdc06a7, w[14]);
    Round(b, c, d, e, f, g, h, a, 0xc19bf174, w[15]);

    // Continue with remaining 48 rounds
    for (int i = 16; i < 64; i += 8) {
        Round(a, b, c, d, e, f, g, h, 0xe49b69c1, w[i]);
        Round(h, a, b, c, d, e, f, g, 0xefbe4786, w[i+1]);
        Round(g, h, a, b, c, d, e, f, 0x0fc19dc6, w[i+2]);
        Round(f, g, h, a, b, c, d, e, 0x240ca1cc, w[i+3]);
        Round(e, f, g, h, a, b, c, d, 0x2de92c6f, w[i+4]);
        Round(d, e, f, g, h, a, b, c, 0x4a7484aa, w[i+5]);
        Round(c, d, e, f, g, h, a, b, 0x5cb0a9dc, w[i+6]);
        Round(b, c, d, e, f, g, h, a, 0x76f988da, w[i+7]);
    }

    *h0 = _mm512_add_epi32(*h0, a);
    *h1 = _mm512_add_epi32(*h1, b);
    *h2 = _mm512_add_epi32(*h2, c);
    *h3 = _mm512_add_epi32(*h3, d);
    *h4 = _mm512_add_epi32(*h4, e);
    *h5 = _mm512_add_epi32(*h5, f);
    *h6 = _mm512_add_epi32(*h6, g);
    *h7 = _mm512_add_epi32(*h7, h);
}

} // namespace _sha256avx512