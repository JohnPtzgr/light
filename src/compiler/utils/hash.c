#include "hash.h"


hash128_t fastihasher(uint64_t key, size_t len){
  uint64_t h1 = 0x9e3779b97f4a7c15ULL;
  uint64_t h2 = 0xbf58476d1ce4e5b9ULL;
  return (hash128_t){.as_hash.single[0].primary = ((key * h2) ^ h1),
                     .as_hash.single[0].secondary = ((key * h1) ^ h2)};
}

uint32_t xxh32(const char* key, size_t len, uint32_t seed) {
    const uint32_t prime1 = 0x9E3779B1U;
    const uint32_t prime2 = 0x85EBCA6BU;
    const uint32_t prime3 = 0xC2B2AE35U;
    const uint32_t prime4 = 0x27D4EB2FU;
    
    uint32_t h32 = seed + prime1 + prime2;
    const uint8_t* p = (const uint8_t*)key;
    const uint8_t* end = p + len;
    
    // Process 4 bytes at a time
    for (; p <= end - 4; p += 4) {
        uint32_t lane = *(uint32_t*)p;
        lane *= prime2;
        lane = (lane << 31) | (lane >> 1);
        lane *= prime1;
        h32 ^= lane;
        h32 = (h32 << 27) | (h32 >> 5);
        h32 = h32 * prime1 + prime4;
    }
    
    // Remainder
    for (; p < end; p++) {
        h32 ^= *p * prime1;
        h32 = (h32 << 23) | (h32 >> 9);
        h32 *= prime2;
    }
    
    // Final avalanche
    h32 ^= h32 >> 15;
    h32 *= prime2;
    h32 ^= h32 >> 13;
    h32 *= prime3;
    h32 ^= h32 >> 16;
    
    return h32;
}

// Your interface
size_t xxh_hasher(const char* key, char id) {
    return xxh32(key, 16, id * 0x9E3779B1U);
}

size_t fast_hash(const char* key, char id) {
    // if (strlen(key) != 15) {
    //   return xxh32(key, strlen(key), id * 0x9E3779B1U);
    // }
    uint64_t low = *(uint64_t*)key;
    uint64_t high = *(uint64_t*)(key + 8);
    uint64_t mix = low ^ high ^ (id * 0x9E3779B97F4A7C15llu);
    
    // Just two rounds of mixing
    mix ^= mix >> 33;
    mix *= 0xFF51AFD7ED558CCDllu;
    mix ^= mix >> 33;
    
    return mix;
}

hash128_t test_hash(const void* key, size_t len) {
    uint32_t h = xxh32(key, len, 0x9E3779B9);
    return (hash128_t){.as_hash.single[0].primary = h * 0x9E3779B97F4A7C15llu,
                       .as_hash.single[0].secondary = (h ^ 0x9E3779B9) * 0x9E3779B97F4A7C15llu};
}