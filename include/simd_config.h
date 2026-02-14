/*
 * SIMD Configuration Macros for Conditional Compilation
 * Defines feature macros based on compiler flags
 */

#ifndef SIMD_CONFIG_H
#define SIMD_CONFIG_H

// Detect compiler
#if defined(__GNUC__) || defined(__clang__)
    #define COMPILER_GCC_COMPATIBLE 1
#endif

#if defined(_MSC_VER)
    #define COMPILER_MSVC 1
#endif

// Detect architecture-specific builds
#if defined(__znver5__) || defined(__znver5)
    #define TARGET_ZEN5 1
#endif

// AVX-512 Feature Detection from Compiler Flags
#if defined(__AVX512F__) && defined(__AVX512DQ__) && defined(__AVX512BW__) && defined(__AVX512VL__)
    #define ENABLE_AVX512 1
    #define ENABLE_AVX2 1
    #define ENABLE_SSE2 1
    #define SIMD_VECTOR_SIZE 512
#elif defined(__AVX2__)
    #define ENABLE_AVX2 1
    #define ENABLE_SSE2 1
    #define SIMD_VECTOR_SIZE 256
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    #define ENABLE_SSE2 1
    #define SIMD_VECTOR_SIZE 128
#else
    #define SIMD_VECTOR_SIZE 0
#endif

// Alignment macros
#ifdef COMPILER_GCC_COMPATIBLE
    #define ALIGN_16 __attribute__((aligned(16)))
    #define ALIGN_32 __attribute__((aligned(32)))
    #define ALIGN_64 __attribute__((aligned(64)))
#elif defined(COMPILER_MSVC)
    #define ALIGN_16 __declspec(align(16))
    #define ALIGN_32 __declspec(align(32))
    #define ALIGN_64 __declspec(align(64))
#else
    #define ALIGN_16
    #define ALIGN_32
    #define ALIGN_64
#endif

// Force inline
#ifdef COMPILER_GCC_COMPATIBLE
    #define FORCE_INLINE inline __attribute__((always_inline))
#elif defined(COMPILER_MSVC)
    #define FORCE_INLINE __forceinline
#else
    #define FORCE_INLINE inline
#endif

// Compiler hints
#ifdef COMPILER_GCC_COMPATIBLE
    #define LIKELY(x)   __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x)   (x)
    #define UNLIKELY(x) (x)
#endif

// SIMD intrinsics headers
#if defined(ENABLE_AVX512)
    #include <immintrin.h>
    #define SIMD_WIDTH_BYTES 64
    #define SIMD_WIDTH_UINT32 16
    #define SIMD_WIDTH_UINT64 8
#elif defined(ENABLE_AVX2)
    #include <immintrin.h>
    #define SIMD_WIDTH_BYTES 32
    #define SIMD_WIDTH_UINT32 8
    #define SIMD_WIDTH_UINT64 4
#elif defined(ENABLE_SSE2)
    #include <emmintrin.h>
    #include <tmmintrin.h>
    #define SIMD_WIDTH_BYTES 16
    #define SIMD_WIDTH_UINT32 4
    #define SIMD_WIDTH_UINT64 2
#endif

#endif // SIMD_CONFIG_H
