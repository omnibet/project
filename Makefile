# Makefile for keyhunt with AMD Zen5 AVX-512 optimization support

# Compiler settings
CXX = g++
CC = gcc

# Common flags
COMMON_FLAGS = -m64 -Wall -Wextra -Ofast -ftree-vectorize -ffast-math
COMMON_CXX_FLAGS = $(COMMON_FLAGS) -Wno-deprecated-copy
COMMON_C_FLAGS = $(COMMON_FLAGS) -Wno-unused-parameter

# Architecture-specific flags
ZEN5_FLAGS = -march=znver5 -mtune=znver5 -mavx2 -mavx512f -mavx512dq -mavx512bw -mavx512vl
ZEN5_AVX2_FLAGS = -march=znver3 -mtune=znver5 -mavx2
GENERIC_FLAGS = -march=native -mtune=native
LEGACY_SSE_FLAGS = -mssse3

# Default architecture
ARCH_FLAGS ?= $(GENERIC_FLAGS) $(LEGACY_SSE_FLAGS)

# Link-time optimization
LTO_FLAGS = -flto

# Libraries
LIBS = -lm -lpthread

# Object files
ALL_OBJ = oldbloom.o bloom.o base58.o rmd160.o sha3.o keccak.o xxhash.o util.o \
          Int.o Point.o SECP256K1.o IntMod.o Random.o IntGroup.o \
          hash/ripemd160.o hash/sha256.o hash/ripemd160_sse.o hash/sha256_sse.o \
          simd_features.o

AVX512_OBJ = hash/sha256_avx512.o hash/ripemd160_avx512.o

# Default target
default: ARCH_FLAGS = $(GENERIC_FLAGS) $(LEGACY_SSE_FLAGS)
default: keyhunt

# AMD Zen5 targets
zen5-full: ARCH_FLAGS = $(ZEN5_FLAGS)
zen5-full: keyhunt-zen5

zen5-avx2: ARCH_FLAGS = $(ZEN5_AVX2_FLAGS)
zen5-avx2: keyhunt

generic: ARCH_FLAGS = $(GENERIC_FLAGS) $(LEGACY_SSE_FLAGS)
generic: keyhunt

# Build rules
oldbloom.o: oldbloom/bloom.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) $(LTO_FLAGS) -c $< -o $@

bloom.o: bloom/bloom.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) $(LTO_FLAGS) -c $< -o $@

base58.o: base58/base58.c
	$(CC) $(COMMON_C_FLAGS) $(ARCH_FLAGS) -c $< -o $@

rmd160.o: rmd160/rmd160.c
	$(CC) $(COMMON_C_FLAGS) $(ARCH_FLAGS) -c $< -o $@

sha3.o: sha3/sha3.c
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) -c $< -o $@

keccak.o: sha3/keccak.c
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) -c $< -o $@

xxhash.o: xxhash/xxhash.c
	$(CC) $(COMMON_C_FLAGS) $(ARCH_FLAGS) -c $< -o $@

util.o: util.c
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) -c $< -o $@

Int.o: secp256k1/Int.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) -c $< -o $@

Point.o: secp256k1/Point.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) -c $< -o $@

SECP256K1.o: secp256k1/SECP256K1.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) -c $< -o $@

IntMod.o: secp256k1/IntMod.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) -c $< -o $@

Random.o: secp256k1/Random.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) $(LTO_FLAGS) -c $< -o $@

IntGroup.o: secp256k1/IntGroup.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) $(LTO_FLAGS) -c $< -o $@

hash/ripemd160.o: hash/ripemd160.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) $(LTO_FLAGS) -c $< -o $@

hash/sha256.o: hash/sha256.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) $(LTO_FLAGS) -c $< -o $@

hash/ripemd160_sse.o: hash/ripemd160_sse.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) $(LTO_FLAGS) -c $< -o $@

hash/sha256_sse.o: hash/sha256_sse.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) $(LTO_FLAGS) -c $< -o $@

simd_features.o: include/simd_features.c
	$(CC) $(COMMON_C_FLAGS) $(ARCH_FLAGS) -c $< -o $@

hash/sha256_avx512.o: hash/sha256_avx512.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ZEN5_FLAGS) $(LTO_FLAGS) -c $< -o $@

hash/ripemd160_avx512.o: hash/ripemd160_avx512.cpp
	$(CXX) $(COMMON_CXX_FLAGS) $(ZEN5_FLAGS) $(LTO_FLAGS) -c $< -o $@

keyhunt: $(ALL_OBJ)
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) -o keyhunt keyhunt.cpp $(ALL_OBJ) $(LIBS)
	rm -f *.o

keyhunt-zen5: $(ALL_OBJ) $(AVX512_OBJ)
	$(CXX) $(COMMON_CXX_FLAGS) $(ZEN5_FLAGS) -o keyhunt keyhunt.cpp $(ALL_OBJ) $(AVX512_OBJ) $(LIBS)
	rm -f *.o

legacy:
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) $(LTO_FLAGS) -c oldbloom/bloom.cpp -o oldbloom.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) $(LTO_FLAGS) -c bloom/bloom.cpp -o bloom.o
	$(CC) $(GENERIC_FLAGS) -Wno-unused-result -Ofast -ftree-vectorize -c base58/base58.c -o base58.o
	$(CC) $(COMMON_C_FLAGS) $(GENERIC_FLAGS) -c xxhash/xxhash.c -o xxhash.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) -c util.c -o util.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) -c sha3/sha3.c -o sha3.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) -c sha3/keccak.c -o keccak.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) -c hashing.c -o hashing.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) -c gmp256k1/Int.cpp -o Int.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) -c gmp256k1/Point.cpp -o Point.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) -c gmp256k1/GMP256K1.cpp -o GMP256K1.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) -c gmp256k1/IntMod.cpp -o IntMod.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) $(LTO_FLAGS) -c gmp256k1/Random.cpp -o Random.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) $(LTO_FLAGS) -c gmp256k1/IntGroup.cpp -o IntGroup.o
	$(CXX) $(COMMON_CXX_FLAGS) $(GENERIC_FLAGS) -o keyhunt keyhunt_legacy.cpp base58.o bloom.o oldbloom.o xxhash.o util.o Int.o Point.o GMP256K1.o IntMod.o IntGroup.o Random.o hashing.o sha3.o keccak.o -lm -lpthread -lcrypto -lgmp
	rm -f *.o

bsgsd: $(ALL_OBJ)
	$(CXX) $(COMMON_CXX_FLAGS) $(ARCH_FLAGS) -o bsgsd bsgsd.cpp $(ALL_OBJ) $(LIBS)
	rm -f *.o

check-cpu: include/simd_features.c
	$(CC) $(COMMON_C_FLAGS) -DSIMD_TEST_MAIN -o check-cpu include/simd_features.c
	./check-cpu
	rm -f check-cpu

clean:
	rm -f keyhunt bsgsd *.o hash/*.o secp256k1/*.o bloom/*.o

help:
	@echo "Available build targets:"
	@echo "  make               - Build with generic optimizations (default)"
	@echo "  make generic       - Same as default"
	@echo "  make zen5-full     - Build for AMD Zen5 with full AVX-512 support"
	@echo "  make zen5-avx2     - Build for AMD Zen5 with AVX2 (no AVX-512)"
	@echo "  make legacy        - Build legacy version with GMP"
	@echo "  make bsgsd         - Build BSGSD mode"
	@echo "  make check-cpu     - Detect and display CPU features"
	@echo "  make clean         - Remove build artifacts"
	@echo ""
	@echo "Compiler requirements:"
	@echo "  zen5-full: GCC 11+ or Clang 14+ (for AVX-512 support)"
	@echo "  zen5-avx2: GCC 4.9+ or Clang 3.4+"
	@echo "  generic:   GCC 4.8+ or Clang 3.3+"

.PHONY: default zen5-full zen5-avx2 generic legacy bsgsd check-cpu clean help
